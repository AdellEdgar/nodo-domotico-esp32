/**
 * @file pir.h
 * @brief Sensor de presencia PIR HC-SR501 con interrupción.
 */
#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Configura el pin del PIR y su ISR.
 *
 * Cada detección (flanco de subida de la salida) libera el semáforo
 * binario recibido.
 *
 * @note Antes hay que llamar una vez a gpio_install_isr_service().
 *
 * @param sem Semáforo binario que se libera al detectar presencia.
 */
void pir_init(SemaphoreHandle_t sem);

/** @brief Devuelve true mientras la salida del PIR está activa. */
bool pir_activo(void);