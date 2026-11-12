/**
 * @file servo.c
 * @brief Servo SG90 que mueve la persiana, controlado por PWM (LEDC).
 *
 * El servo espera un pulso cada 20 ms (50 Hz). La anchura del pulso fija
 * el ángulo: SERVO_PULSO_MIN_US para 0° y SERVO_PULSO_MAX_US para 180°.
 *
 * Usa el timer 1 del LEDC a 50 Hz con 16 bits de resolución, separado del
 * timer 0 del LED (5 kHz), porque cada timer tiene una sola frecuencia.
 * Con 16 bits, cada paso de duty equivale a 20 ms / 65536 = 0,3 us.
 *
 * El servo se alimenta a 5 V desde la placa de expansión. La señal de
 * 3,3 V del ESP32 la reconoce sin adaptador de nivel.
 */
#include "servo.h"
#include "config.h"

#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "servo";

#define SERVO_MODO      LEDC_LOW_SPEED_MODE
#define SERVO_TIMER     LEDC_TIMER_1
#define SERVO_CANAL     LEDC_CHANNEL_3       // Los canales 0-2 son del LED
#define SERVO_RES_BITS  16
#define SERVO_FREQ_HZ   50
#define SERVO_PERIODO_US (1000000 / SERVO_FREQ_HZ)   // 20 000 us

/** Convierte una anchura de pulso en microsegundos al valor de duty. */
static uint32_t pulso_a_duty(uint32_t pulso_us)
{
    const uint32_t duty_max = (1U << SERVO_RES_BITS) - 1;   // 65535
    return (uint64_t)pulso_us * duty_max / SERVO_PERIODO_US;
}

void servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = SERVO_MODO,
        .timer_num       = SERVO_TIMER,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz         = SERVO_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t canal = {
        .gpio_num   = PIN_SERVO,
        .speed_mode = SERVO_MODO,
        .channel    = SERVO_CANAL,
        .timer_sel  = SERVO_TIMER,
        .duty       = 0,         // Sin pulso hasta la primera orden
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&canal));

    ESP_LOGI(TAG, "Servo listo en GPIO%d (%d Hz, %d bits)",
             PIN_SERVO, SERVO_FREQ_HZ, SERVO_RES_BITS);
}

void servo_set_porcentaje(uint8_t porcentaje)
{
    if (porcentaje > 100) porcentaje = 100;

    uint32_t pulso_us = SERVO_PULSO_MIN_US +
        (uint32_t)(SERVO_PULSO_MAX_US - SERVO_PULSO_MIN_US) * porcentaje / 100;

    ledc_set_duty(SERVO_MODO, SERVO_CANAL, pulso_a_duty(pulso_us));
    ledc_update_duty(SERVO_MODO, SERVO_CANAL);

    ESP_LOGI(TAG, "Persiana al %u %% (pulso %lu us)",
             porcentaje, (unsigned long)pulso_us);
}