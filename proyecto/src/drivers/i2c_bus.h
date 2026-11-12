/**
 * @file i2c_bus.h
 * @brief Bus I2C compartido por los sensores.
 */
#pragma once

#include "driver/i2c_master.h"

/** @brief Crea el bus I2C en los pines de config.h. */
void i2c_bus_init(void);

/** @brief Devuelve el handle del bus, para añadir dispositivos. */
i2c_master_bus_handle_t i2c_bus_handle(void);

/** @brief Recorre todas las direcciones y muestra las que responden. */
void i2c_bus_escanear(void);