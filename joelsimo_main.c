
/*
 * All macros and preprocessor directives were
 * moved to libcommon.h
 *
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/util/queue.h"

// custom libraries
#include "libcommon.h"
#include "libuart.h"
#include "seterr.h"

// Global variables
int pill_count = 7;
static queue_t events;

// Function Prototypes
void initialize_hardware();
void calibrate_dispenser();
void rotate_stepper_one_step();
void dispense_pill();
static void piezo_handler();

// moved to libuart.h
//int sendATCommand(const char *command, char *response, int maxlen, int max_attempts);
//int readUARTResponse(char *response, int maxlen, int timeout_ms);

// Main Program
int main() {
    // Initialization
    queue_init(&events, sizeof(int), 25);
    initialize_hardware();



    int state = 0;
    char response[STRLEN];

    while (true) {
        switch (state) {
            case 0:
                printf("Waiting for button press to start calibration...\n");
                gpio_put(LED_PIN, true); // LED on
                while (gpio_get(BUTTON_PIN)) {
                    sleep_ms(10);
                }
                sleep_ms(DEBOUNCE_DELAY);
                gpio_put(LED_PIN, false); // LED off
                printf("Button pressed. Starting calibration...\n");
                calibrate_dispenser();

                // subsituted. OG func is in the end of this file.
                //if ( !sendATCommand("AT\r\n", response, STRLEN, MAX_ATTEMPTS)) {
                if ( ! uart_cmd ("AT\r\n", response ) ) {
                    printf("Connected to LoRa module. Starting pill dispensing...\n");
                    state = 1;
                } else {
                    printf("Failed to connect to LoRa module. Retrying...\n");
                }
                break;

            case 1:
                if (pill_count > 0) {
                    dispense_pill();
                    sleep_ms(10000); // Wait 30 seconds, (lowered to 10s for easier debug)
                } else {
                    printf("All pills dispensed. Restarting calibration...\n");
                    state = 0;
                }
                break;

            default:
                state = 0;
                break;
        }
    }
}

// Function Definitions

void initialize_hardware() {
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(OPTICAL_SENSOR_PIN);
    gpio_set_dir(OPTICAL_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(OPTICAL_SENSOR_PIN); // Enable pull-up for optical sensor

    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, true, piezo_handler);
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, false);


    gpio_init(STEPPER_PIN1);
    gpio_init(STEPPER_PIN2);
    gpio_init(STEPPER_PIN3);
    gpio_init(STEPPER_PIN4);
    gpio_set_dir(STEPPER_PIN1, GPIO_OUT);
    gpio_set_dir(STEPPER_PIN2, GPIO_OUT);
    gpio_set_dir(STEPPER_PIN3, GPIO_OUT);
    gpio_set_dir(STEPPER_PIN4, GPIO_OUT);

    uart_init(UART_NR, BAUD_RATE);
    uart_set_format(UART_NR, 8, 1, UART_PARITY_NONE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void calibrate_dispenser() {
    printf("Calibrating dispenser...\n");
    while (gpio_get(OPTICAL_SENSOR_PIN)) {
        rotate_stepper_one_step();
    }
    printf("Calibration complete. Optical sensor aligned.\n");
}

void rotate_stepper_one_step() {
    static const bool step_sequence[4][4] = {
        {true, false, false, false},
        {false, true, false, false},
        {false, false, true, false},
        {false, false, false, true}
    };

    for (int i = 0; i < 4; i++) {
        gpio_put(STEPPER_PIN1, step_sequence[i][0]);
        gpio_put(STEPPER_PIN2, step_sequence[i][1]);
        gpio_put(STEPPER_PIN3, step_sequence[i][2]);
        gpio_put(STEPPER_PIN4, step_sequence[i][3]);
        sleep_ms(10);
    }
}

static void piezo_handler(const uint gpio, uint32_t event_mask) {
    const int sensor_impact = 1;
    queue_try_add(&events, &sensor_impact);
}

void dispense_pill() {
    int piezo_val = 0;
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, true);
    printf("Dispensing pill...\n");
    for (int step = 0; step < 70; step++) { // Rotate to next compartment
        rotate_stepper_one_step();
    }


    sleep_ms(500);
    if (queue_try_remove(&events, &piezo_val)) {
        if (piezo_val == 1) {
            pill_count--;
            printf("Pill dispensed successfully. Pills remaining: %d\n", pill_count);
            piezo_val = 0;
        }
        //queue burn
        while (queue_try_remove(&events, &piezo_val)) {
            piezo_val = 0;
        }
    } else {
        printf("No pill detected! Blinking LED...\n");
        for (int i = 0; i < 5; i++) {
            gpio_put(LED_PIN, true);
            sleep_ms(200);
            gpio_put(LED_PIN, false);
            sleep_ms(200);
        }
    }
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, false);
}

/* *moved to libuart.h*
int readUARTResponse(char *response, int maxlen, int timeout_ms) {
    int response_len = 0;
    uint64_t start_time = time_us_64();

    while (time_us_64() - start_time < timeout_ms * 1000) {
        if (uart_is_readable(UART_NR)) {
            response[response_len++] = uart_getc(UART_NR) & 0xFF; // Ensure 8-bit data
            if (response_len >= maxlen - 1) break;               // Avoid buffer overflow
        }
    }

    if (response_len > 0) {
        response[response_len] = '\0'; // Null-terminate the response
        return 1;                      // Success
    }
    return 0; // Timeout or no data received
}

int sendATCommand(const char *command, char *response, int maxlen, int max_attempts) {
    int attempt = 0;

    while (attempt < max_attempts) {
        uart_write_blocking(UART_NR, (const uint8_t *)command, strlen(command));
        printf("Sent command: %s\n", command);
        if (readUARTResponse(response, maxlen, 500)) { // 500 ms timeout
            printf("Received response: %s\n", response);
            return 1; // Success
        } else {
            printf("No response received (attempt %d)\n", attempt + 1);
        }
        attempt++;
    }
    return 0; // Failure
}
*/
