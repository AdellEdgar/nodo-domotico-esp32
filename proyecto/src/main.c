#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "nodo";
#define LED_PIN GPIO_NUM_2   // LED integrado del DevKit V1

static void vTareaBlink(void *pvParameters)
{
    int nivel = 0;
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    for (;;) {
        nivel = !nivel;
        gpio_set_level(LED_PIN, nivel);
        ESP_LOGI(TAG, "LED %s (nucleo %d)", nivel ? "on" : "off", xPortGetCoreID());
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    esp_chip_info_t info;
    esp_chip_info(&info);
    ESP_LOGI(TAG, "ESP32 rev %d, %d nucleos, heap libre %" PRIu32 " B",
             info.revision, info.cores, esp_get_free_heap_size());

    xTaskCreatePinnedToCore(vTareaBlink, "Blink", 3072, NULL, 1, NULL, 1);
}