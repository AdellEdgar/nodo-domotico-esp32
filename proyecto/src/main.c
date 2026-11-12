/**
 * @file main.c
 * @brief Arranque del nodo domótico.
 *
 * Crea los objetos de FreeRTOS, inicializa los módulos y lanza las tareas.
 * De momento incluye el pulsador, el LED RGB y una versión provisional de
 * ControlTask que cambia de modo con cada pulsación y lo indica con el LED.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "config.h"
#include "pulsador.h"
#include "led_rgb.h"
#include "servo.h"
#include "pir.h"
#include "i2c_bus.h"
#include "bh1750.h"
#include "bme280.h"
#include "mpu6050.h"

static const char *TAG = "main";

static SemaphoreHandle_t sPulsador;   // Pulsación válida: timer de antirrebote -> ControlTask

static SemaphoreHandle_t sPresencia;  // Detección del PIR: ISR -> ControlTask

/**
 * ControlTask (provisional). Espera bloqueada en sPulsador y recorre los
 * modos automático -> manual -> noche, con un color de prueba para cada uno.
 */
static void vControlTask(void *pvParameters)
{
    const char     *modos[] = { "automatico", "manual", "noche" };
    const uint32_t  color[] = { 0x00FF00,     0x0000FF, 0xFF0000 };   // verde, azul, rojo
    const uint8_t   brillo[] = { 100,         100,      10 };          // noche: tenue
    int modo = 0;

    led_rgb_aplicar(true, brillo[modo], color[modo]);
    ESP_LOGI(TAG, "Modo inicial: %s", modos[modo]);

    for (;;) {
        xSemaphoreTake(sPulsador, portMAX_DELAY);
        modo = (modo + 1) % 3;
        led_rgb_aplicar(true, brillo[modo], color[modo]);
        ESP_LOGI(TAG, "Pulsacion -> modo %s", modos[modo]);
    }
}

/** Prueba provisional: recorre varias posiciones de la persiana. */
static void vPruebaServo(void *pvParameters)
{
    const uint8_t posiciones[] = { 0, 50, 100, 50 };
    for (;;) {
        for (int i = 0; i < 4; i++) {
            servo_set_porcentaje(posiciones[i]);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

/** Prueba provisional: mide cuánto dura cada detección del PIR. */
static void vPruebaPir(void *pvParameters)
{
    for (;;) {
        xSemaphoreTake(sPresencia, portMAX_DELAY);
        TickType_t inicio = xTaskGetTickCount();
        ESP_LOGI(TAG, "Presencia detectada");

        while (pir_activo()) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        uint32_t ms = pdTICKS_TO_MS(xTaskGetTickCount() - inicio);
        ESP_LOGI(TAG, "Fin de presencia: salida activa %lu.%lu s",
                 (unsigned long)(ms / 1000), (unsigned long)((ms % 1000) / 100));
    }
}

/** Prueba provisional: muestra la luz cada segundo. */
static void vPruebaLuz(void *pvParameters)
{
    float lux;
    for (;;) {
        if (bh1750_leer_lux(&lux) == ESP_OK) {
            ESP_LOGI(TAG, "Luz: %.1f lux", lux);
        }
        float temp, hum, pres;
        if (bme280_leer(&temp, &hum, &pres) == ESP_OK) {
        ESP_LOGI(TAG, "Temp: %.2f C  Hum: %.1f %%  Pres: %.1f hPa", temp, hum, pres);
        }
        mpu6050_datos_t m;
        if (mpu6050_leer(&m) == ESP_OK) {
            ESP_LOGI(TAG, "Acel: %+.2f %+.2f %+.2f g  Giro: %+6.1f %+6.1f %+6.1f dps",
                 m.ax, m.ay, m.az, m.gx, m.gy, m.gz);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Servicio de ISR de GPIO: se instala una sola vez para todos los pines
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    i2c_bus_init();
    i2c_bus_escanear();

    bh1750_init();
    bme280_init();
    mpu6050_init();
    xTaskCreatePinnedToCore(vPruebaLuz, "PruebaLuz", 3072, NULL, 3, NULL, 1);

    sPulsador = xSemaphoreCreateBinary();
    configASSERT(sPulsador);

    led_rgb_init();
    pulsador_init(sPulsador);

    sPresencia = xSemaphoreCreateBinary();
    configASSERT(sPresencia);
    pir_init(sPresencia);
    xTaskCreatePinnedToCore(vPruebaPir, "PruebaPir", 3072, NULL, 3, NULL, 1);

    servo_init();
    //xTaskCreatePinnedToCore(vPruebaServo, "PruebaServo", 3072, NULL, 3, NULL, 1);

    // Núcleo 1: sensores y control. Prioridad 4, según la especificación
    xTaskCreatePinnedToCore(vControlTask, "ControlTask", 3072, NULL, 4, NULL, 1);
}