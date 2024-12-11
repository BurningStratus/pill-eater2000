/*
 * All macros and preprocessor directives were
 * moved to libcommon.h
 *
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/util/queue.h"

// custom libraries
#include "libcommon.h"
#include "libuart.h"
#include "libpwm.h"
#include "librom.h"
#include "seterr.h"

// State Variables
int pill_count = 7;
int motor_position = 0;
static queue_t events;

// Function Prototypes
void initialize_hardware();
void calibrate_dispenser();
void rotate_stepper_one_step();
void dispense_pill();
bool is_pill_detected();
static void piezo_handler();
void report_status(const char *status);

//=EXPERIMENTAL=TECH===========================================================
#ifndef __PILL_PWM__
#define __PILL_PWM__

#define D1 22
#define ON 1
#define OFF 0

void led_set_level (uint LED, uint level)
{
    gpio_put(LED, level);
}
void initpwm ()
{
    gpio_init(D1);
    gpio_set_dir(D1, GPIO_OUT);
   printf("INFO: libpwm.h is not included.\n");
}
#else
#endif

#ifndef __PILL_ROM__
#define __PILL_ROM__
void initrom ()
{
    printf("INFO: librom.h is not included.\n")
}
#endif
//=============================================================================
// * UART functions were moved to libuart.h * //


// Main Function
int main() {
    queue_init(&events, sizeof(int), 25);
    // Initialize hardware
    initialize_hardware();

    // Recalibrate if motor was interrupted
    if (motor_position % 50 != 0) {
        printf("Motor misaligned. Recalibrating...\n");
        calibrate_dispenser();
        motor_position = 0; // Reset motor position after recalibration
    }

    int state = 0;
    char response[STRLEN];

    while (true) {
        switch (state) {
            case 0:
                // Waiting for button press and blinking LED
                printf("Waiting for button press to start calibration...\n");
                while (gpio_get(BUTTON_PIN)) {

                    // gpio_put(D1, true);
                    led_set_level (D1, ON);
                    sleep_ms(200);

                    // gpio_put(D1, false);
                    led_set_level (D1, OFF);
                    sleep_ms(200);
                }
                sleep_ms(DEBOUNCE_DELAY);

                // gpio_put(D1, false);
                led_set_level (D1, OFF);

                printf("Button pressed. Starting calibration...\n");
                calibrate_dispenser();
                motor_position = 0;

                // Keep LED on until next button press
                printf("Calibration complete. Waiting to start dispensing...\n");
                // gpio_put(D1, true);
                led_set_level (D1, ON);

                while (gpio_get(BUTTON_PIN)) {
                    sleep_ms(10);
                }

                sleep_ms(DEBOUNCE_DELAY);
                // gpio_put(D1, false);
                led_set_level (D1, ON);

                state = 1;
                break;

            case 1:
                // Dispense pills
                if (pill_count > 0) {
                    dispense_pill();
                    sleep_ms(10000); // Simulate daily interval for testing
                } else {
                    printf("All pills dispensed. Restarting calibration...\n");
                    report_status("Dispenser empty");
                    state = 0;
                }
                break;

            default:
                state = 0;
                break;
        }
    }
}

// Initialize hardware
void initialize_hardware() {
    stdio_init_all();

    initpwm();
    initrom();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    //gpio_init(LED_PIN);               collision w/ header
    //gpio_set_dir(LED_PIN, GPIO_OUT);  collision w/ header

    gpio_init(OPTICAL_SENSOR_PIN);
    gpio_set_dir(OPTICAL_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(OPTICAL_SENSOR_PIN);

    gpio_init(PIEZO_SENSOR_PIN);
    gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR_PIN);
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, true, piezo_handler);
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, false);

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

// Rotate the stepper motor one step
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
        sleep_ms(5); // Adjust motor speed
    }
    motor_position++;
}

// Calibrate the dispenser
void calibrate_dispenser() {
    printf("Calibrating dispenser...\n");
    int steps = 0;

    // Rotate the motor at least one full turn
    while (gpio_get(OPTICAL_SENSOR_PIN) || steps < 200) {
        rotate_stepper_one_step();
        steps++;
    }
    steps = 0;
    for (int step = 0; step < 20; step++) {
        rotate_stepper_one_step();
    }
    printf("Calibration complete. Motor aligned.\n");
}

// Dispense a pill
void dispense_pill() {
    int piezo_val = 0;
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, true);
    printf("Dispensing pill...\n");

    // Rotate the motor to the next compartment
    for (int step = 0; step < 63; step++) {
        rotate_stepper_one_step();
    }
    sleep_ms(500);
    // Check if the pill is dispensed
    printf("Waiting for pill detection...\n");
    if (queue_try_remove(&events, &piezo_val)) {
        if (piezo_val == 1) {
            pill_count--;
            printf("Pill dispensed successfully. Pills remaining: %d\n", pill_count);
            piezo_val = 0;
        }
        //queue burn
        while (queue_try_remove(&events, &piezo_val)) {
            piezo_val = 0;
        }
    } else {
        printf("No pill detected! Blinking LED...\n");
        pill_count--;
        for (int i = 0; i < 5; i++) {
            // gpio_put(LED_PIN, true);
            led_set_level (D1, ON);
            sleep_ms(200);
            // gpio_put(LED_PIN, false);
            led_set_level (D1, OFF);
            sleep_ms(200);
        }
        gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, false);
        report_status("Pill not detected");
    }
}

static void piezo_handler(const uint gpio, uint32_t event_mask) {
    const int sensor_impact = 1;
    queue_try_add(&events, &sensor_impact);
}

// Check if a pill is detected
bool is_pill_detected() {
    uint64_t start_time = time_us_64();
    int pulse_count = 0;
    int last_sensor_state = 1; // Assume the sensor starts in the "high" state

    printf("Detecting pill...\n");

    while (time_us_64() - start_time < 1000000) { // Wait for up to 1 second
        int current_sensor_state = gpio_get(PIEZO_SENSOR_PIN);

        // Only log when the state changes
        if (current_sensor_state != last_sensor_state) {
            printf("Piezo sensor state changed to: %d\n", current_sensor_state);
            last_sensor_state = current_sensor_state;
        }

        // Count consecutive low signals
        if (current_sensor_state == 0) { // Active low signal detected
            pulse_count++;
            sleep_ms(5); // Short debounce delay
            if (pulse_count > 2) { // Require multiple low readings for confirmation
                printf("Pill detected successfully after %d pulses.\n", pulse_count);
                return true;
            }
        } else {
            pulse_count = 0; // Reset the pulse count if the signal goes high
        }
    }

    printf("No pill detected after 1 second.\n");
    return false;
}

// Report status via LoRaWAN
void report_status(const char *status) {
    char response[STRLEN];
    char command[STRLEN];
    snprintf(command, sizeof(command), "AT+MSG=\"%s\"\r\n", status);
    sendATCommand(command, response);
}

/*
// UART response reader
int readUARTResponse(char *response, int maxlen, int timeout_ms) {
    int response_len = 0;
    uint64_t start_time = time_us_64();

    while (time_us_64() - start_time < timeout_ms * 1000) {
        if (uart_is_readable(UART_NR)) {
            response[response_len++] = uart_getc(UART_NR) & 0xFF;
            if (response_len >= maxlen - 1) break;
        }
    }

    if (response_len > 0) {
        response[response_len] = '\0';
        return 1;
    }
    return 0;
}

// Send AT command to LoRa module
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
*/
