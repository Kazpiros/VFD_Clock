#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <stdint.h>
#include "clock_calendar.h"

#define DISPLAY_COLUMNS 16U
#define CLOCK_DISPLAY_TEXT_LENGTH 13U
#define CLOCK_DISPLAY_CLEAN_COMMAND '\f'
#define CLOCK_DISPLAY_CARRIAGE_RETURN '\r'
#define CLOCK_DISPLAY_AUTOMATIC_RETURN 0x11
#define CLOCK_DISPLAY_CURSOR_INVISIBLE 0x14

#if DISPLAY_COLUMNS < CLOCK_DISPLAY_TEXT_LENGTH
#error DISPLAY_COLUMNS must be at least 13 for "MM, DD, HH:MM"
#endif
#if DISPLAY_COLUMNS > 255
#error DISPLAY_COLUMNS must fit in an 8-bit column index
#endif

#define CLOCK_DISPLAY_MAX_SHIFT (DISPLAY_COLUMNS - CLOCK_DISPLAY_TEXT_LENGTH)

static uint8_t clock_display_horizontal_shift;

static char clock_display_digit(uint8_t value, uint8_t tens)
{
    uint8_t decimal_tens = 0;

    while (value >= 10)
    {
        value -= 10;
        decimal_tens++;
    }

    return (char)('0' + ((tens != 0) ? decimal_tens : value));
}

static char clock_display_text_character(
    clock_calendar_time_t time,
    clock_field_t selected_field,
    uint8_t character_index,
    uint8_t blink_visible)
{
    if (!blink_visible &&
        (((selected_field == CLOCK_FIELD_MONTH) && (character_index <= 1)) ||
         ((selected_field == CLOCK_FIELD_DAY) && (character_index >= 4) && (character_index <= 5)) ||
         ((selected_field == CLOCK_FIELD_HOUR) && (character_index >= 8) && (character_index <= 9)) ||
         ((selected_field == CLOCK_FIELD_MINUTE) && (character_index >= 11) && (character_index <= 12))))
    {
        return ' ';
    }

    switch (character_index)
    {
        case 0: return clock_display_digit(time.month, 1);
        case 1: return clock_display_digit(time.month, 0);
        case 2: return ',';
        case 3: return ' ';
        case 4: return clock_display_digit(time.day, 1);
        case 5: return clock_display_digit(time.day, 0);
        case 6: return ',';
        case 7: return ' ';
        case 8: return clock_display_digit(time.hour, 1);
        case 9: return clock_display_digit(time.hour, 0);
        case 10: return ':';
        case 11: return clock_display_digit(time.minute, 1);
        case 12: return clock_display_digit(time.minute, 0);
        default: return ' ';
    }
}

void clock_display_render(
    clock_calendar_time_t time,
    clock_field_t selected_field,
    uint8_t blink_visible,
    uint8_t cleaning_sequence,
    void (*write_character)(char))
{
    uint8_t column;

    if (cleaning_sequence)
    {
        if (clock_display_horizontal_shift < CLOCK_DISPLAY_MAX_SHIFT)
        {
            clock_display_horizontal_shift++;
        }
        else
        {
            clock_display_horizontal_shift = 0;
        }

        write_character(CLOCK_DISPLAY_CLEAN_COMMAND);
        write_character((char)CLOCK_DISPLAY_AUTOMATIC_RETURN);
        write_character((char)CLOCK_DISPLAY_CURSOR_INVISIBLE);
    }
    else
    {
        write_character(CLOCK_DISPLAY_CARRIAGE_RETURN);
    }

    for (column = 0; column < DISPLAY_COLUMNS; column++)
    {
        if (column < clock_display_horizontal_shift)
        {
            write_character(' ');
        }
        else if ((column - clock_display_horizontal_shift) < CLOCK_DISPLAY_TEXT_LENGTH)
        {
            write_character(clock_display_text_character(
                time,
                selected_field,
                (uint8_t)(column - clock_display_horizontal_shift),
                blink_visible));
        }
        else
        {
            write_character(' ');
        }
    }
}

#endif
