#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"

static queue_t events;
static uint32_t time_us;

static void gpio_handler_rot(const uint gpio, uint32_t event_mask) {
    int rot = -1;
    if (gpio == 10) {
        if (gpio_get(11) == 0) {
            rot = 1;
            queue_try_add(&events, &rot);
        }
    } else if (gpio == 11) {
        if (gpio_get(10) == 0) {
            rot = 2;
            queue_try_add(&events, &rot);
        }
    } else if (gpio == 12) {
        if (time_us_32() - time_us > 80000) {
            rot = 3;
            queue_try_add(&events, &rot);
        }
        time_us = time_us_32();
    }
}

void led_setup(const uint *led_pin, const uint Rot_A, const uint Rot_Sw, const uint Rot_B) {
    for (int i = 0; i < 3; i++) {
        gpio_init(led_pin[i]);
        gpio_set_dir(led_pin[i], GPIO_OUT);
    }
    gpio_init(Rot_A);
    gpio_set_dir(Rot_A, GPIO_IN);
    //gpio_pull_up(Rot_A);
    gpio_init(Rot_Sw);
    gpio_set_dir(Rot_Sw, GPIO_IN);
    gpio_pull_up(Rot_Sw);
    gpio_init(Rot_B);
    gpio_set_dir(Rot_B, GPIO_IN);
    //gpio_pull_up(Rot_B);
}

void led_onoff(int *status, int *brt, int *brt_mem, uint *pwm_slice, uint *pwm_chan) {
    if (*brt == 0) {
        *status = 0;
    } else {
        *status = 1;
    }
    if(*status == 1) {
        *brt_mem = *brt;
        *brt = 0;
        for (int i = 0; i < 3; i++) {
            pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
        }
        *status = 0;
    } else if (*status == 0) {
        if (*brt_mem <= 0) {
            *brt = 500;
            for (int i = 0; i < 3; i++) {
                pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
            }
        } else {
            *brt = *brt_mem;
            for (int i = 0; i < 3; i++) {
                pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
            }
        }
        *status = 1;
    }
}

void inc_brt(int *brt, uint *pwm_slice, uint *pwm_chan) {
    *brt += 25;
    if (*brt > 1000) {
        *brt = 1000;
    }
    for (int i = 0; i < 3; i++) {
        pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
    }
}

void dec_brt(int *brt, uint *pwm_slice, uint *pwm_chan) {
    *brt -= 25;
    if (*brt < 0) {
        *brt = 0;
    }
    for (int i = 0; i < 3; i++) {
        pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
    }
}

int main() {
    const uint led_pin[3] = {20, 21, 22};
    uint pwm_slice[3];
    uint pwm_chan[3];
    const uint Rot_A = 10;
    const uint Rot_Sw = 12;
    const uint Rot_B = 11;
    uint press = 0;
    int brt = 500;
    int brt_mem = 500;
    int status = 1;
    int rot_val = 0;
    led_setup(led_pin, Rot_A, Rot_Sw, Rot_B);
    queue_init(&events, sizeof(int), 50);

    gpio_set_irq_enabled_with_callback(10, GPIO_IRQ_EDGE_RISE, true, &gpio_handler_rot);
    gpio_set_irq_enabled_with_callback(11, GPIO_IRQ_EDGE_RISE, true, &gpio_handler_rot);
    gpio_set_irq_enabled_with_callback(12, GPIO_IRQ_LEVEL_LOW, true, &gpio_handler_rot);


    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, 125);
    pwm_config_set_wrap(&config, 999);
    for (int i = 0; i < 3; i++) {
        pwm_slice[i] = pwm_gpio_to_slice_num(led_pin[i]);
        pwm_chan[i] = pwm_gpio_to_channel(led_pin[i]);
        pwm_set_enabled(pwm_slice[i], false);
        pwm_init(pwm_slice[i], &config, false);
        //pwm_set_chan_level(pwm_slice[i], pwm_chan[i], brt);
        gpio_set_function(led_pin[i], GPIO_FUNC_PWM);
        pwm_set_enabled(pwm_slice[i], true);
    }
    for (int i = 0; i < 3; i++) {
        pwm_set_chan_level(pwm_slice[i], pwm_chan[i], brt);
    }

    // Initialize chosen serial port
    stdio_init_all();



    // Loop forever
    while (true) {
        while (queue_try_remove(&events, &rot_val)) {
            if (rot_val == 1 && status == 1) {
                inc_brt(&brt, pwm_slice, pwm_chan);
            } else if (rot_val == 2 && status == 1) {
                dec_brt(&brt, pwm_slice, pwm_chan);
            } else if (rot_val == 3) {
                led_onoff(&status, &brt, &brt_mem, pwm_slice, pwm_chan);
            }
            printf("rot_val: %d brt: %d\n", rot_val, brt);
        }
    }
}
