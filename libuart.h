
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


int 
call_uart_inner (char *dst_ptr)
{
/*
 * Send arbitrary command to Lora && write response to a destination string. 
 * Return 0 on success
 */
    int         rslen = 0;
    uint64_t    sttime_us = time_us_64();

    while ( (time_us_64()-sttime_us) < (TIMEOUT_MS * 1000) ) 
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
uart_cmd (const char *cmd, char *dst_str)
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
        printf("Sent command: %s\n", cmd);

        if ( !call_uart_inner (dst_str)) 
        { 
            printf("Received response: %s\n", dst_str);
            return 0; // Success
        } else {
            attempt++;
            printf("No response received (attempt %d)\n", attempt);
        }
    }

    // a lil bit of error handling
    #ifdef __PILL_ERROR_MACROS__
        if (ERROR_CODE == 0) {
            ERROR_CODE=UART_TIMEOUT;
        } else {
            printf("More than one error:\n");
            printerr(__FUNCTION__); 
        }
    #endif

    return 1; // Failure
}

//============================================================================
// wrappers to not mess with func naming

int 
sendATCommand(const char *command, char *response)
{
    return !uart_cmd(command, response);
    // inverted to not break the programs logic and follow conventions
    // 0 == no errors(SUCCESS)
    // 1 == errors occured(UNSPECIFIED ERROR)
}
    
//============================================================================
#endif
