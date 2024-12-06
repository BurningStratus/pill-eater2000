#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_DUTY_STEP 10
#define WRAP_ 999
#define PWM_OUTFMOD 1000

int initf(const uint SW_1, const uint D1);
int initctb(const uint CTL_UP, const uint CTL_DN); // UP/DOWN
struct pwmstruct *mkpwm(uint pin);
struct pwmstruct *mkstruct(void);
void 
setcommpwm(uint level, 
                struct pwmstruct *d1_struct, 
                struct pwmstruct *d2_struct, 
                struct pwmstruct *d3_struct);

struct pwmstruct {
    uint    PIN;
    uint    SLICE;
    uint    CHANN;
};


int main() {
    const uint      SW_1    = 8;
    const uint      SW_0    = 7;
    const uint      SW_2    = 9;

    const uint      D1      = 22;
    const uint      D2      = 21;
    const uint      D3      = 20;

    unsigned int    PWM_DUTY= 0;
    unsigned int    PWM_MEM = 0;

    bool            D1_STAT = false;
    
    initf(SW_1, D1);
    initctb(SW_0, SW_2);
    
    struct pwmstruct *D1_STRUCT = mkpwm(D1);
    struct pwmstruct *D2_STRUCT = mkpwm(D2);
    struct pwmstruct *D3_STRUCT = mkpwm(D3);
    
    // Loop forever
    while (true) {
        bool SW1_STAT = gpio_get(SW_1);

        sleep_ms(10);

        printf("\rPIN 8 > %d | PWM > %d", SW1_STAT, PWM_DUTY );
        stdio_flush();

        if ( !SW1_STAT )
        {
            bool SW1_STAT = gpio_get(SW_1);
            if ( !SW1_STAT && D1_STAT && ( PWM_DUTY == 0 ) )
            {
                PWM_DUTY = (uint) ( (WRAP_+1) * 50 ) / 100;
                D1_STAT=true;
            } else if ( !SW1_STAT && D1_STAT  ) {
                D1_STAT=false;
                setcommpwm(0, D1_STRUCT, D2_STRUCT, D3_STRUCT);

            } else if ( !SW1_STAT && !D1_STAT ) {
                D1_STAT=true;
                setcommpwm(PWM_DUTY, D1_STRUCT, D2_STRUCT, D3_STRUCT);
            }

            while ( !gpio_get(SW_1) )
                ; // busy loop while pin reads 0(button is pressed)
            // program stops while 
        }

        /* 
        translation: only if LED is ON, SW0 && SW2 change the brightness.
        if brightness is 0, then set it to 50%.
        */
        if ( D1_STAT )
        {
            if (!gpio_get(SW_0) && PWM_DUTY < (WRAP_+1) ) {
                PWM_DUTY += PWM_DUTY_STEP;
            } else if ( !gpio_get(SW_2) && PWM_DUTY>0 ) {
                if (PWM_DUTY < PWM_DUTY_STEP && PWM_DUTY > 0) {
                    PWM_DUTY--;
                }
                else {
                    PWM_DUTY -= PWM_DUTY_STEP;
                }
            }

            setcommpwm(PWM_DUTY, D1_STRUCT, D2_STRUCT, D3_STRUCT);
        }
    }
}

struct pwmstruct *mkpwm(uint pin)
{
    //init the PWM on a pin
    /* init the PWM on pin */
    gpio_set_function(pin, GPIO_FUNC_PWM);

    /* get PWM slice # of a pin */
    uint SLICE_NUM = pwm_gpio_to_slice_num(pin);
    uint CHANN_NUM = pwm_gpio_to_channel(pin);
    
    pwm_set_clkdiv(SLICE_NUM,
                (float)( clock_get_hz(clk_sys) / (PWM_OUTFMOD * WRAP_)) );

    pwm_set_wrap(SLICE_NUM, WRAP_);
    pwm_set_enabled(SLICE_NUM, true);
    
    struct pwmstruct *ptr=(struct pwmstruct *)mkstruct();
    (*ptr).PIN=pin;
    (*ptr).SLICE=SLICE_NUM;
    (*ptr).CHANN=CHANN_NUM;
    
    return ptr;
}

void setcommpwm(uint level, 
                struct pwmstruct *d1_struct, 
                struct pwmstruct *d2_struct, 
                struct pwmstruct *d3_struct)
{
    pwm_set_chan_level( (*d1_struct).SLICE, (*d1_struct).CHANN, level);
    pwm_set_chan_level( (*d2_struct).SLICE, (*d2_struct).CHANN, level);
    pwm_set_chan_level( (*d3_struct).SLICE, (*d3_struct).CHANN, level);
}

struct pwmstruct *mkstruct(void)
{
    //malloc the struct
    struct pwmstruct *ptr;
    ptr=(struct pwmstruct *)malloc( sizeof(struct pwmstruct) );
    if ( ptr == NULL )
        printf("\nMALLOC ERROR!! >%d\n", __LINE__);

    return ptr;
}

// initialize control buttons
int initctb(const uint CTL_UP, const uint CTL_DN) // UP/DOWN
{
    int     ERR=0;

    gpio_init(CTL_UP);
    gpio_init(CTL_UP);

    // set pull up on butt pin
    gpio_pull_up(CTL_UP);
    gpio_pull_up(CTL_DN);
    if ( !gpio_is_pulled_up(CTL_UP) || !gpio_is_pulled_up(CTL_DN) )
    {
        printf("\nCTL BUTTS ARE NOT PULL UP!\n");
        ERR=1;
    }
    
     
    gpio_set_dir(CTL_UP, GPIO_IN);
    gpio_set_dir(CTL_DN, GPIO_IN);
    
    return ERR;
}


int initf(const uint SW_1, const uint D1)
{
    int     ERR=0;

    // serial init
    stdio_init_all();

    // LED init 
    gpio_init(SW_1);

    // set pull up on butt pin
    gpio_pull_up(SW_1);
    if ( !gpio_is_pulled_up(SW_1) )
    {
        printf("\nBUTT PIN IS NOT PULL UP!\n");
        ERR=1;
    }
        
    gpio_set_dir(SW_1, GPIO_IN);

    return ERR;
}

