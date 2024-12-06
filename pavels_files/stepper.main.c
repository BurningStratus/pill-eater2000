#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"

#define SHMAX 64
#define PS1 "sh-dev# "

void initf ();
void shhandler ();
void nullstr (char *ptr_str);
void spin ();

//  v CLI commands v
void echo (const char *arg);    // print arg(only one)
void calib ();          // calibrate
void run (uint amt);    // spin
void step (int amt);   // step
void nullf ();          // if cmd was unrecognized
void motstat ();        // print info
void config_sleep (uint arg);   // set the sleep between stators
//  ^ CLI commands ^

static uint     MOT_LN4 = 2;    // motor line 4
static uint     MOT_LN3 = 3;    // motor line 3
static uint     MOT_LN2 = 6;    // motor line 2
static uint     MOT_LN1 = 13;   // motor line 1
static uint     OPT_FORK= 28;   // optical fork

static uint     mot_cal = 0;    // calibrated or not
static uint     mot_stepmax =0;

static uint     mot_step    =0; // current step. 0 - 4096
static uint32_t mot_stat    =3; // bitmask of stators

static uint     sleeptime_ms=0;     // the delay between stator engagement
static uint     stepseq[] = { 1, 3, 2, 6, 4, 12, 8, 9 };

int main() 
{
    stdio_init_all();
    initf();

    // dont pause debugger with timers
    timer_hw->dbgpause = 0;
    
    while (1)
    {
        shhandler();
    }
}

void initf ()
{
    gpio_init(MOT_LN4);
    gpio_init(MOT_LN3);
    gpio_init(MOT_LN2);
    gpio_init(MOT_LN1);
    gpio_init(OPT_FORK);
    
    gpio_set_dir(MOT_LN4, GPIO_OUT);
    gpio_set_dir(MOT_LN3, GPIO_OUT);
    gpio_set_dir(MOT_LN2, GPIO_OUT);
    gpio_set_dir(MOT_LN1, GPIO_OUT);
    
    gpio_set_dir(OPT_FORK, GPIO_IN);
    gpio_pull_up(OPT_FORK);
}

void nullstr(char *ptr_str)
{
    for (int i=0; i<SHMAX; i++)
        ptr_str[i] = '\0'; // null the string
}

void shhandler()
{
    char    cmd[SHMAX];
    char    cmd_args[SHMAX];
    char    inputbuf[SHMAX];

    nullstr(inputbuf);    // clear the string
    nullstr(cmd);    // clear the string
    nullstr(cmd_args);    // clear the string

    printf("\n%s", PS1);
    scanf(" %[^\r]", inputbuf);

    // command parser
    int i=0;
    int j=0;
    while (inputbuf[i] != ' ' && inputbuf[i] != 0)
    {
        cmd[i]=inputbuf[i];       // copy the "command" part to a string
        i++;
    }

    i++;
    while (inputbuf[i] != 0)
    {
        cmd_args[j]=inputbuf[i];  // get argv
        i++;
        j++;
    }

    printf("in:%s. cmd:%s. narg:%s.\n", inputbuf, cmd, cmd_args);
    
    /* execute the appropriate command */
    if ( !strcmp( "echo", cmd) ) {
        echo (cmd_args); 
    } else if ( !strcmp( "calib", cmd) ) {
        calib ();
    } else if ( !strcmp( "stat", cmd) ) {
        motstat ();
    } else if ( !strcmp( "run", cmd) ) {
        int rev=0;
        sscanf(cmd_args, "%d", &rev);
        run (rev);
    } else if ( !strcmp( "step", cmd) ) {
        int stepamt=0;
        sscanf(cmd_args, "%d", &stepamt);
        step (stepamt);
    } else if ( !strcmp( "config_sleep", cmd) ) {
        int cfg_ms=0;
        sscanf(cmd_args, "%d", &cfg_ms);
        config_sleep (cfg_ms);
    /* if command was not found, then bail */
    } else {
        nullf ();
    }

}

/* CLI progs */
void spin ()
{
    mot_step++;
    if (mot_step >= mot_stepmax && mot_stepmax != 0 )
        mot_step=0;

    mot_stat= stepseq[ mot_step % ( sizeof(stepseq)/sizeof(stepseq[0])) ];

    printf("\r0x%02x b%05b 10_%02d stp:%04d", 
            mot_stat, mot_stat, mot_stat, mot_step);
    stdio_flush();

    sleep_ms(sleeptime_ms);

    gpio_put(MOT_LN4, (1 & mot_stat ) != 0);
    gpio_put(MOT_LN3, (2 & mot_stat ) != 0);
    gpio_put(MOT_LN2, (4 & mot_stat ) != 0);
    gpio_put(MOT_LN1, (8 & mot_stat ) != 0);
}

inline void nullf ()                { printf("Unrecognized command."); }
inline void config_sleep (uint arg) { sleeptime_ms=arg; }
inline void echo (const char *arg)  { printf("echo: %s", arg); }

void motstat () {
    const char *stat_str = (mot_stepmax == 0) ? "FALSE" : "TRUE"; // perkele

    printf("STATS:\n");
    printf("REAL STEPS: %d\nSTEP: %d\nCALIBRATED: %s\nSLEEP TIME: %d\n",
        mot_stepmax,
        mot_step,
        stat_str,
        sleeptime_ms
    );
}

void calib ()
{
    uint step_sum=0;

    // find the fork
    do { spin (); }
    while ( !gpio_get (OPT_FORK) );
    do { spin(); }
    while ( gpio_get (OPT_FORK) );

    // start calibration
    for (int i=0; i<3; i++)
    {
        do { 
            spin();
            step_sum++;
        } while ( !gpio_get( OPT_FORK ) );

        /* fork activates on multiple steps
           so we need to spin until fork is gone */
        do { 
            spin(); 
            step_sum++;
        } while ( gpio_get ( OPT_FORK ) );
    }
    /*
     * STEP 170 - 172 - 175 will set the wheel into the right spot
    */

    putchar('\n');
    printf("REAL STEPS: %d\n", step_sum/3);
    mot_stepmax=step_sum/3;
    mot_step=0;
    step (170);
}

void run (uint amt) {
    if (amt == 0)
        amt = 8;
    if (mot_stepmax == 0)
    {
        printf("(!) MOTOR IS NOT CALIBRATED.\nSPINNING UNTIL SENSOR\n");

        do { spin (); }
        while ( !gpio_get (OPT_FORK) );

        do { spin(); }
        while ( gpio_get (OPT_FORK) );

        return;
    }
        
    uint    tgsection = amt * (mot_stepmax/8); // target section
    printf("%d %f", tgsection);
    uint    tgmot_step=mot_step;    // target motor step

    int i=0;
    while ( i != tgsection )
    {
        spin();
        i++;
    }
}

void step (int amt)
{
    for (int i=0; i<amt; i++)
        spin();
}

