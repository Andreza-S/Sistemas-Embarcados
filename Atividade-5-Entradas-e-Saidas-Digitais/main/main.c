#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Definições de Pinos conforme seu esquemático
#define LED_PIN 2   // GP2
#define BTN_PIN 10  // GP10

// Configurações de tempo
#define DEBOUNCE_TIME_US 500000      // 500 milissegundos
#define TIMEOUT_LIMIT_US 10000000   // 10 segundos (em microssegundos)

void app_main(void) {
    // 1. Configuração do LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // 2. Configuração do Botão (Entrada)
    // Usamos pull_up_en = DISABLE pois colocamos o resistor de 10k no esquemático
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_DISABLE); 

    int led_state = 0;            // 0 = desligado, 1 = ligado
    int64_t last_button_time = 0; // Para o debounce
    int64_t led_start_time = 0;   // Para os 10 segundos

    printf("Sistema Iniciado! Aguardando comando no GP10...\n");

    while (1) {
        int64_t now = esp_timer_get_time(); // Pega o tempo atual do processador

        // --- LÓGICA DO BOTÃO (POLLING) ---
        if (gpio_get_level(BTN_PIN) == 0) { // Botão pressionado (GND)
            if (now - last_button_time > DEBOUNCE_TIME_US) {
                
                led_state = !led_state; // Toggle: Inverte o estado
                gpio_set_level(LED_PIN, led_state);

                if (led_state) {
                    led_start_time = now; // Salva o momento exato que ligou
                    printf("LED ACESO! Desligamento automatico em 10s.\n");
                } else {
                    printf("LED APAGADO via botao.\n");
                }

                last_button_time = now; // Atualiza tempo do debounce
            }
        }

        // --- LÓGICA DO TEMPORIZADOR (MÁQUINA DE ESTADOS) ---
        if (led_state == 1) { // Se o LED estiver ligado...
            // Verifica se a diferença entre agora e o início é maior que 10s
            if (now - led_start_time > TIMEOUT_LIMIT_US) {
                led_state = 0;
                gpio_set_level(LED_PIN, led_state);
                printf("TEMPO ESGOTADO: LED desligado automaticamente.\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}