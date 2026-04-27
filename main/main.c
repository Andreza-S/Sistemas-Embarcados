/* * ENUNCIADO DO PROJETO:
 * Controlador de Iluminação Inteligente com ESP32 (ESP-IDF).
 * * OBJETIVO: 
 * Implementar um sistema de acionamento de LED via botão com as seguintes regras:
 * 1. Clique Curto: Liga o LED por 10 segundos. Se o LED já estiver ligado, 
 * reinicia (reset) a contagem de 10 segundos.
 * 2. Desligamento Forçado: Se o botão for segurado por mais de 2 segundos, 
 * o LED apaga imediatamente.
 * 3. Modo Ciclo: Se o botão continuar pressionado após o desligamento forçado, 
 * o LED alternará seu estado (ligar/desligar) a cada 2 segundos.
 * 4. Persistência: Se solto enquanto apagado após um ciclo, permanece apagado.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"  // Biblioteca base do sistema operacional de tempo real
#include "freertos/task.h"      // Gerenciamento de tarefas (vTaskDelay)
#include "driver/gpio.h"        // Driver para controle de pinos de entrada e saída
#include "esp_timer.h"          // Driver para medição de tempo em microssegundos

// Definição física dos pinos
#define LED_PIN 2               // Pino onde o LED está conectado
#define BUTTON_PIN 10           // Pino onde o botão está conectado

void app_main(void) {
    // Inicialização dos periféricos
    gpio_reset_pin(LED_PIN);                          // Garante que o pino do LED esteja limpo
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);    // Configura pino do LED como saída
    
    gpio_reset_pin(BUTTON_PIN);                       // Garante que o pino do botão esteja limpo
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);  // Configura pino do botão como entrada
    gpio_pullup_en(BUTTON_PIN);                       // Ativa resistor interno (Padrão: 1, Pressionado: 0)

    // Variáveis de controle de estado e tempo
    bool led_state = false;             // Armazena se o LED está ligado ou desligado
    int64_t press_start_time = 0;       // Armazena o timestamp de quando o botão foi apertado
    bool timer_10s_active = false;      // Flag que indica se o temporizador automático está rodando
    int64_t timer_10s_deadline = 0;     // Armazena o momento exato (futuro) em que o LED deve apagar
    bool cycling_mode = false;          // Indica se o usuário está no modo "segurar por muito tempo"

    printf("Sistema Final Iniciado.\n");

    while (1) {
        // Verifica o estado do botão (0 = pressionado, 1 = solto devido ao Pull-up)
        int btn_pressed = (gpio_get_level(BUTTON_PIN) == 0);
        int64_t now = esp_timer_get_time(); // Obtém o tempo atual do sistema em microssegundos

        if (btn_pressed) {
            // Se for o início de um novo pressionamento
            if (press_start_time == 0) {
                press_start_time = now; // Salva o instante inicial
                
                // Se o LED estava apagado, liga agora
                if (!led_state) {
                    led_state = true;
                    gpio_set_level(LED_PIN, 1);
                    timer_10s_deadline = now + (10 * 1000000); // Define desligamento para daqui a 10s
                    timer_10s_active = true;
                    printf("LED ligado! Timer de 10s iniciado.\n");
                }
                cycling_mode = false; // Começa como um clique normal, não ciclo ainda
            }

            // Calcula há quanto tempo o botão está sendo segurado (convertendo para milissegundos)
            int64_t duration = (now - press_start_time) / 1000;

            // Se o botão for segurado por 2 segundos ou mais
            if (duration >= 2000) {
                led_state = !led_state;           // Inverte o estado atual do LED
                gpio_set_level(LED_PIN, led_state);
                
                // Se o LED religou no ciclo, reinicia o timer de 10s; se desligou, desativa o timer
                timer_10s_active = led_state; 
                if (led_state) {
                    timer_10s_deadline = now + (10 * 1000000);
                    printf("Ciclo: LED RELIGADO (Timer resetado para 10s).\n");
                } else {
                    printf("Ciclo: LED DESLIGADO forçado.\n");
                }

                press_start_time = now; // Reseta a contagem de pressão para permitir o próximo ciclo de 2s
                cycling_mode = true;    // Marca que entramos no modo de repetição por pressão longa
            }
        } 
        else {
            // Se o botão foi solto agora
            if (press_start_time > 0) {
                int64_t duration = (now - press_start_time) / 1000;

                // Se foi um clique rápido (menos de 2s) e não era modo ciclo
                if (duration < 2000 && !cycling_mode) {
                    if (led_state) {
                        // REQUISITO: Resetar o contador de 10s se o LED já estava aceso
                        timer_10s_deadline = now + (10 * 1000000);
                        timer_10s_active = true;
                        printf("Clique curto detectado: Contador de 10s RESETADO.\n");
                    }
                }
                press_start_time = 0;   // Reseta a variável de controle de pressão para o próximo clique
                cycling_mode = false;   // Sai do modo ciclo
            }
        }

        // LÓGICA DO TEMPORIZADOR AUTOMÁTICO (Desligar após 10s)
        if (timer_10s_active && now >= timer_10s_deadline) {
            if (led_state) {
                led_state = false;
                gpio_set_level(LED_PIN, 0); // Apaga o LED fisicamente
                printf("Timer esgotado. LED apagado automaticamente.\n");
            }
            timer_10s_active = false; // Desativa o monitoramento do timer
        }

        // Pequena pausa para evitar alto consumo de CPU e filtrar ruidos (debounce)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}