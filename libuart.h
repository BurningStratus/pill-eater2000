
/* Library to communicate with UART
 * 
 *
*/

#include "libcommon.h"
#include "seterr.h"

#ifndef __PILL_UART_MACROS__
#define __PILL_UART_MACROS__

#define UART_NR uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define TIMEOUT_S 5


int 
call_uart_inner (char *dst_ptr, int timeout_s)
{
/*
 * Send arbitrary command to Lora && write response to a destination string. 
 * Return 0 on success
 */
    int         rslen = 0;
    uint64_t    sttime_us = time_us_64();

    while ( (time_us_64()-sttime_us) < (timeout_s * 1000 * 1000) ) 
    {
        if ( uart_is_readable(UART_NR) ) 
        {
            dst_ptr[rslen++]=uart_getc(UART_NR) & 0xFF;     // mask for 8 bit 
            if (rslen >= STRLEN - 1) // cut it if it goes beyond bufffer size 
                break;
        }
    }
    if (rslen > 0) {
        dst_ptr[rslen] = '\0'; // Null-terminate the response
        return 0;              // Success
    }

    return 1; // Timeout or no data received
}


int
uart_cmd (const char *cmd, char *dst_str, int timeout)
{
/* 
 * call UART with this function.
 *
 *
*/
    int     attempt = 0;
    
    // start communicating
    while (attempt < MAX_ATTEMPTS) 
    {
        uart_write_blocking(UART_NR, (const uint8_t *)cmd, strlen(cmd));
        printf("UART(SEND): %s", cmd);

        if ( !call_uart_inner (dst_str, timeout)) 
        { 
            //printf("UART: %s", dst_str);
            return 0; // Success
        } else {
            attempt++;
            //printf("UART: NULL(%d)", attempt);
        }
    }

    // a lil bit of error handling
    #ifdef __PILL_ERROR_MACROS__
        if (ERROR_CODE == 0) {
            ERROR_CODE=UART_TIMEOUT;
        } else {
            printf("More than one error:\n");
            printerr(__FUNCTION__, __LINE__); 
        }
    #endif

    return 1; // Failure
}

//============================================================================
// wrappers to not mess with func naming

int 
sendATCommand(const char *command, char *response, int timeout)
{
    if ( !timeout )
        return !uart_cmd(command, response, TIMEOUT_S);
    return !uart_cmd(command, response, timeout);
    
    // inverted to not break the programs logic and follow conventions
    // 0 == no errors(SUCCESS)
    // 1 == errors occured(UNSPECIFIED ERROR)
}
    
//============================================================================
#endif
