#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"


#define ROT_A_PIN 10
#define ROT_B_PIN 11
#define ROT_SW_PIN 12
#define LED_PIN1 20
#define LED_PIN2 21
#define LED_PIN3 22


#define PWM_FREQ_DIVIDER 125
#define PWM_FREQ 999
#define MAX_BRIGHTNESS 1000


volatile bool led_on = true;
volatile int brightness = 505;
volatile int last_brightness = 500;
absolute_time_t last_press_time;
uint pwm_slice_num;
queue_t queue;

void set_led_brightness(uint16_t level) {
    pwm_set_gpio_level(LED_PIN1,  level);
    pwm_set_gpio_level(LED_PIN2, level);
    pwm_set_gpio_level(LED_PIN3, level);
}

void gpio_callback(uint gpio, uint32_t events) {
    queue_try_add(&queue, &gpio);
}

int main() {
    // Initialize stdio for debugging
    stdio_init_all();
    queue_init(&queue, sizeof(int), 10);

    gpio_init(LED_PIN1);
    gpio_init(LED_PIN2);
    gpio_init(LED_PIN3);
    gpio_set_dir(LED_PIN1, GPIO_OUT);
    gpio_set_dir(LED_PIN2, GPIO_OUT);
    gpio_set_dir(LED_PIN3, GPIO_OUT);

    // Initialize GPIO pins
    gpio_init(ROT_A_PIN);
    //gpio_set_dir(ROT_A_PIN, GPIO_IN);
    //gpio_pull_down(ROT_A_PIN);

    gpio_init(ROT_B_PIN);
    //gpio_set_dir(ROT_B_PIN, GPIO_IN);
    //gpio_pull_down(ROT_B_PIN);

    gpio_init(ROT_SW_PIN);
    //gpio_set_dir(ROT_SW_PIN, GPIO_IN);
    gpio_pull_up(ROT_SW_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN1),125);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN2),125);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN3),125);
    uint pwm_slice_num1 = pwm_gpio_to_slice_num(LED_PIN1);
    uint pwm_slice_num2 = pwm_gpio_to_slice_num(LED_PIN2);
    uint pwm_slice_num3 = pwm_gpio_to_slice_num(LED_PIN3);
    pwm_init(pwm_slice_num1, &config, false);
    pwm_init(pwm_slice_num2, &config, false);
    pwm_init(pwm_slice_num3, &config,false);


    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN1), 999);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN2), 999);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN3), 999);
   // pwm_set_wrap(0, MAX_BRIGHTNESS);
   // pwm_set_clkdiv(0, PWM_FREQ_DIVIDER);

    gpio_set_function(LED_PIN1, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN2, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN3, GPIO_FUNC_PWM);

    uint pwm_chan1 = pwm_gpio_to_channel(LED_PIN1);
    uint pwm_chan2 = pwm_gpio_to_channel(LED_PIN2);
    uint pwm_chan3 = pwm_gpio_to_channel(LED_PIN3);


    pwm_set_gpio_level(LED_PIN1, 0);
    pwm_set_gpio_level(LED_PIN2, 0);
    pwm_set_gpio_level(LED_PIN3, 0);
    pwm_set_enabled(pwm_slice_num1, true);
    pwm_set_enabled(pwm_slice_num2, true);
    pwm_set_enabled(pwm_slice_num3, true);

    // Set initial brightness
    set_led_brightness(brightness);
    gpio_set_irq_enabled_with_callback(ROT_A_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(ROT_B_PIN, GPIO_IRQ_EDGE_RISE, true, gpio_callback);
    gpio_set_irq_enabled_with_callback(ROT_SW_PIN, GPIO_IRQ_LEVEL_LOW, true, gpio_callback);

    uint gpio = 0;
    absolute_time_t now = get_absolute_time();

    // Main loop
    while (true) {
        while (queue_try_remove(&queue, &gpio)) {
            if (gpio == ROT_A_PIN) {
                bool b_state = gpio_get(ROT_B_PIN);

                if (led_on) {
                    if (!b_state) {
                        if (brightness < MAX_BRIGHTNESS) brightness += 50;
                        set_led_brightness(brightness);
                    }
                }
            } else if (gpio == ROT_B_PIN) {
                bool b_state = gpio_get(ROT_A_PIN);

                if (led_on) {
                    if (!b_state) {
                        if (brightness - 50 >= 0) {
                            brightness -= 50;
                        } else {
                            brightness = 0;
                        }
                        set_led_brightness(brightness);
                    }
                }
            } else if (gpio == ROT_SW_PIN) {
                now = get_absolute_time();
                if (absolute_time_diff_us(last_press_time, now) > 250000) {
                    last_press_time = now;

                    // Toggle LED state
                    if (led_on) {
                        if (brightness == 0) {
                            brightness = 500;
                        } else {
                            last_brightness = brightness;
                            brightness = 0;
                            led_on = false;
                        }
                    } else {
                        brightness = last_brightness;
                        led_on = true;
                    }
                    set_led_brightness(brightness);
                }
            }
        }

        //tight_loop_contents();
        //printf("brt: %d\n", brightness);
    }

    return 0;
}