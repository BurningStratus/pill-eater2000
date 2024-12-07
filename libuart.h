
/* Library to communicate with UART
 * 
 *
*/
#include "libcommon.h"

#ifndef __UARTMACROS__
#define __UARTMACROS__

#define UART_NR uart1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600

// wrappers to not mess with func naming
int 
readUARTResponse(char *response, int maxlen, int timeout_ms)
    return uart_recv ( response, maxlen, timeout_ms );

int 
sendATCommand(const char *command, char *response, int maxlen, int max_attempts)
    return uart_cmd(cmd, dst_str, maxlen, max_attempts);



int 
uart_recv (char *dst_ptr, int maxlen, int timeout_ms) 
{
    int         rslen = 0;
    uint64_t    sttime_us = time_us_64();

    while ( (time_us_64()-sttime_us) < (timeout_ms * 1000) ) 
    {
        if ( uart_is_readable(UART_NR) ) 
        {
            response[rslen++]=uart_getc(UART_NR) & 0xFF; 
            if (rslen >= maxlen - 1) // cut it if it goes beyond bufffer size 
                break; 
        }
    }
    if (rslen > 0) {
        response[rslen] = '\0'; // Null-terminate the response
        return 1;                      // Success
    }
    return 0; // Timeout or no data received
}

int 
uart_cmd (const char *cmd, char *dst_str, int maxlen, int max_attempts)
{
    int attempt = 0;

    while (attempt < max_attempts) 
    {
        uart_write_blocking(UART_NR, (const uint8_t *)cmd, strlen(cmd));
        printf("Sent command: %s\n", cmd);

        if ( readUARTResponse(dst_str, maxlen, 500 )) 
        { 
            // 500 ms timeout
            printf("Received response: %s\n", dst_str);
            return 1; // Success

        } else {
            printf("No response received (attempt %d)\n", attempt + 1);
        }
        attempt++;
    }
    return 0; // Failure
}

#endif
