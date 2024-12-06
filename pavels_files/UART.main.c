#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define MAX_TRIES 5
#define BAUD_RATE 9600 
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define UART_INST uart1
#define STRLEN 64

#define BUTT_SW1 9

void initf ();
int recvstr (char *recv);   // recover string
void exthex (char *ptr_str, char *ptr_dest);  // extract hex

int main() 
{
    const uint8_t   send[] = "AT+ID=DevEui\n";

    char            data[STRLEN] = "";
    char            recv[STRLEN] = "";
    int             state = 0;
    int             com_try=0;


    enum            com_states { STANDBY, COMM_MOD, MOD_FAIL };
    
    initf ();

    printf("UART INSTANCE: %b\n", uart_is_enabled (UART_INST));

    while(1) 
    {
        switch (state)
        {
            case STANDBY:

                if(!gpio_get(BUTT_SW1)) 
                {
                    while(!gpio_get(BUTT_SW1)) {  sleep_ms(50); }
                    state=COMM_MOD;
                }
                break;

            case COMM_MOD:

                com_try=0;
                while ( com_try++ < MAX_TRIES)
                {
                    printf("retrying\n");
                    uart_write_blocking(UART_INST, send, strlen(send));
                    uart_puts(UART_INST, send);
                    if ( !recvstr(recv) ) // on succesful transmission
                        break;
                }
                if ( com_try >= MAX_TRIES )
                {
                    state=MOD_FAIL;
                    break;
                }

                exthex (recv, data);
                printf("Connected to LoRa\n");
                printf("HEX: %s\n", data);
                state=STANDBY;

                break;

            case MOD_FAIL:
                printf("Module stopped responding.\n");
                state=STANDBY;
                break;

            default:
        }
    }    
}

void exthex(char *ptr_str, char *buf)
{
    //char    buf[STRLEN]= "";
    strcpy(buf, "0x"); // add 
    
    int     i=0; // this will iterate over buf above

    int     pos=0; // this will iterate over ptr_str
    int     read_payload=0;

    while ( ptr_str[pos] != 0 ) // safeguard, because it can go beyond \0
    {
        // string looks like: <...>,[0x20]00:00:00:00:00:00:00:00[0x00]
        if ( ptr_str[pos] == ',' && ptr_str[pos+1] == ' ' )
        {
            pos+=2;
            read_payload=1;
        }
        
        // read only meaningful hex symbols 00 00 00<.> and skip ':'
        if ( read_payload && ptr_str[pos] != ':' ) 
        {
            buf[i]=ptr_str[pos];
            buf[i]=tolower(buf[i]);
            i++;
        }

        pos++;
    }
    //printf("HEX BUF>>%s<<", buf);
}

int recvstr (char *recv)
{
    //char        recv[STRLEN] = "";
    int         c   = 0;
    int         pos = 0;
    int         stat= 1; // 1 if couldnt read, and 0 if read is successful

    //while ( uart_is_readable (UART_INST) ) 
    while ( uart_is_readable_within_us (UART_INST, 500000) ) // 500 ms
    {
        c = uart_getc(UART_INST) & 0xFF;

        if (c == '\r' || c == '\n') // check if CR/LF 
        {
            recv[pos] = '\0';       // if CR/LF, then set \0
            //pos  = 0;
            stat = 0; // set stat to success
        }
        else 
        {
            if(pos < STRLEN - 1) 
            {
                recv[pos++] = c; // herra jestas 
            }
        }
    }
    //printf(">>%s<< [%d STAT .%02d|%c$%x] DEBUG: Recovered message\n", recv, stat, pos, c, c);
    return stat;
}

void initf ()
{
    stdio_init_all();
    printf("Start of main before GPIO\n");

    gpio_init(BUTT_SW1);
    gpio_set_dir(BUTT_SW1, 0);
    gpio_pull_up(BUTT_SW1);
    
    // set TX and RX pins 
    //gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_INST, 0));
    //gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_INST, 1));
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // set the bitrate of uart
    // (!) uart0 and uart1 exist already
    uart_init(UART_INST, BAUD_RATE);

    //uart_set_fifo_enabled (UART_INST, 1);
    uart_set_format (UART_INST, 8, 1, UART_PARITY_NONE);

    // dont pause debugger with timers
    timer_hw->dbgpause = 0;
}

