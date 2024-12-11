#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// GPIO pin definitions for LEDs and switches
#define LED0 20
#define LED1 21
#define LED2 22
#define SW0  9
#define SW1  8
#define SW2  7

// I2C configuration
#define I2C_PORT i2c0
#define SDA_PIN 16
#define SCL_PIN 17
#define EEPROM_ADDR 0x50 // Default I2C address of EEPROM
#define EEPROM_SIZE 2048
#define PAGE_MAX 64

// Log configuration
#define LOG_ENTRY_SIZE 64
#define LOG_MAX_ENTRIES (EEPROM_SIZE / LOG_ENTRY_SIZE)
#define LOG_STRING_SIZE (LOG_ENTRY_SIZE - 3) // 61 characters + null terminator + 2-byte CRC

#define COMMAND_BUFFER_SIZE 32

void erase_log();

// Structure to store LED states
typedef struct {
    uint8_t state;
    uint8_t not_state;
} ledstate;

// CRC function
uint16_t crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    while (length--) {
        uint8_t x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }
    return crc;
}

// Helper to set LED state in the structure
void set_led_state(ledstate *ls, uint8_t value) {
    ls->state = value;
    ls->not_state = ~value;
}

// Validate LED state
bool led_state_is_valid(const ledstate *ls) {
    return ls->state == (uint8_t)~ls->not_state;
}

// EEPROM write function
void eeprom_write(uint16_t addr, const void *data, size_t length) {
    if (length > PAGE_MAX) {
        length = PAGE_MAX;
    }
    uint8_t buffer[length + 2];
    buffer[0] = (uint8_t)(addr >> 8);
    buffer[1] = (uint8_t)addr;
    memcpy(&buffer[2], data, length);
    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buffer, length + 2, false);
    sleep_ms(5);
}

// EEPROM read function
void eeprom_read(uint16_t addr, void *data, size_t length) {
    uint8_t mem_addr[2] = {(uint8_t)(addr >> 8), (uint8_t)addr};
    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, mem_addr, 2, true);
    i2c_read_blocking(I2C_PORT, EEPROM_ADDR, data, length, false);
}

// Read LED state from EEPROM
bool read_led_state(ledstate *ls) {
    eeprom_read(EEPROM_SIZE - sizeof(ledstate), ls, sizeof(ledstate));
    return led_state_is_valid(ls);
}

// Write LED state to EEPROM
void write_led_state(const ledstate *ls) {
    eeprom_write(EEPROM_SIZE - sizeof(ledstate), ls, sizeof(ledstate));
}
int get_log_index() {
    uint8_t buffer[LOG_ENTRY_SIZE];
    for (uint16_t i = 0; i < LOG_MAX_ENTRIES; i++) {
        eeprom_read(i * LOG_ENTRY_SIZE, buffer, LOG_ENTRY_SIZE);
        if (buffer[0] == 0) {
            return i;
        }
        size_t msg_len = strnlen((char *)buffer, LOG_STRING_SIZE + 1);
        uint16_t crc = crc16(buffer, msg_len + 1);
        uint16_t stored_crc = (buffer[msg_len + 1] << 8) | buffer[msg_len + 2];

        if (crc == stored_crc) {
        } else {
            return i;
        }
    }
}
// Write to the log
void write_log(const char *message) {
    uint16_t current_log_index = get_log_index();
    uint8_t buffer[LOG_ENTRY_SIZE] = {0};
    size_t msg_len = strlen(message);
    if (msg_len > LOG_STRING_SIZE) {
        msg_len = LOG_STRING_SIZE;
    }
    memcpy(buffer, message, msg_len);
    uint16_t crc = crc16(buffer, msg_len + 1);
    buffer[msg_len + 1] = (uint8_t)(crc >> 8);
   buffer[msg_len + 2] = (uint8_t)crc;
if (current_log_index >= LOG_MAX_ENTRIES) {
    erase_log();
    current_log_index = 0;
}
    eeprom_write(current_log_index * LOG_ENTRY_SIZE, buffer, LOG_ENTRY_SIZE);
    current_log_index = (current_log_index + 1) % LOG_MAX_ENTRIES;
}

