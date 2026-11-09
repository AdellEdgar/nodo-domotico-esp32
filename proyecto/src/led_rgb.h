/**
 * @file led_rgb.h
 * @brief LED RGB de ánodo común controlado por PWM (periférico LEDC).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/** @brief Configura el timer y los tres canales LEDC. El LED arranca apagado. */
void led_rgb_init(void);

/** @brief Fija cada color directamente (0 = apagado, 255 = máximo). */
void led_rgb_set(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Aplica un estado de luz completo.
 * @param encendida false apaga el LED sin perder el color ni el brillo.
 * @param brillo    Intensidad de 0 a 100 %.
 * @param color     Color en formato 0xRRGGBB.
 */
void led_rgb_aplicar(bool encendida, uint8_t brillo, uint32_t color);