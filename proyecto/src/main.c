/**
 * @file main.c
 * @brief Arranque del nodo domótico.
 *
 * Crea los objetos de FreeRTOS, inicializa los módulos y lanza las tareas.
 * De momento solo incluye el pulsador y una versión provisional de
 * ControlTask que cambia de modo con cada pulsación.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "config.h"
#include "pulsador.h"

static const char *TAG = "main";

static SemaphoreHandle_t sPulsador;   // Pulsación válida: timer de antirrebote -> ControlTask

/**
 * ControlTask (provisional). Espera bloqueada en sPulsador, sin consumir
 * CPU, y recorre los modos automático -> manual -> noche.
 */
static void vControlTask(void *pvParameters)
{
    const char *modos[] = { "automatico", "manual", "noche" };
    int modo = 0;

    for (;;) {
        xSemaphoreTake(sPulsador, portMAX_DELAY);
        modo = (modo + 1) % 3;
        ESP_LOGI(TAG, "Pulsacion -> modo %s", modos[modo]);
    }
}

void app_main(void)
{
    // Servicio de ISR de GPIO: se instala una sola vez para todos los pines
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    sPulsador = xSemaphoreCreateBinary();
    configASSERT(sPulsador);

    pulsador_init(sPulsador);

    // Núcleo 1: sensores y control. Prioridad 4, según la especificación
    xTaskCreatePinnedToCore(vControlTask, "ControlTask", 3072, NULL, 4, NULL, 1);
}