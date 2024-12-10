/*
    Functions to work with ROM.
*/

#ifndef __PILL_ROM__
#define __PILL_ROM__

#define SDA_LINE 16
#define SCL_LINE 17
#define I2C_ADDR 0x50

#include <stdlib.h>

#include "hardware/i2c.h"
#include "pico/malloc.h"
#include "pico/stdlib.h"

#include "libcommon.h"


void initrom ()
{
    i2c_init (i2c0, 100*1000);

    gpio_set_function(SDA_LINE, GPIO_FUNC_I2C);
    gpio_set_function(SCL_LINE, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_LINE);
    gpio_pull_up(SCL_LINE);
}

uint8_t readb (uint16_t rom_addr)
{
/* 
 *  read byte and return as uint8_t
*/
    uint8_t     dst_str;
    uint8_t     address[2];

    address[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    address[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits
    
    i2c_write_blocking( i2c0, I2C_ADDR, address, 2, 1 );
    sleep_ms(5);
    i2c_read_blocking ( i2c0, I2C_ADDR, &dst_str, 1, 0 );
    return dst_str; 
}

int writepg (uint16_t rom_addr, const uint8_t *payload, int payload_size)
{
    int         stat=0;
    uint8_t     packet[2+payload_size];

    packet[0] = 0xFF; // placeholder for address (hi)
    packet[1] = 0xFF; // placeholder for address (lo)

    int ii=0;
    //strcat(packet, payload); 
    for (int i=2; i<2+payload_size; i++)
    {
        packet[i]=payload[ii];
        ii++;
    }

    packet[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    packet[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits

    stat=i2c_write_blocking( i2c0, I2C_ADDR, packet, payload_size, 0 ); 
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
        //printf("COULDN'T ALLOCATE MEMORY!");
        return (void *)NULL;
    }
    
    address[0] = ((rom_addr & 0xFF00) >> 8);     // extract hi bits
    address[1] = (uint8_t)(rom_addr & 0x00FF);   // extract lo bits
    
    i2c_write_blocking( i2c0, I2C_ADDR, address, 2, 1 );
    sleep_ms(5);
    i2c_read_blocking ( i2c0, I2C_ADDR, dst_str, wcnt, 0 ); // last
    return dst_str;

//printf("seqread: >%s<\n", (char *)dst_str);
/* ASCII DUMP
if ( (char)dst_str >= 33 && (char)dst_str <= 126) 
    printf("%c", (char)dst_str); //print out only printable chars
else 
    printf("."); // if special chars, print a dot

if (i % 80 == 0)
    printf("\n%d\t", i);
*/
}

#endif
