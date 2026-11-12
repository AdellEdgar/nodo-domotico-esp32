/**
 * @file mpu6050.c
 * @brief Acelerómetro y giroscopio MPU6050 (I2C), para los gestos de TinyML.
 *
 * Igual que el BME280, se maneja con registros. El registro 0x75
 * (WHO_AM_I) devuelve 0x68 en un MPU6050 original. Algunos módulos baratos
 * llevan clones u otros chips de la familia que devuelven otro valor
 * (por ejemplo 0x70 en el MPU6500), así que se avisa de ello.
 */
#include "mpu6050.h"
#include "config.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mpu6050";

#define REG_WHO_AM_I  0x75
#define ID_MPU6050    0x68
#define TIMEOUT_MS    100
#define ID_MPU6500    0x70   // Variante habitual en módulos GY-521 actuales
#define REG_SMPLRT    0x19   // Divisor de la frecuencia de muestreo
#define REG_CONFIG    0x1A   // Filtro paso bajo (DLPF)
#define REG_GYRO_CFG  0x1B   // Escala del giroscopio
#define REG_ACCEL_CFG 0x1C   // Escala del acelerómetro
#define REG_PWR_MGMT1 0x6B   // Gestión de energía (reset, sleep, reloj)

#define VAL_RESET     0x80   // Reset de todos los registros
#define VAL_DESPERTAR 0x01   // Sale de sleep, reloj del PLL del giroscopio X
#define VAL_SMPLRT    9      // 1 kHz / (1 + 9) = 100 muestras/s
#define VAL_CONFIG    0x03   // DLPF ~44 Hz
#define VAL_GYRO_CFG  0x10   // ±1000 °/s (32,8 LSB por °/s)
#define VAL_ACCEL_CFG 0x08   // ±4 g      (8192 LSB por g)
#define REG_DATOS     0x3B      // 0x3B–0x48: acel. XYZ, temperatura, giro XYZ (14 bytes)
#define LSB_POR_G     8192.0f   // Sensibilidad con ±4 g
#define LSB_POR_GRADO 32.8f     // Sensibilidad con ±1000 °/s

static i2c_master_dev_handle_t dev;

#define REG_GIRO      0x43   // 0x43–0x48: giro XYZ (6 bytes)
#define MUESTRAS_CAL  100    // 1 s de muestras a 100 Hz

static float off_gx, off_gy, off_gz;   // Sesgo del giroscopio en reposo (°/s)

static int16_t be16(const uint8_t *p);  // Definidas al final del fichero
static esp_err_t calibrar_giro(void);

/** Lee 'len' bytes a partir del registro 'reg'. */
static esp_err_t leer_registros(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, TIMEOUT_MS);
}

/** Escribe un byte en el registro 'reg'. */
static esp_err_t escribir_registro(uint8_t reg, uint8_t valor)
{
    uint8_t buf[2] = { reg, valor };
    return i2c_master_transmit(dev, buf, sizeof(buf), TIMEOUT_MS);
}

esp_err_t mpu6050_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = MPU6050_DIR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &cfg, &dev));

    uint8_t id;
    esp_err_t err = leer_registros(REG_WHO_AM_I, &id, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No responde en 0x%02X: %s", MPU6050_DIR, esp_err_to_name(err));
        return err;
    }
    if (id == ID_MPU6500) {
        ESP_LOGI(TAG, "WHO_AM_I 0x70: es un MPU6500 (compatible para acel. y giro)");
    } else if (id != ID_MPU6050) {
        ESP_LOGW(TAG, "WHO_AM_I 0x%02X desconocido: puede ser un clon", id);
    }

    // Reset y despertar: tras encenderse está en modo sleep y no mide
    escribir_registro(REG_PWR_MGMT1, VAL_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    err = escribir_registro(REG_PWR_MGMT1, VAL_DESPERTAR);

    // Muestreo, filtro y escalas
    if (err == ESP_OK) err = escribir_registro(REG_SMPLRT, VAL_SMPLRT);
    if (err == ESP_OK) err = escribir_registro(REG_CONFIG, VAL_CONFIG);
    if (err == ESP_OK) err = escribir_registro(REG_GYRO_CFG, VAL_GYRO_CFG);
    if (err == ESP_OK) err = escribir_registro(REG_ACCEL_CFG, VAL_ACCEL_CFG);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(50));   // Deja que se estabilice y haga las primeras medidas

    uint8_t r[4];   // 0x19 a 0x1C: smplrt, config, gyro_cfg, accel_cfg
    leer_registros(REG_SMPLRT, r, sizeof(r));
    ESP_LOGI(TAG, "Registros: smplrt=%u config=0x%02X gyro=0x%02X accel=0x%02X",
             r[0], r[1], r[2], r[3]);
    
    err = calibrar_giro();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error calibrando el giroscopio: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Listo en 0x%02X (WHO_AM_I 0x%02X): ±4 g, ±1000 °/s, 100 Hz",
             MPU6050_DIR, id);
    return ESP_OK;
}

/** Junta dos bytes big endian en un entero con signo de 16 bits. */
static int16_t be16(const uint8_t *p)
{
    return (int16_t)((p[0] << 8) | p[1]);
}

esp_err_t mpu6050_leer(mpu6050_datos_t *d)
{
    uint8_t b[14];
    esp_err_t err = leer_registros(REG_DATOS, b, sizeof(b));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error de lectura: %s", esp_err_to_name(err));
        return err;
    }

    d->ax = be16(&b[0])  / LSB_POR_G;
    d->ay = be16(&b[2])  / LSB_POR_G;
    d->az = be16(&b[4])  / LSB_POR_G;
    // b[6] y b[7]: temperatura interna, no se usa
    d->gx = be16(&b[8])  / LSB_POR_GRADO - off_gx;
    d->gy = be16(&b[10]) / LSB_POR_GRADO - off_gy;
    d->gz = be16(&b[12]) / LSB_POR_GRADO - off_gz;
    return ESP_OK;
}

/**
 * Mide el sesgo del giroscopio: media de MUESTRAS_CAL lecturas con el
 * sensor quieto. Si se mueve durante la calibración, el sesgo saldrá mal
 * hasta el siguiente reinicio.
 */
static esp_err_t calibrar_giro(void)
{
    float sx = 0, sy = 0, sz = 0;
    uint8_t b[6];

    for (int i = 0; i < MUESTRAS_CAL; i++) {
        esp_err_t err = leer_registros(REG_GIRO, b, sizeof(b));
        if (err != ESP_OK) return err;
        sx += be16(&b[0]) / LSB_POR_GRADO;
        sy += be16(&b[2]) / LSB_POR_GRADO;
        sz += be16(&b[4]) / LSB_POR_GRADO;
        vTaskDelay(pdMS_TO_TICKS(10));   // Una muestra nueva cada 10 ms
    }
    off_gx = sx / MUESTRAS_CAL;
    off_gy = sy / MUESTRAS_CAL;
    off_gz = sz / MUESTRAS_CAL;

    ESP_LOGI(TAG, "Sesgo del giroscopio: %+.2f %+.2f %+.2f dps", off_gx, off_gy, off_gz);
    return ESP_OK;
}