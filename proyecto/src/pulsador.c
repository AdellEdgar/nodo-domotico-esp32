/**
 * @file pulsador.c
 * @brief Pulsador con interrupción y antirrebote por software timer.
 *
 * Funcionamiento (procesamiento diferido de interrupciones):
 *  1. El pin tiene la pull-up interna activada: en reposo lee 1 y al
 *     pulsar lee 0. La ISR salta en el flanco de bajada.
 *  2. La ISR no procesa nada: solo (re)arranca el timer one-shot
 *     tAntirrebote. Cada rebote del contacto vuelve a reiniciarlo.
 *  3. Cuando pasan ANTIRREBOTE_MS sin rebotes, el timer vence. Su callback
 *     comprueba que el botón sigue pulsado y libera el semáforo binario.
 *  4. La tarea que espera en ese semáforo atiende la pulsación.
 *
 * Medida previa: sin antirrebote, una sola pulsación generaba de 1 a 4
 * flancos por los rebotes mecánicos del contacto.
 */
#include "pulsador.h"
#include "config.h"

#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"

static const char *TAG = "pulsador";

static TimerHandle_t     tAntirrebote;   // Timer one-shot de antirrebote
static SemaphoreHandle_t sPulsador;      // Semáforo que se libera en cada pulsación válida

/**
 * ISR del flanco de bajada. Se mantiene mínima: solo usa funciones
 * ...FromISR y no bloquea. Está en IRAM para poder ejecutarse aunque
 * la caché de la flash esté desactivada.
 */
static void IRAM_ATTR isr_pulsador(void *arg)
{
    BaseType_t despertar = pdFALSE;
    xTimerStartFromISR(tAntirrebote, &despertar);   // Arranca o reinicia el timer
    portYIELD_FROM_ISR(despertar);                  // Cambio de contexto si hace falta
}

/**
 * Callback del timer. Se ejecuta en la tarea de servicio de timers,
 * no en la ISR, así que puede usar la API normal de FreeRTOS.
 */
static void cb_antirrebote(TimerHandle_t t)
{
    if (gpio_get_level(PIN_PULSADOR) == 0) {   // Sigue pulsado: pulsación válida
        xSemaphoreGive(sPulsador);
    }
}

void pulsador_init(SemaphoreHandle_t sem)
{
    sPulsador = sem;

    tAntirrebote = xTimerCreate("tAntirrebote",
                                pdMS_TO_TICKS(ANTIRREBOTE_MS),
                                pdFALSE,            // One-shot
                                NULL,
                                cb_antirrebote);
    configASSERT(tAntirrebote);

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_PULSADOR,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     // Sin resistencia externa
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,      // Flanco de bajada = pulsar
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_PULSADOR, isr_pulsador, NULL));

    ESP_LOGI(TAG, "Pulsador listo en GPIO%d (antirrebote %d ms)",
             PIN_PULSADOR, ANTIRREBOTE_MS);
}