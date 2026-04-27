/**
 * ---------------------------------------------------------------------------------------
 * PROJETO: Atividade 07 - Comunicação Serial UART com Loopback Físico
 * OBJETIVO: Demonstrar o envio e recebimento de dados via UART2 do ESP32.
 * FUNCIONAMENTO: O ESP32 envia "LIGAR" ou "DESLIGAR" pelo pino TX2 (17). 
 * Se houver um jumper conectando o TX2 ao RX2 (16), o dado retorna, 
 * é processado e liga/desliga o LED interno (GPIO 2).
 * DESENVOLVEDOR: Andreza Santos e Nivaldo Neto (Ambiente ESP-IDF v6.0)
 * ---------------------------------------------------------------------------------------
 */

#include <stdio.h>              // Biblioteca padrão de entrada e saída (para o printf)
#include <string.h>             // Biblioteca para manipulação de strings (strlen, strcmp)
#include "freertos/FreeRTOS.h"  // Núcleo do sistema operacional em tempo real
#include "freertos/task.h"      // Gerenciamento de tarefas e delays (vTaskDelay)
#include "driver/uart.h"        // Driver oficial para controle das portas UART
#include "driver/gpio.h"        // Driver para controle dos pinos de entrada e saída

// Definições de constantes para facilitar a manutenção do código
#define LED_PIN 2               // Pino do LED onboard (azul)
#define UART_NUM UART_NUM_2     // Usaremos a segunda porta serial do ESP32
#define TXD_PIN 17              // Pino físico que envia os dados (Transmitter)
#define RXD_PIN 16              // Pino físico que recebe os dados (Receiver)
#define BUF_SIZE 1024           // Tamanho do reservatório (buffer) de dados recebidos

void app_main(void) {
    // --- Configuração do LED ---
    gpio_reset_pin(LED_PIN);                         // Limpa qualquer configuração anterior do pino
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);    // Define o pino do LED como saída

    // --- Configuração Estrutural da UART2 ---
    uart_config_t uart_config = {
        .baud_rate = 115200,                         // Velocidade de transmissão (115.2k bits/s)
        .data_bits = UART_DATA_8_BITS,               // Cada caractere tem 8 bits
        .parity    = UART_PARITY_DISABLE,            // Sem bit de conferência (paridade)
        .stop_bits = UART_STOP_BITS_1,               // 1 bit de parada ao final de cada dado
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,       // Sem controle de fluxo por hardware
        .source_clk = UART_SCLK_DEFAULT,             // Usa o clock padrão do sistema
    };
    
    // Aplica as configurações acima na porta UART2
    uart_param_config(UART_NUM, &uart_config);

    // Define quais pinos físicos do ESP32 serão TX2 e RX2
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Instala o driver na memória, reservando espaço para os dados que chegam
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Aloca um espaço na memória RAM para guardar o que for lido pela UART
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    bool alternar = false;      // Variável de controle para alternar entre ligar e desligar

    // --- Loop Principal (Infinito) ---
    while (1) {
        // Define a mensagem baseada no estado da variável 'alternar'
        const char* msg = alternar ? "LIGAR" : "DESLIGAR";
        
        // 1. Envia a string (texto) pela porta UART2
        uart_write_bytes(UART_NUM, msg, strlen(msg));
        printf("\n--- Ciclo UART ---\n");
        printf("Enviado para TX2: %s\n", msg);

        // 2. Tenta ler o que voltou pelo RX2 (Loopback)
        // O sistema espera por até 100ms se o dado ainda não tiver chegado
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));
        
        if (len > 0) { // Se recebeu algum dado (len > 0)
            data[len] = '\0'; // Insere o caractere nulo para indicar o fim da string
            printf("Recebido no RX2: %s\n", (char*)data);

            // Compara o texto recebido para decidir o que fazer com o LED
            if (strcmp((char*)data, "LIGAR") == 0) {
                gpio_set_level(LED_PIN, 1);          // Liga o LED
                printf("Ação: LED ACESO\n");
            } else if (strcmp((char*)data, "DESLIGAR") == 0) {
                gpio_set_level(LED_PIN, 0);          // Desliga o LED
                printf("Ação: LED APAGADO\n");
            }
        } else {
            // Caso o jumper esteja desconectado, o código cairá aqui
            printf("Erro: Nada recebido. Verifique o jumper entre 17 e 16!\n");
        }

        alternar = !alternar;                        // Inverte o estado para a próxima repetição
        vTaskDelay(pdMS_TO_TICKS(2000));             // Pausa o programa por 2 segundos (2000ms)
    }
}