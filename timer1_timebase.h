#ifndef TIMER1_TIMEBASE_H
#define TIMER1_TIMEBASE_H

#include <stdint.h>

#define TIMER1_TICKS_PER_SECOND 8192UL

void timer1_timebase_init(void)
{
    T1CON = 0x00;
    TMR1H = 0x00;
    TMR1L = 0x00;
    TMR1IF = 0;
    TMR1IE = 0;
    T1CON = 0x01;
}

uint16_t timer1_timebase_read_ticks(void)
{
    uint8_t high;
    uint8_t low;
    uint8_t verify;

    do
    {
        high = TMR1H;
        low = TMR1L;
        verify = TMR1H;
    } while (high != verify);

    return ((uint16_t)high << 8) | low;
}

#endif
