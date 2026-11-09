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

static const char *TAG = "main";

static SemaphoreHandle_t sPulsador;   // Pulsación válida: timer de antirrebote -> ControlTask

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

void app_main(void)
{
    // Servicio de ISR de GPIO: se instala una sola vez para todos los pines
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    sPulsador = xSemaphoreCreateBinary();
    configASSERT(sPulsador);

    led_rgb_init();
    pulsador_init(sPulsador);

    servo_init();
    xTaskCreatePinnedToCore(vPruebaServo, "PruebaServo", 3072, NULL, 3, NULL, 1);

    // Núcleo 1: sensores y control. Prioridad 4, según la especificación
    xTaskCreatePinnedToCore(vControlTask, "ControlTask", 3072, NULL, 4, NULL, 1);
}