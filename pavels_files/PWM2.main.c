#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pico/util/queue.h"

#define PWM_DUTY_STEP 10
#define WRAP_ 999 
#define PWM_OUTFMOD 1000
#define DEB_TIME_MS 100


static const uint   ROT_SW  = 12;   /* pull up */
static const uint   ROT_A   = 10;   /* no pull */
static const uint   ROT_B   = 11;   /* no pull */

static const uint   D1      = 22;
static const uint   D2      = 21;
static const uint   D3      = 20;

static uint32_t     time_us;


bool LEDSTAT = false;

static uint         PWM_DUTY= 0;
static queue_t      swdtq;      // switch data queue
static uint         swdt_i=0;   // data from switch queue

static int          QPOS = 2;
static int          QNEG = 1;

int    QSTAT=0;


void initf (const uint MAIN_SW);
void mkpwm (const uint D_PIN);
void set3pwm (int pwm_duty, const uint P1, const uint P2, const uint P3);
int initctsw (const uint CTL_A, const uint CTL_B); // CONTROL UP/DOWN
void irq_handler ();


int main() 
{
    /*
    PWM LED AND ROTARY ENCODER ON INTERRUPTS
    */

    queue_init ( &swdtq, sizeof(int), 50);

    initf (ROT_SW);
    initctsw (ROT_A, ROT_B);

    mkpwm(D1);
    mkpwm(D2);
    mkpwm(D3);
    
    // Loop forever
    while (true) {
        sleep_ms (10);
        swdt_i=0; // must be reset on every iteration

        printf ("\rMAINSW: %d PWM: %03d DATA: %d", 
            gpio_get(ROT_SW), 
            PWM_DUTY,
            swdt_i );

        stdio_flush ();

        /* Change brightness only when ON. Clockwise == +, Counterclk == -.
         * 
        */

        if ( LEDSTAT ) {
            if ( PWM_DUTY == 0 )
            {
                if ( !gpio_get (12) ) 
                {
                    while ( !gpio_get (12) )
                        ;
                    PWM_DUTY = ((WRAP_+1)*50) / 100; // set to 50%
                    set3pwm(PWM_DUTY, D1, D2, D3);
                } 
            } 

            queue_try_remove ( &swdtq, &swdt_i ); // get data from queue
            // 2 == HI(+)
            if ( swdt_i == 2 )
            {
                if ( PWM_DUTY < (WRAP_+1) ) 
                    PWM_DUTY+=PWM_DUTY_STEP;

            // 1 == LO(-)
            } else if ( swdt_i == 1 ) {
                if ( PWM_DUTY > 0 )
                {
                    if ( PWM_DUTY < PWM_DUTY_STEP && PWM_DUTY > 0 )
                    {
                        PWM_DUTY--;
                    } else {
                        PWM_DUTY-=PWM_DUTY_STEP;
                    }
                }
            }
            set3pwm(PWM_DUTY, D1, D2, D3);

        } else if ( !LEDSTAT) {
            set3pwm(0, D1, D2, D3);
        }

        // clear the queue when its full
        if ( QSTAT == 1 )
        {
            int junk=0;
            while ( !queue_is_empty ( &swdtq ))
                queue_try_remove( &swdtq, &junk);
        }

        // check the knob button
        if ( !gpio_get (12) ) 
        {
            if ( time_us_32() - time_us > DEB_TIME_MS*1000 )
            {
                LEDSTAT=!LEDSTAT;
                time_us = time_us_32();
            }
                
            while ( !gpio_get (12) )
                ;
        } 

    }
}

void irq_handler()
{
    if ( queue_is_full (&swdtq) )
    {
        QSTAT=1;
        return;
    }
        
    
    // get data only when LEDs are ON.
    if ( LEDSTAT )
    {
        if ( !gpio_get (11) ) {
            queue_try_add ( &swdtq, &QPOS );
        } else if ( gpio_get (11) ) {
            queue_try_add ( &swdtq, &QNEG );
        } 
    }
}        

void mkpwm (const uint D_PIN)
{
    /* set pin to PWM mode  */
    gpio_set_function (D_PIN, GPIO_FUNC_PWM);

    /* get PWM slice # of a pin */
    uint D_PINSLICE_NUM = pwm_gpio_to_slice_num(D_PIN);
    uint D_PINCHANN_NUM = pwm_gpio_to_channel(D_PIN);
    
    pwm_set_clkdiv (D_PINSLICE_NUM,
                (float)( clock_get_hz(clk_sys) / (PWM_OUTFMOD * (WRAP_+1))) );

    pwm_set_wrap (D_PINSLICE_NUM, WRAP_);
    pwm_set_enabled (D_PINSLICE_NUM, true);
}

void set3pwm (int pwm_duty, const uint P1, const uint P2, const uint P3)
{
    pwm_set_chan_level( pwm_gpio_to_slice_num(P1), pwm_gpio_to_channel(P1), 
        pwm_duty);
    pwm_set_chan_level( pwm_gpio_to_slice_num(P2), pwm_gpio_to_channel(P2), 
        pwm_duty);
    pwm_set_chan_level( pwm_gpio_to_slice_num(P3), pwm_gpio_to_channel(P3), 
        pwm_duty);
}

int initctsw(const uint CTL_A, const uint CTL_B) // CONTROL UP/DOWN
{
    gpio_init(CTL_A);
    gpio_init(CTL_A);

    gpio_set_dir(CTL_A, GPIO_IN);
    gpio_set_dir(CTL_B, GPIO_IN);

    // GPIO_IRQ_EDGE_RISE == 0x8u
    gpio_set_irq_enabled_with_callback(CTL_A,0x8u,true,irq_handler );
    
}

void initf(const uint MAIN_SW)
{
    // serial init
    stdio_init_all();

    gpio_init(MAIN_SW);

    // set pull up on butt pin
    gpio_pull_up(MAIN_SW);
    if ( !gpio_is_pulled_up(MAIN_SW) )
        printf("\nBUTT PIN IS NOT PULL UP!\n");

    gpio_set_dir(MAIN_SW, GPIO_IN);
}

