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
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
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

    while (true) {
        if (!gpio_get(SW0)) {
            debounce = btn_press(SW0);
            if (debounce) {
                toggle_led(&ls, led_pins[0]);
                printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]));
            }
        }
        if (!gpio_get(SW1)) {
            debounce = btn_press(SW1);
            if (debounce) {
                toggle_led(&ls, led_pins[1]);
                printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]));
            }
        }
        if (!gpio_get(SW2)) {
            debounce = btn_press(SW2);
            if (debounce) {
                toggle_led(&ls, led_pins[2]);
                printf("Seconds since boot: %llu\n", time_us_64() / 1000000);
                printf("LED state to: [LED0: %d LED1: %d LED2: %d]\n", gpio_get(led_pins[0]), gpio_get(led_pins[1]), gpio_get(led_pins[2]));
            }
        }
        sleep_ms(5);
    }
return 0;
}