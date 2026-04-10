/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;
QueueHandle_t xQueueBtn;

volatile bool blink_r = false;
volatile bool blink_y = false;

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

    while (true) {
        if (xQueueReceive(xQueueBtn, &btn, portMAX_DELAY) == pdTRUE) {
            if (btn == BTN_PIN_R) {
                blink_r = !blink_r;
                xSemaphoreGive(xSemaphoreLedR);
            } else if (btn == BTN_PIN_Y) {
                blink_y = !blink_y;
                xSemaphoreGive(xSemaphoreLedY);
            }
        }
    }
}

void led_r_task(void *p) {
    while (true) {
        xSemaphoreTake(xSemaphoreLedR, portMAX_DELAY);

        if (blink_r) {
            while (blink_r) {
                gpio_put(LED_PIN_R, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(LED_PIN_R, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            gpio_put(LED_PIN_R, 0);
        } else {
            gpio_put(LED_PIN_R, 0);
        }
    }
}

void led_y_task(void *p) {
    while (true) {
        xSemaphoreTake(xSemaphoreLedY, portMAX_DELAY);

        if (blink_y) {
            while (blink_y) {
                gpio_put(LED_PIN_Y, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(LED_PIN_Y, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            gpio_put(LED_PIN_Y, 0);
        } else {
            gpio_put(LED_PIN_Y, 0);
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

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