
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
enum pillerrlist { SUCCESS, FAIL_GEN, UART_TIMEOUT };

// global variable to see the error from anywhere.
static unsigned int ERROR_CODE=0; 

void printerr(const char *offend_func)
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
            printf("2: UART_TIMEOUT");
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
        printf(":: ERROR IN FUNC:[%s]\n", offend_func);
}

#endif
