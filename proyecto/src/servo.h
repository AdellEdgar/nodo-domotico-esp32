/**
 * @file servo.h
 * @brief Servo SG90 que mueve la persiana, controlado por PWM (LEDC).
 */
#pragma once

#include <stdint.h>

/** @brief Configura el timer y el canal LEDC del servo. */
void servo_init(void);

/**
 * @brief Mueve la persiana a una posición.
 * @param porcentaje De 0 (0°) a 100 (180°). Los valores mayores se limitan a 100.
 */
void servo_set_porcentaje(uint8_t porcentaje);