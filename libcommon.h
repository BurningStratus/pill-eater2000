
#ifndef __PILL_COMMON_MACROS__
#define __PILL_COMMON_MACROS__

#define BUTTON_PIN 8 
//#define LED_PIN 20 defined in libpwm.h
#define OPTICAL_SENSOR_PIN 28
#define PIEZO_SENSOR_PIN 27
#define STEPPER_PIN1 2
#define STEPPER_PIN2 3
#define STEPPER_PIN3 6
#define STEPPER_PIN4 13
#define STRLEN 80
#define MAX_ATTEMPTS 5
#define DEBOUNCE_DELAY 300 // Debounce time in milliseconds
#define STATE_ADDR 0x0000 

// macros
#define ARRSIZE (arr) ( sizeof(arr)/sizeof(arr[0]) ) // useful macro

#endif