// Read and print the log
void read_log() {
    uint8_t buffer[LOG_ENTRY_SIZE];
    for (uint16_t i = 0; i < LOG_MAX_ENTRIES; i++) {
        eeprom_read(i * LOG_ENTRY_SIZE, buffer, LOG_ENTRY_SIZE);
        if (buffer[0] == 0) {
            printf("End of log.Boot complete\n");
            break;
        }
        size_t msg_len = strnlen((char *)buffer, LOG_STRING_SIZE + 1);
        uint16_t crc = crc16(buffer, msg_len + 1);
        uint16_t stored_crc = (buffer[msg_len + 1] << 8) | buffer[msg_len + 2];

        if (crc == stored_crc) {
            printf("Log entry %d: %s\n", i, buffer);
        } else {
            printf("Log entry %d: CRC validation failed.\n", i);
            break;
        }
    }
}

// Erase the log
void erase_log() {
    uint8_t buffer[LOG_ENTRY_SIZE] = {0};
    for (uint16_t i = 0; i < LOG_MAX_ENTRIES; i++) {
        eeprom_write(i * LOG_ENTRY_SIZE, buffer, LOG_ENTRY_SIZE);
    }
    printf("Log erased successfully.\n");
}

// Toggle an LED
void toggle_led(uint8_t led_gpio, ledstate *ls, uint8_t led_bit) {
    uint8_t new_state = ls->state ^ (1 << led_bit);
    set_led_state(ls, new_state);
    gpio_put(led_gpio, (new_state >> led_bit) & 1);
    printf("LED state changed to\nLED0 : %d\nLED1 : %d\nLED2 : %d\n", gpio_get(LED0), gpio_get(LED1), gpio_get(LED2));
    write_led_state(ls);
    char log_message[64];
    snprintf(log_message, sizeof(log_message), "LED state updated: %02X", ls->state);
    write_log(log_message);
}

// Initialize LEDs
void initialize_leds() {
    gpio_init(LED0);
    gpio_init(LED1);
    gpio_init(LED2);
    gpio_set_dir(LED0, GPIO_OUT);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_set_dir(LED2, GPIO_OUT);

    gpio_init(SW0);
    gpio_init(SW1);
    gpio_init(SW2);
    gpio_set_dir(SW0, GPIO_IN);
    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_pull_up(SW0);
    gpio_pull_up(SW1);
    gpio_pull_up(SW2);
}

// Initialize I2C
void initialize_i2c() {
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

// Main program
int main() {
    stdio_init_all();

    // Initialize GPIOs for LEDs and buttons
    initialize_leds();
    initialize_i2c();

    ledstate ls;

    // Read the LED state from EEPROM or set defaults
    if (!read_led_state(&ls)) {
        printf("No valid LED state found in EEPROM, setting default state.\n");
        set_led_state(&ls, 0b010); // Default: middle LED ON
        write_led_state(&ls);
    }

    gpio_put(LED0, (ls.state >> 0) & 1);
    gpio_put(LED1, (ls.state >> 1) & 1);
    gpio_put(LED2, (ls.state >> 2) & 1);

    write_log("Boot complete");

    char command_buffer[COMMAND_BUFFER_SIZE] = {0};
    size_t buffer_index = 0;

    // Main loop
    while (true) {
        if (!gpio_get(SW0)) {
            while (!gpio_get(SW0)) {
                sleep_ms(20); // Debounce delay

            }
            toggle_led(LED0, &ls, 0);
        }
        if (!gpio_get(SW1)) {
            while (!gpio_get(SW1)) {
                sleep_ms(20); // Debounce delay
            }
            toggle_led(LED1, &ls, 1);
        }
        if (!gpio_get(SW2)) {
            while(!gpio_get(SW2)) {
                sleep_ms(20); // Debounce delay
            }
            toggle_led(LED2, &ls, 2);
        }

        // Handle user input
        int c = getchar_timeout_us(1000);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                command_buffer[buffer_index] = '\0';
                if (c == '\r') {
                    getchar();
                }

                if (strcmp(command_buffer, "read") == 0) {
                    read_log();
                } else if (strcmp(command_buffer, "erase") == 0) {
                    erase_log();
                } else {
                    printf("Unknown command: %s\n", command_buffer);
                }
                buffer_index = 0;
                memset(command_buffer, 0, sizeof(command_buffer));
            } else if (buffer_index < COMMAND_BUFFER_SIZE - 1) {
                command_buffer[buffer_index++] = (char)c;
            } else {
                printf("Command too long. Clearing buffer.\n");
                buffer_index = 0;
                memset(command_buffer, 0, sizeof(command_buffer));
            }
        }


        sleep_ms(1);
    }

    return 0;
}
