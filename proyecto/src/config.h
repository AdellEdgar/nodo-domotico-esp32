/**
 * @file config.h
 * @brief Pines y constantes del nodo domótico.
 *
 * Toda la configuración de hardware está aquí para poder cambiarla
 * sin tocar el código de los módulos.
 */
#pragma once

#include "driver/gpio.h"

/* ---------- Pulsador ---------- */
#define PIN_PULSADOR     GPIO_NUM_32   // Pin RTC: también despertará del deep sleep
#define ANTIRREBOTE_MS   50            // Tiempo sin rebotes para dar la pulsación por válida

/* ---------- LED RGB (ánodo común, común a 3V3) ---------- */
#define PIN_LED_R        GPIO_NUM_25
#define PIN_LED_G        GPIO_NUM_26
#define PIN_LED_B        GPIO_NUM_27
#define LED_PWM_FREQ_HZ  5000          // Frecuencia del PWM del LED (sin parpadeo visible)

/* ---------- Servo SG90 (persiana) ---------- */
#define PIN_SERVO           GPIO_NUM_13
#define SERVO_PULSO_MIN_US  500       // Pulso para 0 % (0°)
#define SERVO_PULSO_MAX_US  2500      // Pulso para 100 % (180°)

/* ---------- Sensor PIR HC-SR501 ---------- */
#define PIN_PIR          GPIO_NUM_33   // Pin RTC: despertará del deep sleep (ext0)

/* ---------- Bus I2C (BH1750, y más adelante BME280 y MPU6050) ---------- */
#define PIN_I2C_SDA      GPIO_NUM_21
#define PIN_I2C_SCL      GPIO_NUM_22
#define I2C_FREQ_HZ      400000        // 400 kHz (modo rápido)

/* ---------- Sensor de luz BH1750 (I2C) ---------- */
#define BH1750_DIR      0x23          // ADDR sin conectar (0x5C con ADDR a 3,3 V)
#define BME280_DIR      0x76          // BME280 (Grove, puente ADDR en 0x76 por defecto)