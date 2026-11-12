/**
 * @file bh1750.c
 * @brief Sensor de luz BH1750 (I2C), medida en lux.
 *
 * El sensor se controla con comandos de un byte. Se enciende (0x01) y se
 * pone en medida continua de alta resolución (0x10): mide sin parar, con
 * 1 lux de resolución y una medida nueva cada 120 ms. Leer consiste en
 * recibir 2 bytes (big endian) y dividir el valor entre 1,2.
 *
 * Sustituye al LDR analógico previsto en la especificación: da la luz
 * calibrada en lux y comparte el bus I2C con el resto de sensores.
 */
#include "bh1750.h"
#include "config.h"
#include "i2c_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bh1750";

#define CMD_ENCENDER          0x01
#define CMD_CONTINUO_ALTA_RES 0x10
#define TIMEOUT_MS            100

static i2c_master_dev_handle_t dev;

static esp_err_t enviar_comando(uint8_t cmd)
{
    return i2c_master_transmit(dev, &cmd, 1, TIMEOUT_MS);
}

esp_err_t bh1750_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BH1750_DIR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &cfg, &dev));

    esp_err_t err = enviar_comando(CMD_ENCENDER);
    if (err == ESP_OK) err = enviar_comando(CMD_CONTINUO_ALTA_RES);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No responde en 0x%02X: %s", BH1750_DIR, esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(180));   // Espera a la primera medida
    ESP_LOGI(TAG, "BH1750 listo en 0x%02X (continuo, alta resolucion)", BH1750_DIR);
    return ESP_OK;
}

esp_err_t bh1750_leer_lux(float *lux)
{
    uint8_t buf[2];
    esp_err_t err = i2c_master_receive(dev, buf, sizeof(buf), TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error de lectura: %s", esp_err_to_name(err));
        return err;
    }
    uint16_t bruto = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = bruto / 1.2f;
    return ESP_OK;
}