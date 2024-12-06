#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"


#define BUTTON_PIN 12
#define LED_PIN 20
#define OPTICAL_SENSOR_PIN 28
#define PIEZO_SENSOR_PIN 27
#define STEPPER_PIN1 2
#define STEPPER_PIN2 3
#define STEPPER_PIN3 6
#define STEPPER_PIN4 13


#define UART_NR uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STRLEN 80
#define MAX_ATTEMPTS 5
#define DEBOUNCE_DELAY 300 // Debounce time in milliseconds

// Global variables
int pill_count = 7;

// Function Prototypes
void initialize_hardware();
void calibrate_dispenser();
void rotate_stepper_one_step();
void dispense_pill();
int sendATCommand(const char *command, char *response, int maxlen, int max_attempts);
int readUARTResponse(char *response, int maxlen, int timeout_ms);

// Main Program
int main() {
    // Initialization
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

                if (sendATCommand("AT\r\n", response, STRLEN, MAX_ATTEMPTS)) {
                    printf("Connected to LoRa module. Starting pill dispensing...\n");
                    state = 1;
                } else {
                    printf("Failed to connect to LoRa module. Retrying...\n");
                }
                break;

            case 1:
                if (pill_count > 0) {
                    dispense_pill();
                    sleep_ms(30000); // Wait 30 seconds
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

void dispense_pill() {
    printf("Dispensing pill...\n");
    for (int step = 0; step < 50; step++) { // Rotate to next compartment
        rotate_stepper_one_step();
    }


    sleep_ms(500);
    if (gpio_get(PIEZO_SENSOR_PIN)) {
        pill_count--;
        printf("Pill dispensed successfully. Pills remaining: %d\n", pill_count);
    } else {
        printf("No pill detected! Blinking LED...\n");
        for (int i = 0; i < 5; i++) {
            gpio_put(LED_PIN, true);
            sleep_ms(200);
            gpio_put(LED_PIN, false);
            sleep_ms(200);
        }
    }
}

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
