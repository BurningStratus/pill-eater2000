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
static uint8_t      pill_count = 7;
int                 motor_position = 0;

static  queue_t events;
enum    prog_states {STANDBY, WORKING, CALIBRATE, CAL_STANDBY, CAL_DISPENSE};

// Function Prototypes
void initialize_hardware();
void calibrate_dispenser();
void rotate_stepper_one_step();
void dispense_pill();
bool is_pill_detected();
static void piezo_handler();
void report_status(const char *status);
void netw_connect();

int st_update(uint8_t st_prog, uint8_t pill_count); //update state
uint8_t st_get(uint8_t *state, uint8_t *pill_amt);  // get state

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

// TODO: DEBUG ONLY
void 
rom_dump ()
{
    uint16_t startmem_addr = STATE_ADDR;    
    uint16_t endmem_addr = STATE_ADDR+64;    

    printf("\n");
    for (int i=startmem_addr; i<endmem_addr; i++)
        printf("=");
    printf("HEXDUMP: >\n");

    for (int i=startmem_addr; i<endmem_addr; i++)     
    {
        if (i % 64 == 0)
            printf("\n0x%04X\t", i);
        printf("%02X ", readb(i) ); //print out only printable chars
    }

    // ASCII dump <0x7FC0 is the limit>
    printf("\n ASCII DUMP: > \n");
    for (int i=startmem_addr; i<endmem_addr; i++)     
    {
        if (i % 64 == 0)
            printf("\n0x%04X\t", i);
        char dst_str = (char)readb (i) ;
        if ( (char)dst_str >= 33 && (char)dst_str <= 126) 
            printf("%c", dst_str ); //print out only printable chars
        else 
            printf("."); // if special chars, print a dot
    }
}

// TODO: DEBUG ONLY

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

    st_update(state, pill_count);
    // rom_dump ();

    while (true) {
        switch (state) {
            case STANDBY:
                st_update(state, pill_count);

                // Waiting for button press and blinking LED
                printf("Waiting for button press to start calibration...\n");
                while (gpio_get(BUTTON_PIN)) {

                    led_set_level (D1, ON);
                    sleep_ms(200);

                    led_set_level (D1, OFF);
                    sleep_ms(200);
                }
                sleep_ms(DEBOUNCE_DELAY);

                led_set_level (D1, OFF);

                printf("Button pressed. Starting calibration...\n");

                st_update(CALIBRATE, pill_count);
                calibrate_dispenser();
                st_get(NULL, NULL); // print out info

                motor_position = 0;

                // Keep LED on until next button press
                printf("Calibration complete. Waiting to start dispensing...\n");
                led_set_level (D1, ON);

                while (gpio_get(BUTTON_PIN)) {
                    sleep_ms(10);
                }

                sleep_ms(DEBOUNCE_DELAY);
                led_set_level (D1, ON);

                state = 1;
                break;

            case WORKING:

                st_update(state, pill_count);
                st_get(NULL, NULL); // print out info

                // Dispense pills
                if (pill_count > 0) {
                    st_update(CAL_DISPENSE, pill_count);

                    dispense_pill();
                    sleep_ms(10000); // Simulate daily interval for testing
                } else {
                    printf("All pills dispensed. Restarting calibration...\n");
                    report_status("Dispenser empty");

                    state = 0;
                    st_update(state, pill_count);
                    st_get(NULL, NULL); // print out info
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
    printf("Calibration complete. Motor aligned.\n");
}

// Dispense a pill
void dispense_pill() {
    int piezo_val = 0;
    gpio_set_irq_enabled(PIEZO_SENSOR_PIN, GPIO_IRQ_EDGE_RISE, true);
    printf("Dispensing pill...\n");

    // Rotate the motor to the next compartment
    for (int step = 0; step < 50; step++) {
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
void netw_connect()
{
    /*
    1. +MODE=LWOTAA
    2. +KEY=APPKEY,”7c00d832d4ab66faee402dbb2c996da3"
    3. +CLASS=A
    4. +PORT=8
    5. +JOIN
    6. +MSG=”text message” 
    */
    char    resp [60]="";
    char    cmdseq [][51] = {
        "+MODE=LWOTAA\n",
        "+KEY=APPKEY,\”7c00d832d4ab66faee402dbb2c996da3\"\n",
        "+CLASS=A\n",
        "+PORT=8\n",
        "+JOIN\n",
        "+MSG=\"TRANSMISSION FROM devE773\"\n"
    };
    for (int i=0; i<6; i++)
    {
        sendATCommand(cmdseq[i], resp);
        printf("INFO NET: %s\n", resp);
        memset(resp, 0, sizeof(resp));
    }
}

// Report status via LoRaWAN
void report_status(const char *status) {
    char response[STRLEN];
    char command[STRLEN];
    snprintf(command, sizeof(command), "AT+MSG=\"%s\"\r\n", status);
    sendATCommand(command, response);
}

int st_update(uint8_t st_prog, uint8_t pill_count)
{
    uint8_t     payload[4]="";
    int         stat;

    payload[0] =  st_prog; 
    payload[1] = ~st_prog;     // validate
    payload[2] =  pill_count; 
    payload[3] = ~pill_count;  // validate

    //printf("INFO STATE >> 0x%08X at 0x%04X\n", payload, (uint16_t)STATE_ADDR);

    stat=writepg((uint16_t)STATE_ADDR, payload, 4); 

    /*
    printf("INFO STATE w[%d] >> %u ~%u %u ~%u at 0x%04X\n", 
        stat,
        payload[0], 
        payload[1], 
        payload[2], 
        payload[3], 
        (uint16_t)STATE_ADDR
    );
    */

    if (stat < 1)
        ERROR_CODE=WRITEPG;
    return stat;
}

uint8_t st_get(uint8_t *prog_state, uint8_t *pillamt_ptr)
{
    uint8_t     *data_ptr = seqread( (uint16_t)STATE_ADDR, 4 );
    uint8_t     st_read=0;
    uint8_t     pillamt_read=0;

    // validate and save
    if ( data_ptr[0] == (uint8_t) ~data_ptr[1] )
        st_read      = data_ptr[0]; // write state if valid
    if ( data_ptr[2] == (uint8_t) ~data_ptr[3] )
        pillamt_read = data_ptr[2]; // write pill amt if valid


    if ( !prog_state || !pillamt_ptr ) // check if pointers are not null
    {
        printf("INFO STATE r >> %u ~%u %u ~%u 0x%04X at 0x%04X\n", 
            data_ptr[0], 
            data_ptr[1], 
            data_ptr[2], 
            data_ptr[3], 
            data_ptr,
            (uint16_t)STATE_ADDR
        );
        free(data_ptr); // free bc seqread calloc:s the memory

        ERROR_CODE=4;
        return 1;
    } else { 
        *prog_state = st_read;  // write and call it a day
        *pillamt_ptr= pillamt_read;
        free(data_ptr); // free bc seqread calloc:s the memory

        return 0;
    }
    
    /* IF debug needed
    printf("INFO STATE r >> %u ~%u %u ~%u 0x%04X at 0x%04X\n", 
        data_ptr[0], 
        data_ptr[1], 
        data_ptr[2], 
        data_ptr[3], 
        data_ptr,
        (uint16_t)STATE_ADDR
    );
    */
}
