#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"


void led_setup(const uint *led_pin, const uint sw0, const uint sw1, const uint sw2) {
    for (int i = 0; i < 3; i++) {
        gpio_init(led_pin[i]);
        gpio_set_dir(led_pin[i], GPIO_OUT);
    }
    gpio_init(sw0);
    gpio_set_dir(sw0, GPIO_IN);
    gpio_pull_up(sw0);
    gpio_init(sw1);
    gpio_set_dir(sw1, GPIO_IN);
    gpio_pull_up(sw1);
    gpio_init(sw2);
    gpio_set_dir(sw2, GPIO_IN);
    gpio_pull_up(sw2);
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
    *brt += 1;
    if (*brt > 1000) {
        *brt = 1000;
    }
    for (int i = 0; i < 3; i++) {
        pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
    }
}

void dec_brt(int *brt, uint *pwm_slice, uint *pwm_chan) {
    *brt -= 1;
    if (*brt < 0) {
        *brt = 0;
    }
    for (int i = 0; i < 3; i++) {
        pwm_set_chan_level(pwm_slice[i], pwm_chan[i], *brt);
    }
}

uint btn_press(uint sw_n) {
    if(!gpio_get(sw_n)) {
        while (!gpio_get(sw_n)) {sleep_ms(10);}
        return 1;
    }
    return 0;
}


int main() {
    const uint led_pin[3] = {20, 21, 22};
    uint pwm_slice[3];
    uint pwm_chan[3];
    const uint sw0 = 9;
    const uint sw1 = 8;
    const uint sw2 = 7;
    uint press = 0;
    int brt = 500;
    int brt_mem = 500;
    int status = 1;
    led_setup(led_pin, sw0, sw1, sw2);

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
        if(gpio_get(sw1) == 0) {
            press = btn_press(sw1);
            if (press == 1) {
                led_onoff(&status, &brt, &brt_mem, pwm_slice, pwm_chan);
            }
        } else if(gpio_get(sw0) == 0 && status != 0) {
            inc_brt(&brt, pwm_slice, pwm_chan);
        } else if (gpio_get(sw2) == 0 && status != 0) {
            dec_brt(&brt, pwm_slice, pwm_chan);
        }
        printf("%d\n", brt);
        sleep_ms(1);
    }
}
