#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

typedef enum {
    CMD_TOGGLE = 1
} led_cmd_t;

QueueHandle_t xQueueBtn;
QueueHandle_t xQueueLedR;
QueueHandle_t xQueueLedY;

void btn_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        int btn_id = -1;

        if (gpio == BTN_PIN_R) {
            btn_id = BTN_PIN_R;
        } else if (gpio == BTN_PIN_Y) {
            btn_id = BTN_PIN_Y;
        }

        if (btn_id != -1) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(xQueueBtn, &btn_id, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void btn_task(void *p) {
    int btn = 0;
    led_cmd_t cmd = CMD_TOGGLE;

    while (true) {
        if (xQueueReceive(xQueueBtn, &btn, portMAX_DELAY) == pdTRUE) {
            if (btn == BTN_PIN_R) {
                xQueueSend(xQueueLedR, &cmd, portMAX_DELAY);
            } else if (btn == BTN_PIN_Y) {
                xQueueSend(xQueueLedY, &cmd, portMAX_DELAY);
            }
        }
    }
}

void led_r_task(void *p) {
    led_cmd_t cmd;
    bool blinking = false;

    while (true) {
        if (xQueueReceive(xQueueLedR, &cmd, 0) == pdTRUE) {
            if (cmd == CMD_TOGGLE) {
                blinking = !blinking;
                if (!blinking) {
                    gpio_put(LED_PIN_R, 0);
                }
            }
        }

        if (blinking) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void led_y_task(void *p) {
    led_cmd_t cmd;
    bool blinking = false;

    while (true) {
        if (xQueueReceive(xQueueLedY, &cmd, 0) == pdTRUE) {
            if (cmd == CMD_TOGGLE) {
                blinking = !blinking;
                if (!blinking) {
                    gpio_put(LED_PIN_Y, 0);
                }
            }
        }

        if (blinking) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);

    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_put(LED_PIN_Y, 0);

    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xQueueLedR = xQueueCreate(8, sizeof(led_cmd_t));
    xQueueLedY = xQueueCreate(8, sizeof(led_cmd_t));

    xTaskCreate(btn_task, "BTN_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_R_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y_Task", 256, NULL, 1, NULL);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    vTaskStartScheduler();

    while (1) {
    }

    return 0;
}