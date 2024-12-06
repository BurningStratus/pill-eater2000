#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define SW_0_PIN 9
#define STRLEN  80


void btn_init() {
    gpio_init(SW_0_PIN);
    gpio_set_dir(SW_0_PIN, GPIO_IN);
    gpio_pull_up(SW_0_PIN);
}

void uart_pin_init() {
    uart_init(uart1, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    //uart_set_translate_crlf(uart1, true);
}

void uart_read_func(char *str) {
    int pos = 0;
    char c = 0;
    while (uart_is_readable_within_us(uart1, 500000)) {
        c = uart_getc(uart1) & 0xFF;
        //printf("%c", c);
        if (c == '\r' || c == '\n') {
            str[pos] = '\0';
            //pos = 0;
        } else {
            if (pos < STRLEN -1) {
                str[pos++] = c;
            }
        }
    }
}

void process_deveui(char *processd, const char *str) {
    int j = 0;
    for (int i = 13; str[i] != '\0'; i++) {
        if (str[i] != ':') {
            processd[j] = str[i];
            processd[j] = tolower(processd[j]);
            j++;
        }
    }
}




int main() {
    stdio_init_all();
    timer_hw->dbgpause = 0;

    btn_init();

    volatile uint state = 0;
    char str[STRLEN] = "";
    char processd[64] = "";

    int tries = 0;
    const uint8_t conn_test[] = "at\n";
    const uint8_t firmware_ver[] = "at+VER\n";
    const uint8_t get_dev_ui[] = "at+ID=DEVEUI\n";
    uart_pin_init();


    while (true) {
        switch (state) {
            case 0: {
                if (gpio_get(SW_0_PIN) == 0) {
                    while (!gpio_get(SW_0_PIN)) {
                        sleep_ms(50);
                    }
                    sleep_ms(50);
                    state = 1;
                    break;
                }
                state = 0;
                break;
            }
            case 1: {
                tries = 0;
                while (tries < 5) {
                    uart_write_blocking(uart1, conn_test, strlen((char *)conn_test));
                    if (uart_is_readable_within_us(uart1, 500000)) {
                        uart_read_func(str);
                        if (strcmp(str, "+AT: OK") == 0) {
                            //printf("%s\n", str);
                            strcpy(str, "");
                            state = 2;
                        }
                        break;
                    }
                    tries++;
                }
                if (tries == 5) {
                    state = 0;
                    printf("Module stopped responding\n");
                    break;
                }
                if (state == 2) {
                    printf("Connected to LoRa module\n");
                    break;
                }
            }

            case 2: {
                uart_write_blocking(uart1, firmware_ver, strlen((char *)firmware_ver));
                if (uart_is_readable_within_us(uart1, 500000)) {
                    uart_read_func(str);
                    printf("%s\n", str);
                    strcpy(str, "");
                    state = 3;
                    break;
                }
                printf("Module stopped responding\n");
                state = 0;
                break;

            }

            case 3: {
                uart_write_blocking(uart1, get_dev_ui, strlen((char *)get_dev_ui));
                if (uart_is_readable_within_us(uart1, 500000)) {
                    uart_read_func(str);
                    process_deveui(processd, str);
                    printf("%s\n", processd);
                    strcpy(str, "");
                    strcpy(processd, "");
                    state = 0;
                    break;
                }
                printf("Module Stopped Responding\n");
                state = 0;
                break;
            }
        }
    }
    return 0;
}
