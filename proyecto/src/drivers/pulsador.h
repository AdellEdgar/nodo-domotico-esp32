/**
 * @file pulsador.h
 * @brief Pulsador con interrupción y antirrebote por software timer.
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**
 * @brief Configura el pulsador, su ISR y el timer de antirrebote.
 *
 * Cada pulsación válida (ya filtrada de rebotes) libera el semáforo
 * binario recibido, en el que espera la tarea que debe atenderla.
 *
 * @note Antes hay que llamar una vez a gpio_install_isr_service().
 *
 * @param sem Semáforo binario que se libera en cada pulsación válida.
 */
void pulsador_init(SemaphoreHandle_t sem);