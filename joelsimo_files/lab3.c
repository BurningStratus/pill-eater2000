#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"
#include <string.h>


const int step[8][4] = {
    {2, 0, 0, 0},
    {2, 3, 0, 0},
    {0, 3, 0, 0},
    {0, 3, 6, 0},
    {0, 0, 6, 0},
    {0, 0, 6, 13},
    {0, 0, 0, 13},
    {2, 0, 0, 13}
};

const uint stepper_pins[4] = {2, 3, 6, 13};
const uint opto_fork = 28;


void gpio_setup() {
    for (int i = 0; i < 4; i++) {
        gpio_init(stepper_pins[i]);
        gpio_set_dir(stepper_pins[i], GPIO_OUT);
    }
    gpio_init(opto_fork);
    gpio_set_dir(opto_fork, GPIO_IN);
    gpio_pull_up(opto_fork);
}

void step_1() {
    static uint step_n = 0;
    gpio_put(stepper_pins[0], step[step_n][0]);
    gpio_put(stepper_pins[1], step[step_n][1]);
    gpio_put(stepper_pins[2], step[step_n][2]);
    gpio_put(stepper_pins[3], step[step_n][3]);
    step_n = (step_n + 1) % 8;
    sleep_ms(1);
}

void status(const uint calibrated, const uint step_per_rev) {
    if (calibrated != 1) {
        printf("not available\n");
    } else {
        printf("steps per revolution %d\n", step_per_rev);
    }
}

void calib(uint *steps_per_rev, uint *calibrated) {
    uint steps = *steps_per_rev;

    while (gpio_get(28)) {
        step_1();
    }
    for (uint i = 0; i < 3; i++) {
        while (gpio_get(28) != 1) {
            step_1();
            steps++;
        }
        while (gpio_get(28)) {
            step_1();
            steps++;
        }
    }
    printf("%d\n", steps);
    *steps_per_rev = steps / 3;
    *calibrated = 1;
}

void run_N(const uint run_num, uint steps_per_rev) {
    printf("Running %d\n", run_num);
    uint steps = (steps_per_rev / 8) * run_num;
    for (uint i = 0; i < steps; i++) {
        step_1();
    }
}


int main() {

    gpio_setup(stepper_pins, opto_fork);

    char *str_cmmd[15];
    char *str_input[15];
    uint run_num = 0;
    uint calibrated = 0;
    uint steps_per_rev = 0;

    stdio_init_all();


    while (true) {
        printf("Give a command (status, calib or run N)\n");
        if (fgets(str_cmmd, 15, stdin) == NULL) {
            continue;
        }
        sscanf(str_cmmd, "%s %d", str_input, &run_num);
        if (run_num == 0) {
            run_num = 8;
        }
        if (strcmp(str_input, "status") == 0) {
            status(calibrated, steps_per_rev);
        } else if (strcmp(str_input, "calib") == 0) {
            calib(&steps_per_rev, &calibrated);
            printf("steps: %d\n", steps_per_rev);
        } else if (strcmp(str_input, "run") == 0 && calibrated == 1) {
            run_N(run_num, steps_per_rev);
        } else {
            printf("Please provide a valid command\n");
        }
        strcpy(str_cmmd, "");
        strcpy(str_input, "");
        run_num = 0;
    }
    return 0;
}
