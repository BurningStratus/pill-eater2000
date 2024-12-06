#include <stdio.h>
//#include <stdint.h>
//#include <stdbool.h>
//#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SW0 9
#define SW1 8
#define SW2 7
#define SDA 16
#define SCL 17
#define I2C_DEVICE_ID 0x50
#define EEPROM_ADDR 254
#define LOG_ADDR_START 0
#define LOG_ADDR_END 32

uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;
    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) x);
    }
    return crc;
}

typedef struct {
    uint8_t state;
    uint8_t not_state;
} ledstate;

void set_led_state(ledstate *ls, const uint8_t value) {
    ls->state = value;
    ls->not_state = ~value;
}

bool led_state_is_valid(const ledstate *ls) {
    return ls->state == (uint8_t)~ls->not_state;
}

void led_btn_pins(const uint *led_pins, const uint *btns) {
    for (int i = 0; i < 3; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
        gpio_init(btns[i]);
        gpio_set_dir(btns[i], GPIO_IN);
        gpio_pull_up(btns[i]);

    }
}

uint btn_press(const uint sw_n) {
    if(!gpio_get(sw_n)) {
        while (!gpio_get(sw_n)) {sleep_ms(10);}
        return 1;
    }
    return 0;
}


void mem_read(const uint8_t mem_address, ledstate *ls) {
    uint8_t read_data[2];
    uint8_t read_address[2];
    read_address[0] = 0;
    read_address[1] = mem_address;
    i2c_write_blocking(i2c0, I2C_DEVICE_ID, read_address, sizeof(read_address), true);
    i2c_read_blocking(i2c0, I2C_DEVICE_ID, read_data, sizeof(read_data), false);
    ls->state = read_data[0];
    ls->not_state = read_data[1];
}


void mem_write(const uint8_t mem_address, const ledstate *ls) {
    uint8_t data_package[4];
    data_package[0] = 0;
    data_package[1] = mem_address;
    data_package[2] = ls->state;
    data_package[3] = ls->not_state;
    i2c_write_blocking(i2c0, I2C_DEVICE_ID, data_package, sizeof(data_package), false);
    sleep_ms(5);
}

bool read_led(ledstate *ls) {
    mem_read(EEPROM_ADDR, ls);
    return led_state_is_valid(ls);
}

void toggle_led(ledstate *ls, const uint led_pin) {
    uint8_t led_n = 0;
    if (led_pin == 21) {
        led_n = 1;
    } else if (led_pin == 22) {
        led_n = 2;
    }
    uint8_t state = ls->state ^ 1 << led_n;
    //printf("state: %d\n", state);
    set_led_state(ls, state);
    gpio_put(led_pin, state >> led_n & 1);
    mem_write(EEPROM_ADDR, ls);
}

void start_config(ledstate *ls, const uint *led_pins) {
    if (read_led(ls) == 0) {
        set_led_state(ls, 0b010);
        mem_write(EEPROM_ADDR, ls);
        printf("No data in EEPROM, using default setting\n");
    }

    for (uint i = 0; i < 3; i++) {
        gpio_put(led_pins[i], ls->state >> i & 1);
    }
}

