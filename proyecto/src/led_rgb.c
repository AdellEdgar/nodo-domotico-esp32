/**
 * @file led_rgb.c
 * @brief LED RGB de ánodo común controlado por PWM (periférico LEDC).
 *
 * El LED es de ánodo común: el común va a 3V3 y cada color se enciende
 * con su pin a nivel bajo. Para trabajar con brillo directo (255 = máximo),
 * cada canal tiene activada la opción output_invert, que invierte la
 * señal por hardware.
 *
 * PWM: un timer LEDC a 5 kHz con 8 bits de resolución, compartido por
 * los tres canales (uno por color). El servo usará otro timer, porque
 * necesita 50 Hz.
 *
 * Solo una tarea debe llamar a este módulo (ahora ControlTask y, en el
 * proyecto final, ActuatorTask), así que no necesita mutex.
 */
#include "led_rgb.h"
#include "config.h"

#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "led_rgb";

#define LED_MODO   LEDC_LOW_SPEED_MODE
#define LED_TIMER  LEDC_TIMER_0
#define LED_RES    LEDC_TIMER_8_BIT        // Duty de 0 a 255

static const gpio_num_t     PIN[3]   = { PIN_LED_R, PIN_LED_G, PIN_LED_B };
static const ledc_channel_t CANAL[3] = { LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2 };

void led_rgb_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LED_MODO,
        .timer_num       = LED_TIMER,
        .duty_resolution = LED_RES,
        .freq_hz         = LED_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t canal = {
            .gpio_num   = PIN[i],
            .speed_mode = LED_MODO,
            .channel    = CANAL[i],
            .timer_sel  = LED_TIMER,
            .duty       = 0,                 // Arranca apagado
            .hpoint     = 0,
            .flags.output_invert = 1,        // Ánodo común: se enciende a nivel bajo
        };
        ESP_ERROR_CHECK(ledc_channel_config(&canal));
    }

    ESP_LOGI(TAG, "LED RGB listo en GPIO%d/%d/%d (%d Hz, 8 bits)",
             PIN_LED_R, PIN_LED_G, PIN_LED_B, LED_PWM_FREQ_HZ);
}

void led_rgb_set(uint8_t r, uint8_t g, uint8_t b)
{
    const uint8_t valor[3] = { r, g, b };
    for (int i = 0; i < 3; i++) {
        ledc_set_duty(LED_MODO, CANAL[i], valor[i]);
        ledc_update_duty(LED_MODO, CANAL[i]);
    }
}

void led_rgb_aplicar(bool encendida, uint8_t brillo, uint32_t color)
{
    if (!encendida || brillo == 0) {
        led_rgb_set(0, 0, 0);
        return;
    }
    if (brillo > 100) brillo = 100;

    // Cada componente del color se escala según el brillo
    uint8_t r = ((color >> 16) & 0xFF) * brillo / 100;
    uint8_t g = ((color >> 8)  & 0xFF) * brillo / 100;
    uint8_t b = ( color        & 0xFF) * brillo / 100;
    led_rgb_set(r, g, b);
}