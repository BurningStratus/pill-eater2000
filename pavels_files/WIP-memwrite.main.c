#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h" 
#include "pico/malloc.h"
#include "hardware/i2c.h"

#define SDA_LINE 16
#define SCL_LINE 17
#define I2C_ADDR 0x50

#define ARRSIZE(arr) ( sizeof(arr)/(sizeof(arr[0])) )

void
initf  ();

int 
writepg (uint16_t rom_addr, const uint8_t *payload);

uint8_t 
readb   (uint16_t rom_addr);

uint8_t*  
seqread (uint16_t rom_addr, int wcnt);


int main()
{
    initf();
    uint8_t STRING[64] = "TRANSMISSION 0xFAFAAA4322FF";
    writepg (0x0000, STRING );
    printf ( "%X\n", seqread(0x0000, 128) );
    
    for (int i=0; i<0xFFFF; i++)
    {
        char dst_str = (char)readb (i) ;
        if ( (char)dst_str >= 33 && (char)dst_str <= 126) 
            printf("%c", (char)dst_str); //print out only printable chars
        else 
            printf("."); // if special chars, print a dot

        if (i % 80 == 0)
            printf("\n%d\t", i);
    }
}

void initf ()
{
    stdio_init_all();

    i2c_init (i2c0, 100*1000);

    gpio_set_function(SDA_LINE, GPIO_FUNC_I2C);
    gpio_set_function(SCL_LINE, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_LINE);
    gpio_pull_up(SCL_LINE);
    
    timer_hw->dbgpause=0;
    printf ("\n\nDATA VVV\n");
}

uint8_t readb (uint16_t rom_addr)
{
    uint8_t     dst_str;
    uint8_t     address[2];

    address[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    address[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits
    
    i2c_write_blocking( i2c0, I2C_ADDR, address, 2, 1 );
    sleep_ms(5);
    i2c_read_blocking ( i2c0, I2C_ADDR, &dst_str, 1, 0 );
    return dst_str; 
}

int writepg (uint16_t rom_addr, const uint8_t *payload)
{
    uint8_t     address[2];
    int         stat=0;

    address[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    address[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits

    i2c_write_blocking ( i2c0, I2C_ADDR, address, 2, 1 ); // start write
    stat=i2c_write_blocking ( i2c0, I2C_ADDR, payload, sizeof(payload), 0 );

    sleep_ms(5);
    return stat;
}

uint8_t* seqread (uint16_t rom_addr, int wcnt)
{
    //uint8_t     dst_str[wcnt];
    uint8_t     *dst_str;
    uint8_t     address[2];

    // dangerous game
    dst_str=(uint8_t *)calloc(wcnt, sizeof(uint8_t));
    if ( !dst_str )
    {
        printf("COULDN'T ALLOCATE MEMORY!");
        return (void *)NULL;
    }
    
    address[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    address[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits
    
    i2c_write_blocking( i2c0, I2C_ADDR, address, 2, 1 );
    sleep_ms(5);
    i2c_read_blocking ( i2c0, I2C_ADDR, dst_str, wcnt, 0 ); // last
    printf("seqread: >%s<\n", (char *)dst_str);

/* ASCII DUMP
if ( (char)dst_str >= 33 && (char)dst_str <= 126) 
    printf("%c", (char)dst_str); //print out only printable chars
else 
    printf("."); // if special chars, print a dot

if (i % 80 == 0)
    printf("\n%d\t", i);
*/

    return dst_str;
}
