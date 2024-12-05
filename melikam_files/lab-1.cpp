#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h" // Include this header for clock functions

#define LED_PIN_1 20
#define LED_PIN_2 21
#define LED_PIN_3 22
#define BUTTON_SW0 9
#define BUTTON_SW1 8
#define BUTTON_SW2 7

#define PWM_FREQ 1000 // 1 kHz
#define PWM_RESOLUTION 1000// 12-bit resolution for PWM

bool led_state = false;      // LED state (ON/OFF)
uint16_t brightness = 500;  // Default brightness (50%)
bool button_sw1_pressed = false;
bool button_sw0_pressed = false;
bool button_sw2_pressed = false;

void setup() {
    stdio_init_all();

    // Initialize LED GPIO pins
    gpio_set_function(LED_PIN_1, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_2, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_3, GPIO_FUNC_PWM);

    // Initialize button GPIO pins
    gpio_init(BUTTON_SW1);
    gpio_set_dir(BUTTON_SW1, GPIO_IN);
    gpio_pull_up(BUTTON_SW1);

    gpio_init(BUTTON_SW0);
    gpio_set_dir(BUTTON_SW0, GPIO_IN);
    gpio_pull_up(BUTTON_SW0);

    gpio_init(BUTTON_SW2);
    gpio_set_dir(BUTTON_SW2, GPIO_IN);
    gpio_pull_up(BUTTON_SW2);

    // Set PWM frequency
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN_1), (float)clock_get_hz(clk_sys) / (PWM_FREQ * PWM_RESOLUTION));
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN_2), (float)clock_get_hz(clk_sys) / (PWM_FREQ * PWM_RESOLUTION));
    pwm_set_clkdiv(pwm_gpio_to_slice_num(LED_PIN_3), (float)clock_get_hz(clk_sys) / (PWM_FREQ * PWM_RESOLUTION));

    // Set initial PWM wrap value
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN_1), PWM_RESOLUTION - 1);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN_2), PWM_RESOLUTION - 1);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_PIN_3), PWM_RESOLUTION - 1);

    // Enable PWM on the LED pins
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_PIN_1), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_PIN_2), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_PIN_3), true);
}

void toggle_leds() {
    if(led_state && brightness == 0){
        brightness = 500;
        pwm_set_gpio_level(LED_PIN_1, brightness);
        pwm_set_gpio_level(LED_PIN_2, brightness);
        pwm_set_gpio_level(LED_PIN_3, brightness);
    }
    else if (led_state) {
        // Turn LEDs off
        pwm_set_gpio_level(LED_PIN_1, 0);
        pwm_set_gpio_level(LED_PIN_2, 0);
        pwm_set_gpio_level(LED_PIN_3, 0);
        led_state = !led_state;
    } else {
        // Turn LEDs on
        pwm_set_gpio_level(LED_PIN_1, brightness);
        pwm_set_gpio_level(LED_PIN_2, brightness);
        pwm_set_gpio_level(LED_PIN_3, brightness);
        led_state = !led_state;
    }

}

void increase_brightness() {
    if (led_state && brightness < PWM_RESOLUTION) {
        brightness += 100;
        pwm_set_gpio_level(LED_PIN_1, brightness);
        pwm_set_gpio_level(LED_PIN_2, brightness);
        pwm_set_gpio_level(LED_PIN_3, brightness);
    }
}


void decrease_brightness() {
    if (led_state) {
        if (brightness >= 100) {
            brightness -= 100;
            pwm_set_gpio_level(LED_PIN_1, brightness);
            pwm_set_gpio_level(LED_PIN_2, brightness);
            pwm_set_gpio_level(LED_PIN_3, brightness);
        }

    }
}
int main() {
    setup();
    while (true) {
        // Check SW1 for toggling LEDs
        if (gpio_get(BUTTON_SW1) == 0 && !button_sw1_pressed) { // Button pressed
            button_sw1_pressed = true;
            toggle_leds();
        } else if (gpio_get(BUTTON_SW1) == 1) { // Button released
            button_sw1_pressed = false;
        }

        // Check SW0 for increasing brightness
        if (gpio_get(BUTTON_SW0) == 0 && !button_sw0_pressed) {
            button_sw0_pressed = true;
            increase_brightness();
        } else if (gpio_get(BUTTON_SW0) == 1) {
            button_sw0_pressed = false;
        }

        // Check SW2 for decreasing brightness
        if (gpio_get(BUTTON_SW2) == 0 && !button_sw2_pressed) {
            button_sw2_pressed = true;
            decrease_brightness();
        } else if (gpio_get(BUTTON_SW2) == 1) {
            button_sw2_pressed = false;
        }

        sleep_ms(50); // Add a small delay to debounce the buttons
    }
    return 0;
}