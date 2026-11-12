/**
 * @file i2c_bus.c
 * @brief Bus I2C compartido por los sensores.
 *
 * Se crea un único bus (puerto I2C 0) en GPIO21 (SDA) y GPIO22 (SCL).
 * Cada sensor se añade después como dispositivo de este bus con su
 * dirección. Los módulos de los sensores ya llevan resistencias de
 * pull-up; las internas del ESP32 se activan como refuerzo.
 */
#include "i2c_bus.h"
#include "config.h"

#include "esp_log.h"

static const char *TAG = "i2c_bus";

static i2c_master_bus_handle_t bus;

void i2c_bus_init(void)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = PIN_I2C_SDA,
        .scl_io_num        = PIN_I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,                     // Filtra picos cortos en las líneas
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
    ESP_LOGI(TAG, "Bus I2C listo (SDA=GPIO%d, SCL=GPIO%d)", PIN_I2C_SDA, PIN_I2C_SCL);
}

i2c_master_bus_handle_t i2c_bus_handle(void)
{
    return bus;
}

void i2c_bus_escanear(void)
{
    int encontrados = 0;
    ESP_LOGI(TAG, "Escaneando el bus I2C...");

    for (uint8_t dir = 0x08; dir < 0x78; dir++) {    // Rango de direcciones válidas
        if (i2c_master_probe(bus, dir, 50) == ESP_OK) {
            ESP_LOGI(TAG, "  Dispositivo encontrado en 0x%02X", dir);
            encontrados++;
        }
    }
    ESP_LOGI(TAG, "Escaneo terminado: %d dispositivo(s)", encontrados);
}