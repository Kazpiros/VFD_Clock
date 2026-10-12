#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <stdint.h>
#include "clock_calendar.h"

#define LED_INDICATOR_GP0_MASK (1 << 0)

void led_indicator_init(void)
{
    GP0 = 0;
}

void led_indicator_service(void)
{
    if (clock_calendar_take_second_elapsed())
    {
        GP0 = !GP0;
    }
}

#endif
