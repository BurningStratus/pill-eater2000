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
void rotate_stepper_one_step(int step_dir);
void dispense_pill();
static void piezo_handler();
void report_status(const char *status);
void netw_connect();

int st_update(uint8_t st_prog, uint8_t pill_count); //update state
uint8_t st_get(uint8_t *state, uint8_t *pill_amt);  // get state
uint8_t setromrot (uint8_t st_rot);
uint8_t getromrot ();

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

    int         state = 0;
    char        response[STRLEN];
    uint8_t     rom_pillamt;
    uint8_t     rom_state;

    netw_connect();
    report_status("TRANSMISSION_0x77FFAAD");
    printf ("\n\n\n\n============================================\n");

    uint8_t SPIN=1;
    
    st_get ((uint8_t *)&rom_state, (uint8_t *)&rom_pillamt);
    if (rom_pillamt != 0 && rom_pillamt != pill_count) // interrupted
    {
        printf("interrupt detected! ST:%u PA:%u\n", rom_state, rom_pillamt);
        while ( gpio_get(OPTICAL_SENSOR_PIN) ) // spin till optofork
            rotate_stepper_one_step(-1);
		for (int step = 0; step < 20; step++) // align offset from optofork
        	rotate_stepper_one_step(-1);


		pill_count = rom_pillamt;
		int n_steps = (7 - pill_count)*63;
		for (int step = 0; step < n_steps; step++) //Move to proper position
			rotate_stepper_one_step(1);
        st_get(NULL, NULL);
        motor_position = 0; // Reset motor position after recalibration
		if ( rom_state > 2 )
			state = rom_state;
    }

    /*
    // Recalibrate if motor was interrupted
    if (motor_position % 50 != 0) {
        printf("Motor misaligned. Recalibrating...\n");

    }
    */

    // rom_dump ();

    while (true)
    {
        switch (state)
        {
            case STANDBY:
				pill_count = 7;
                st_update(state, pill_count);

                // Waiting for button press and blinking LED
                printf("Waiting for button press to start calibration...\n");
                while (gpio_get(BUTTON_PIN))
                {
                    led_set_level (D1, ON);
                    sleep_ms(200);
                    led_set_level (D1, OFF);
                    sleep_ms(200);
                }
                while ( !gpio_get(BUTTON_PIN) )
                    ;

                led_set_level (D1, OFF);
                printf("Button pressed. Starting calibration...\n");
                state = CALIBRATE;
                break;

            case CALIBRATE:
                st_update(state, pill_count);
                //st_get(NULL, NULL); // print out info

                calibrate_dispenser();
                motor_position = 0;

                // Keep LED on until next button press
                led_set_level (D1, ON);
                state=CAL_STANDBY;
                while ( gpio_get(BUTTON_PIN) ) {
					sleep_ms(100);
				}
                    ;
                led_set_level (D1, OFF);
                while ( !gpio_get(BUTTON_PIN) )
                    ;
                break;

            case CAL_STANDBY:
                st_update(state, pill_count);

                led_set_level (D1, ON);
                sleep_ms(5000); // Simulate daily interval for testing
                state = WORKING;
                break;

            case WORKING:
                st_update(state, pill_count);

                // Dispense pills
                if (pill_count > 0)
                {
                    state = CAL_DISPENSE;
                    break;
                } else {
                    printf("All pills dispensed. Restarting calibration...\n");
                    //report_status("Dispenser empty");
                    state = STANDBY;
                    break;
                }
                state=STANDBY;
                ERROR_CODE=5; //skipped state
                break;

            case CAL_DISPENSE:
                st_update(state, pill_count);
                st_get(NULL, NULL); // print out info

                dispense_pill();
				pill_count--;
                state = CAL_STANDBY;
                break;

            default:
                state = STANDBY;
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
void rotate_stepper_one_step(int step_dir) {
    static const bool step_sequence[4][4] = {
        {true, false, false, false},
        {false, true, false, false},
        {false, false, true, false},
        {false, false, false, true}
    };

    if ( step_dir > 0 ) // positive => clockwise
    {
        for (int i = 0; i < 4; i++) {
            gpio_put(STEPPER_PIN1, step_sequence[i][0]);
            gpio_put(STEPPER_PIN2, step_sequence[i][1]);
            gpio_put(STEPPER_PIN3, step_sequence[i][2]);
            gpio_put(STEPPER_PIN4, step_sequence[i][3]);
            sleep_ms(5); // Adjust motor speed
        }
        motor_position++;
    }
    else if ( step_dir < 0) // 0 => counter clockwise
    {
        for (int i = 4; i > 0; i--) {
            gpio_put(STEPPER_PIN1, step_sequence[i][0]);
            gpio_put(STEPPER_PIN2, step_sequence[i][1]);
            gpio_put(STEPPER_PIN3, step_sequence[i][2]);
            gpio_put(STEPPER_PIN4, step_sequence[i][3]);
            sleep_ms(5); // Adjust motor speed
        }
        motor_position--;
    }

    if (motor_position < 0)
        motor_position = 7;
}

// Calibrate the dispenser
void calibrate_dispenser() {
    printf("Calibrating dispenser...\n");
    int steps = 0;

    // Rotate the motor at least one full turn
    //while (gpio_get(OPTICAL_SENSOR_PIN) || steps < 200) {
    while (gpio_get(OPTICAL_SENSOR_PIN) && steps < 504)
    {
        rotate_stepper_one_step(1);
        steps++;
    }
    for (int step = 0; step < 20; step++) {
        rotate_stepper_one_step(1);
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
        rotate_stepper_one_step(1);
    }
    sleep_ms(500);
    // Check if the pill is dispensed
    printf("Waiting for pill detection...\n");
    if (queue_try_remove(&events, &piezo_val)) {
        if (piezo_val == 1) {
            //pill_count--;
            //printf("Pill dispensed successfully. Pills remaining: %d\n", pill_count);
            piezo_val = 0;
        }
        //queue burn
        while (queue_try_remove(&events, &piezo_val)) {
            piezo_val = 0;
        }
    } else {
        printf("No pill detected! Blinking LED...\n");
		//pill_count--;
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
        "+AT\n"
        "AT+MODE=LWOTAA\n",
        "AT+KEY=APPKEY,\"7c00d832d4ab66faee402dbb2c996da3\"\n",
        "AT+CLASS=A\n",
        "AT+PORT=8\n",
        "AT+JOIN\n",
        "AT+MSG=\"TRANSMISSION FROM devE773\"\n"
    };

    for ( int i=0; i<7; i++ )
    {
        sendATCommand(cmdseq[i], resp, 0);
        printf("INFO NET: %s", resp);
        memset(resp, 0, sizeof(resp));
    }
    
}

// Report status via LoRaWAN
void report_status(const char *status) {
    char response[STRLEN];
    char command[STRLEN];
    snprintf(command, sizeof(command), "AT+MSG=\"%s\"\r\n", status);
    sendATCommand(command, response, 0);
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

uint8_t setromrot (uint8_t st_rot)
{
    uint8_t     buf[2];
    int         stat;

    buf[0] =  (uint8_t)st_rot;
    buf[1] = ~(uint8_t)st_rot;
    stat = writepg (STATE_ADDR+5, buf, 2);
    // printf("SETROMROT: %u %u\n", buf[0], buf[1]);
    if ( stat < 0 )
        ERROR_CODE = 6; // TODO: SET ERROR CODES
    
    return stat;
}

uint8_t getromrot ()
{
    uint8_t     *st_rot = seqread (STATE_ADDR+5, 2);
    uint8_t     rot = st_rot[0];
    uint8_t     irot = st_rot[1];

    free(st_rot);
    // printf("GETROMROT: %u %u\n", rot, irot);

    if ( (uint8_t)rot == (uint8_t) ~irot )
        return rot; 


    ERROR_CODE = 7;
    return 0;
}
