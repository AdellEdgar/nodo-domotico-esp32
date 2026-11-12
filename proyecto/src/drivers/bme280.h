/**
 * @file bme280.h
 * @brief Sensor ambiental BME280 (I2C): temperatura, humedad y presión.
 */
#pragma once

#include "esp_err.h"

/**
 * @brief Añade el sensor al bus I2C, comprueba su ID y lo configura.
 * @return ESP_OK si es un BME280; ESP_ERR_NOT_SUPPORTED si es un BMP280.
 */
esp_err_t bme280_init(void);

/**
 * @brief Lee y compensa la última medida.
 * @param[out] temp Temperatura en °C.
 * @param[out] hum  Humedad relativa en %.
 * @param[out] pres Presión en hPa.
 * @return ESP_OK si la lectura es correcta.
 */
esp_err_t bme280_leer(float *temp, float *hum, float *pres);