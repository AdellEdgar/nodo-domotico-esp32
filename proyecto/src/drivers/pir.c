/**
 * @file pir.c
 * @brief Sensor de presencia PIR HC-SR501 con interrupción.
 *
 * El HC-SR501 se alimenta a 5 V, pero su salida es de 3,3 V, así que se
 * conecta directamente al ESP32. Al detectar movimiento, la salida pasa a
 * nivel alto durante el tiempo fijado con su potenciómetro de tiempo.
 *
 * La ISR salta en el flanco de subida y solo libera el semáforo
 * sPresencia (procesamiento diferido): el trabajo lo hace la tarea que
 * espera en él. A diferencia del pulsador, no necesita antirrebote,
 * porque la salida la genera el circuito del sensor y no un contacto.
 */
#include "pir.h"
#include "config.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"

static const char *TAG = "pir";

static SemaphoreHandle_t sPresencia;   // Se libera en cada detección

/** ISR del flanco de subida: solo avisa a la tarea. */
static void IRAM_ATTR isr_pir(void *arg)
{
    BaseType_t despertar = pdFALSE;
    xSemaphoreGiveFromISR(sPresencia, &despertar);
    portYIELD_FROM_ISR(despertar);
}

void pir_init(SemaphoreHandle_t sem)
{
    sPresencia = sem;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_PIR,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   // Nivel bajo si el sensor se desconecta
        .intr_type    = GPIO_INTR_POSEDGE,      // Flanco de subida = detección
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_PIR, isr_pir, NULL));

    ESP_LOGI(TAG, "PIR listo en GPIO%d", PIN_PIR);
}

bool pir_activo(void)
{
    return gpio_get_level(PIN_PIR) == 1;
}