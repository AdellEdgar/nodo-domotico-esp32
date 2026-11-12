/**
 * @file mpu6050.h
 * @brief Acelerómetro y giroscopio MPU6050 (I2C), para los gestos de TinyML.
 */
#pragma once

#include "esp_err.h"

/** @brief Una medida completa de los 6 ejes. */
typedef struct {
    float ax, ay, az;   ///< Aceleración en g (1 g = 9,81 m/s²)
    float gx, gy, gz;   ///< Velocidad de giro en °/s
} mpu6050_datos_t;

/**
 * @brief Añade el sensor al bus I2C, comprueba su ID, lo despierta y lo configura.
 * @return ESP_OK si responde como MPU6050.
 */
esp_err_t mpu6050_init(void);

/**
 * @brief Lee la última medida de los 6 ejes.
 * @param[out] d Aceleración y velocidad de giro.
 * @return ESP_OK si la lectura es correcta.
 */
esp_err_t mpu6050_leer(mpu6050_datos_t *d);