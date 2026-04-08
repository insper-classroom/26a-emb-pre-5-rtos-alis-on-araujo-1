#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;

const int LED_PIN_R = 4;
const int LED_PIN_G = 6;

QueueHandle_t xQueueButId;
QueueHandle_t xQueueBtn2;

// Variáveis globais para armazenar o estado dos delays (entre a detecção e o debounce)
static int btn1_delay = 0;
static int btn2_delay = 0;

// Callback de interrupção para o botão 1
void gpio_callback_btn1(uint gpio, uint32_t events) {
    // Verifica se é uma queda (GPIO passou para 0 = botão pressionado)
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Aguarda o debounce (simples)
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // Verifica se ainda está pressionado
        if (!gpio_get(BTN_PIN_R)) {
            if (btn1_delay < 1000) {
                btn1_delay += 100;
            } else {
                btn1_delay = 100;
            }
            printf("delay btn1: %d \n", btn1_delay);
            xQueueSend(xQueueButId, &btn1_delay, portMAX_DELAY);
        }
    }
}

// Callback de interrupção para o botão 2
void gpio_callback_btn2(uint gpio, uint32_t events) {
    // Verifica se é uma queda (GPIO passou para 0 = botão pressionado)
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Aguarda o debounce (simples)
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // Verifica se ainda está pressionado
        if (!gpio_get(BTN_PIN_G)) {
            if (btn2_delay < 1000) {
                btn2_delay += 100;
            } else {
                btn2_delay = 100;
            }
            printf("delay btn2: %d \n", btn2_delay);
            xQueueSend(xQueueBtn2, &btn2_delay, portMAX_DELAY);
        }
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 0;
    while (true) {
        if (xQueueReceive(xQueueButId, &delay, 0)) {
            printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    // Fica esperando indefinidamente - as interrupções fazem o trabalho
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);

    int delay = 0;
    while (true) {
        if (xQueueReceive(xQueueBtn2, &delay, 0)) {
            printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_G, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_G, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void btn_2_task(void *p) {
    gpio_init(BTN_PIN_G);
    gpio_set_dir(BTN_PIN_G, GPIO_IN);
    gpio_pull_up(BTN_PIN_G);

    // Fica esperando indefinidamente - as interrupções fazem o trabalho
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS \n");

    xQueueButId = xQueueCreate(32, sizeof(int));
    xQueueBtn2 = xQueueCreate(32, sizeof(int));

    // Configura os callbacks de interrupção para os botões
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_btn1);
    gpio_set_irq_enabled_with_callback(BTN_PIN_G, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_btn2);

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
