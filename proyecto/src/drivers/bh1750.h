/**
 * @file bh1750.h
 * @brief Sensor de luz BH1750 (I2C), medida en lux.
 */
#pragma once

#include "esp_err.h"

/** @brief Añade el sensor al bus I2C y lo pone en medida continua. */
esp_err_t bh1750_init(void);

/**
 * @brief Lee la última medida de luz.
 * @param[out] lux Luz ambiente en lux.
 * @return ESP_OK si la lectura es correcta.
 */
esp_err_t bh1750_leer_lux(float *lux);