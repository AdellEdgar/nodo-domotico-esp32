#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "prueba_pwm";

static const int PIN_RGB[3] = { 25, 26, 27 };                 // R, G, B
static const ledc_channel_t CANAL[3] = { LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2 };

static void rgb_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,    // 0-255
        .freq_hz         = 5000,                // 5 kHz
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t canal = {
            .gpio_num   = PIN_RGB[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = CANAL[i],
            .timer_sel  = LEDC_TIMER_0,
            .duty       = 0,
            .hpoint     = 0,
            .flags.output_invert = 1,           // LED de ánodo común
        };
        ESP_ERROR_CHECK(ledc_channel_config(&canal));
    }
}

static void rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t valor[3] = { r, g, b };
    for (int i = 0; i < 3; i++) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, CANAL[i], valor[i]);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, CANAL[i]);
    }
}

void app_main(void)
{
    rgb_init();
    const char *nombre[3] = { "rojo", "verde", "azul" };

    for (;;) {
        // 1. Cada color sube y baja de brillo
        for (int c = 0; c < 3; c++) {
            ESP_LOGI(TAG, "Fundido %s", nombre[c]);
            for (int v = 0; v <= 255; v += 5) {
                rgb_set(c == 0 ? v : 0, c == 1 ? v : 0, c == 2 ? v : 0);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            for (int v = 255; v >= 0; v -= 5) {
                rgb_set(c == 0 ? v : 0, c == 1 ? v : 0, c == 2 ? v : 0);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }

        // 2. Mezclas
        ESP_LOGI(TAG, "Amarillo");  rgb_set(255, 255, 0);   vTaskDelay(pdMS_TO_TICKS(1500));
        ESP_LOGI(TAG, "Cian");      rgb_set(0, 255, 255);   vTaskDelay(pdMS_TO_TICKS(1500));
        ESP_LOGI(TAG, "Magenta");   rgb_set(255, 0, 255);   vTaskDelay(pdMS_TO_TICKS(1500));
        ESP_LOGI(TAG, "Blanco");    rgb_set(255, 255, 255); vTaskDelay(pdMS_TO_TICKS(1500));
        ESP_LOGI(TAG, "Apagado");   rgb_set(0, 0, 0);       vTaskDelay(pdMS_TO_TICKS(1500));
    }
}