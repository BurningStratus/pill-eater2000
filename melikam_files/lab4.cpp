#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define STRLEN 80           // Maximum length for UART response
#define UART_NR uart1       // Using UART1
#define UART_TX_PIN 4       // UART TX pin
#define UART_RX_PIN 5       // UART RX pin
#define BAUD_RATE 9600      // UART communication speed (9600 baud)
#define BUTTON_PIN 12
#define MAX_ATTEMPTS 5      // Maximum number of attempts to retry commands
#define DEBOUNCE_DELAY 300  // Debounce delay in milliseconds

// Function to read a response from UART with a timeout
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
        return 1;                      // Success: data received
    }
    return 0; // Timeout or no data received
}

// Function to send an AT command to the LoRa module and wait for a response
int sendATCommand(const char *command, char *response, int maxlen, int max_attempts) {
    int attempt = 0;

    while (attempt < max_attempts) {
        uart_write_blocking(UART_NR, (const uint8_t *)command, strlen(command)); // Send command
        printf("Sent command: %s\n", command);
        if (readUARTResponse(response, maxlen, 500)) {  // 500 ms timeout
            printf("Received response: %s\n", response);
            return 1; // Success
        } else {
            printf("No response received (attempt %d)\n", attempt + 1);
        }
        attempt++;
    }
    return 0; // Failure
}

// Function to process and format DevEui
void processDevEui(const char *devEui) {
    char processedDevEui[STRLEN] = {0};
    int j = 0;

    for (int i = 0; devEui[i] != '\0'; ++i) {
        if (devEui[i] != ':') {
            processedDevEui[j++] = tolower(devEui[i]);
        }
    }
    processedDevEui[j] = '\0';
    printf("Processed DevEui: %s\n", processedDevEui);
}

// Main function
int main() {
    int state = 0;

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    stdio_init_all();
    uart_init(UART_NR, BAUD_RATE);
    uart_set_format(UART_NR, 8, 1, UART_PARITY_NONE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("Boot\n");

    char str[STRLEN];

    while (true) {
        switch (state) {
            case 0:
                printf("Waiting for button press...\n");
                while (gpio_get(BUTTON_PIN)) {
                    sleep_ms(10);
                }
                sleep_ms(DEBOUNCE_DELAY);
                printf("Button pressed. Starting communication...\n");
                state = 1;
                break;

            case 1:
                if (sendATCommand("AT\r\n", str, STRLEN, MAX_ATTEMPTS)) {
                    printf("Connected to LoRa module\n");
                    state = 2;
                } else {
                    printf("Module not responding\n");
                    state = 0;
                }
                break;

            case 2:
                if (sendATCommand("AT+VER\r\n", str, STRLEN, MAX_ATTEMPTS)) {
                    printf("Firmware Version: %s\n", str);
                    state = 3;
                } else {
                    printf("Module stopped responding\n");
                    state = 0;
                }
                break;

            case 3:
                if (sendATCommand("AT+ID=DEVEUI\r\n", str, STRLEN, MAX_ATTEMPTS)) {
                    processDevEui(str);
                    state = 0;
                } else {
                    printf("Module stopped responding\n");
                    state = 0;
                }
                break;

            default:
                state = 0;
                break;
        }
    }
}