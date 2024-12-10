
#include "libcommon.h"
/*
    Set errors and print those for debugging purposes.
*/
#ifndef __PILL_ERROR_MACROS__
#define __PILL_ERROR_MACROS__

/*
    define error codes and their names here. 
    For example, SUCCESS's index is 0, so SUCCESS's code will be 0.
*/
enum pillerrlist { SUCCESS, FAIL_GEN, UART_TIMEOUT, WRITEPG };

// global variable to see the error from anywhere.
static unsigned int ERROR_CODE=0; 

void printerr(const char *offend_func, int offend_line)
{
    /*
    * Print the error set in the variable ERROR_CODE defined in seterr.h
    */
    printf("LAST ERROR: ");
    switch (ERROR_CODE)
    {
        case (SUCCESS):
            printf("0: GENERIC SUCCESS");
            break;
        case (FAIL_GEN):
            printf("1: LAZY FAILURE");
            break;
        case (UART_TIMEOUT):
            printf("2: UART TIMEOUT");
            break;
        case (WRITEPG):
            printf("3: FAILED WRITING DATA TO EEPROM");
            break;
        default:
            printf("UNSPECIFIED ERROR");
/* template
        case ():
            printf("");
            break;
*/
    }
    if (*offend_func)
        printf(":: ERROR IN FUNC:[%s] ", offend_func);
    if (offend_line > 0)
        printf(":: LINE [%d] ", offend_line);
    printf("\n");
}

#endif