void i2c_setup() {
    i2c_init(i2c0, 400000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
}

void log_write(const uint8_t entry_address, const uint8_t *data_words, const int msg_len, const uint16_t crc) {
    printf("%d\n", crc);
    int i = 0;
    uint8_t log_entry[msg_len + 3];
    log_entry[0] = 0;
    log_entry[1] = entry_address;
    while (data_words[i] != '\0') {
        log_entry[2+i] = data_words[i];
        i++;
    }
    log_entry[msg_len + 1] = '\0';
    uint8_t crc_arr[2] = { crc & 0xFF, crc >> 8};

    log_entry[msg_len + 2] = crc_arr[0];
    log_entry[msg_len + 3] = crc_arr[1];
    i2c_write_blocking(i2c0, I2C_DEVICE_ID, log_entry, sizeof(log_entry), false);
    sleep_ms(5);
    printf("AAAA\n");
}

void boot_log(const uint8_t entry_address) {
    const uint8_t boot[] = "boot";
    uint16_t crc = crc16(boot, sizeof(boot));
    log_write(entry_address, boot, sizeof(boot), crc);
}

void led_state_log(const uint8_t entry_address, const uint *led_pins) {
    uint8_t leds[3];
    for (uint i = 0; i < 3; i++) {
        leds[i] = gpio_get(led_pins[i]);
        if (leds[i] == '\0') {
            leds[i] = '0';
        }
    }

    uint8_t led_state[] = "LED state to: [LED0:  LED1:  LED2: ]";
    led_state[20] = leds[0];
    led_state[27] = leds[1];
    led_state[34] = leds[2];
    uint16_t crc = crc16(led_state, sizeof(led_state));
    log_write(entry_address, led_state, sizeof(led_state), crc);
    sleep_ms(5);
    printf("BBBBB\n");
}

void log_read() {
    uint i = 0;
    uint8_t msg_addr[2];
    msg_addr[0] = 0;
    msg_addr[1] = LOG_ADDR_START;
    uint8_t log_entry[64] = {0};
    i2c_write_blocking(i2c0, I2C_DEVICE_ID, msg_addr, sizeof(msg_addr), true);
    i2c_read_blocking(i2c0, I2C_DEVICE_ID, log_entry, sizeof(log_entry), false);
    if (log_entry[0] != '\0') {
        printf("%s\n", (char *)log_entry);
        while (log_entry[0] != '\0') {
            i2c_write_blocking(i2c0, I2C_DEVICE_ID, msg_addr + 64*i, sizeof(msg_addr), true);
            i2c_read_blocking(i2c0, I2C_DEVICE_ID, log_entry, sizeof(log_entry), true);
            printf("%s\n", (char *)log_entry);
        }
    } else {
        printf("Empty log\n");
    }
}

uint8_t find_entry_address() {
    uint8_t i = 0;
    uint8_t msg_addr[2];
    msg_addr[0] = 0;
    msg_addr[1] = LOG_ADDR_START;
    uint8_t log_entry[1] = {0};
    i2c_write_blocking(i2c0, I2C_DEVICE_ID, msg_addr, sizeof(msg_addr), true);
    i2c_read_blocking(i2c0, I2C_DEVICE_ID, log_entry, sizeof(log_entry), false);
    if (log_entry[0] == '\0') {
        i = 0;
        printf("%d\n", i);
        return i;
    }
    while (i < 32) {
        i2c_write_blocking(i2c0, I2C_DEVICE_ID, msg_addr + 64*i, sizeof(msg_addr), true);
        i2c_read_blocking(i2c0, I2C_DEVICE_ID, log_entry, sizeof(log_entry), false);
        if (log_entry[0] == '\0') {
            return i;
        }
        i++;
    }
    return 33;
}

int main() {
    stdio_init_all();
    timer_hw->dbgpause = 0;

    const uint led_pins[] = {20, 21, 22};
    const uint btns[] = {9, 8, 7};
    led_btn_pins(led_pins, btns);

    uint debounce = 0;
    ledstate ls;

    i2c_setup();
    start_config(&ls, led_pins);

    printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
    printf("Boot LED state: [LED0: %d LED1: %d LED2: %d]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]));

    led_state_log(0, led_pins);
    log_read();

    while (true) {
        if (!gpio_get(SW0)) {
            debounce = btn_press(SW0);
            if (debounce) {
                toggle_led(&ls, led_pins[0]);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d T:%llu]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]), time_us_64() / 1000000);
            }
        }
        if (!gpio_get(SW1)) {
            debounce = btn_press(SW1);
            if (debounce) {
                toggle_led(&ls, led_pins[1]);
                printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d T:%llu]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]), time_us_64() / 1000000);
            }
        }
        if (!gpio_get(SW2)) {
            debounce = btn_press(SW2);
            if (debounce) {
                toggle_led(&ls, led_pins[2]);
                printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d T:%llu]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]), time_us_64() / 1000000);
            }
        }
        sleep_ms(5);
    }
return 0;
}