/*
*  Fuctions to control LEDs 
*
*/

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#ifndef __PILL_PWM__
#define __PILL_PWM__

#define PWM_WRAP 999
#define D1 22
#define D2 21
#define D3 20


enum PWM_LVLS { OFF=0, ON=100 };

//initpwm_inner (uint pin, (struct LED_struct *)led_struct)
void 
initpwm_inner (uint pin)
{
    /* init the PWM on pin */
    gpio_set_function(pin, GPIO_FUNC_PWM);

    /* get PWM slice # of a pin */
    uint slice= pwm_gpio_to_slice_num(pin);
    uint channel= pwm_gpio_to_channel(pin);
    
    pwm_set_clkdiv(slice,
                (float)( clock_get_hz(clk_sys) / (1000*PWM_WRAP)) );

    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_enabled(slice, true);
    
    /* 
    (*led_struct).pin=pin;
    (*led_struct).slice=slice;
    (*led_struct).channel=channel;
    (*led_struct).level=0;
    */
}

void
initpwm ()
{
    /*
        Inittialize PWM on our LEDs.
    */

    /* TODO: remove
    initpwm_inner (D1, &D1_struct);
    initpwm_inner (D2, &D2_struct);
    initpwm_inner (D3, &D3_struct);
    */
    initpwm_inner (D1);
    initpwm_inner (D2);
    initpwm_inner (D3);
}

int
led_set_level (uint LED, uint level)
{
/* 
    Set LED brightness. Any level can be set; 
    Additionally enums OFF && ON are available.
*/
    uint slice= pwm_gpio_to_slice_num(LED);
    uint channel= pwm_gpio_to_channel(LED);

    pwm_set_chan_level( slice, channel, level);
}


#endif
