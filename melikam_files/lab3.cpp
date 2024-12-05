#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>

#define GP28 28           // Optical sensor input
#define IN1 2             // Motor driver input 1
#define IN2 3             // Motor driver input 2
#define IN3 6             // Motor driver input 3
#define IN4 13            // Motor driver input 4

#define STEPS_PER_REV_ESTIMATE 4096
#define NUM_CALIBRATION_MEASUREMENTS 3

bool calibrated = false;
uint16_t steps_per_revolution = 0;

// Stepper motor sequence (half-stepping for smooth rotation)
uint8_t stepper_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

void set_stepper_motor(uint8_t step) {
    gpio_put(IN1, stepper_sequence[step][0]);
    gpio_put(IN2, stepper_sequence[step][1]);
    gpio_put(IN3, stepper_sequence[step][2]);
    gpio_put(IN4, stepper_sequence[step][3]);
    sleep_ms(1);
}

// Function to move the motor one step forward
void step_forward() {
    static uint8_t current_step = 0;
    set_stepper_motor(current_step);
    current_step = (current_step + 1) % 8;
}


void calibrate() {
    int measurements[NUM_CALIBRATION_MEASUREMENTS] = {0};
    int total_steps = 0;
    int steps = 0;
    while(gpio_get(GP28)) {

        step_forward();
    }
    printf("Calibration started\n");
    for (int i = 0; i < NUM_CALIBRATION_MEASUREMENTS; i++) {

        while (gpio_get(GP28) == 0) {
            step_forward();
            steps++;
        }

        // Now count steps to the next falling edge
        printf("Now counting to the next falling edge\n");

        while (gpio_get(GP28) == 1) {
            step_forward();
            steps++;
        }
        measurements[i] = steps;
        total_steps += steps;
        steps = 0;
    }

    // Average the measurements
    steps_per_revolution = total_steps / NUM_CALIBRATION_MEASUREMENTS;
    calibrated = true;
}

void run_motor(int N) {
    if (!calibrated) {
        printf("Motor is not calibrated\n");
        return;
    }

    // Calculate the number of steps for 1/8th revolution
    int steps_to_move = (steps_per_revolution / 8) * N;
    printf("Steps to move %d", steps_to_move);
    // Move the motor N times 1/8 revolution (or one full revolution if N = 8)
    for (int i = 0; i < steps_to_move; i++) {
        step_forward();
    }
}

void status() {
    printf("Calibration: %s\n", calibrated ? "Yes" : "No");
    if (calibrated) {
        printf("Steps per Revolution: %d\n", steps_per_revolution);
    } else {
        printf("Steps per Revolution: not available\n");
    }
}

int main() {
    stdio_init_all();
    char input[20];  // Buffer to hold the entire input line
    char command[10];
    int N;

    // Initialize GPIOs
    gpio_init(GP28);
    gpio_set_dir(GP28, GPIO_IN); // Optical sensor input with pull-up
    gpio_pull_up(GP28);
    gpio_init(IN1); gpio_init(IN2); gpio_init(IN3); gpio_init(IN4);
    gpio_set_dir(IN1, GPIO_OUT); gpio_set_dir(IN2, GPIO_OUT);
    gpio_set_dir(IN3, GPIO_OUT); gpio_set_dir(IN4, GPIO_OUT);

    while (1) {
        printf("Enter command: ");

        // Use fgets to read the whole line, allowing for commands like "run 3"
        if (fgets(input, sizeof(input), stdin) == NULL) continue;

        // Try to parse the command and optional number
        if (sscanf(input, "%s %d", command, &N) == 1) {
            // If only the command is entered without a number, set default N = 8
            N = 8;
        }

        if (strcmp(command, "status") == 0) {
            status();
        } else if (strcmp(command, "calib") == 0) {
            calibrate();
            printf("Calibration completed.\n");
        } else if (strcmp(command, "run") == 0) {
            run_motor(N);
            printf("Motor ran %d times 1/8th of a revolution.\n", N);
        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
