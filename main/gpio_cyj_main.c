#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT 18
#define GPIO_INPUT 4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT) 
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT) 
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            gpio_set_level(GPIO_OUTPUT, 1);
        }
    }
}   


void app_main(void)
{
    gpio_config_t gpio;
    gpio.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    gpio.mode = GPIO_MODE_OUTPUT;
    gpio.pull_up_en = 0;
    gpio.pull_down_en = 0;
    gpio.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&gpio);

    gpio.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    gpio.mode = GPIO_MODE_INPUT;
    gpio.pull_up_en = 1;
    gpio.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&gpio);

       //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT, GPIO_INTR_POSEDGE); //rising

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, (void*) GPIO_INPUT);

    // //remove isr handler for gpio number.
    // gpio_isr_handler_remove(GPIO_INPUT);
    // //hook isr handler for specific gpio pin again
    // gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, (void*) GPIO_INPUT);

    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT, 0);
    }
}