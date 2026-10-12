#ifndef CLOCK_CALENDAR_H
#define CLOCK_CALENDAR_H

#include <stdint.h>
#include "timer1_timebase.h"

#define CLOCK_CALENDAR_CLEAN_INTERVAL_SECONDS 7200UL
#define CLOCK_CALENDAR_ADJUSTMENT_ACTIVE 0x01
#define CLOCK_CALENDAR_DISPLAY_CHANGED 0x02
#define CLOCK_CALENDAR_CLEAN_DUE 0x04
#define CLOCK_CALENDAR_SECOND_ELAPSED 0x08

typedef enum
{
    CLOCK_FIELD_MINUTE = 0,
    CLOCK_FIELD_HOUR,
    CLOCK_FIELD_DAY,
    CLOCK_FIELD_MONTH
} clock_field_t;

typedef struct
{
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
} clock_calendar_time_t;

static const uint8_t clock_calendar_days_per_month[12] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static clock_calendar_time_t clock_calendar_time;
static uint8_t clock_calendar_selected_field;
static uint8_t clock_calendar_last_second_bucket;
static uint16_t clock_calendar_seconds_since_clean;
static uint8_t clock_calendar_flags;

void clock_calendar_increment_selected(void);

static uint8_t clock_calendar_read_second_bucket(void)
{
    return (uint8_t)(timer1_timebase_read_ticks() >> 13);
}

static uint8_t clock_calendar_days_in_month(uint8_t month)
{
    return clock_calendar_days_per_month[month - 1];
}

static void clock_calendar_advance_one_second(void)
{
    clock_calendar_time.minute++;
    clock_calendar_flags |= CLOCK_CALENDAR_DISPLAY_CHANGED;
    if (clock_calendar_time.minute < 60)
    {
        return;
    }

    clock_calendar_time.minute = 0;
    clock_calendar_time.hour++;
    if (clock_calendar_time.hour < 24)
    {
        return;
    }

    clock_calendar_time.hour = 0;
    clock_calendar_time.day++;
    if (clock_calendar_time.day <= clock_calendar_days_in_month(clock_calendar_time.month))
    {
        return;
    }

    clock_calendar_time.day = 1;
    clock_calendar_time.month++;
    if (clock_calendar_time.month > 12)
    {
        clock_calendar_time.month = 1;
    }
}

void clock_calendar_init(void)
{
    clock_calendar_time.month = 1;
    clock_calendar_time.day = 1;
    clock_calendar_time.hour = 0;
    clock_calendar_time.minute = 0;
    clock_calendar_selected_field = CLOCK_FIELD_MINUTE;
    clock_calendar_last_second_bucket = clock_calendar_read_second_bucket();
    clock_calendar_seconds_since_clean = 0;
    clock_calendar_flags = CLOCK_CALENDAR_DISPLAY_CHANGED;
}

void clock_calendar_service(void)
{
    uint8_t now = clock_calendar_read_second_bucket();
    uint8_t elapsed_seconds = (uint8_t)((now - clock_calendar_last_second_bucket) & 0x07);
    clock_calendar_last_second_bucket = now;

    while (elapsed_seconds != 0)
    {
        elapsed_seconds--;
        clock_calendar_flags |= CLOCK_CALENDAR_SECOND_ELAPSED;

        clock_calendar_seconds_since_clean++;
        if (clock_calendar_seconds_since_clean >= CLOCK_CALENDAR_CLEAN_INTERVAL_SECONDS)
        {
            clock_calendar_seconds_since_clean = 0;
            clock_calendar_flags |= CLOCK_CALENDAR_CLEAN_DUE;
        }

        if (clock_calendar_flags & CLOCK_CALENDAR_ADJUSTMENT_ACTIVE)
        {
            clock_calendar_increment_selected();
        }
        else
        {
            clock_calendar_advance_one_second();
        }
    }
}

void clock_calendar_set_adjustment_active(uint8_t active)
{
    uint8_t was_active = (clock_calendar_flags & CLOCK_CALENDAR_ADJUSTMENT_ACTIVE) != 0;
    if (was_active == active)
    {
        return;
    }

    if (active)
    {
        clock_calendar_flags |= CLOCK_CALENDAR_ADJUSTMENT_ACTIVE;
    }
    else
    {
        clock_calendar_flags &= (uint8_t)~CLOCK_CALENDAR_ADJUSTMENT_ACTIVE;
    }

    clock_calendar_last_second_bucket = clock_calendar_read_second_bucket();
}

uint8_t clock_calendar_is_adjusting(void)
{
    return (clock_calendar_flags & CLOCK_CALENDAR_ADJUSTMENT_ACTIVE) != 0;
}

void clock_calendar_select_next_field(void)
{
    clock_calendar_selected_field++;
    if (clock_calendar_selected_field > CLOCK_FIELD_MONTH)
    {
        clock_calendar_selected_field = CLOCK_FIELD_MINUTE;
    }

    clock_calendar_flags |= CLOCK_CALENDAR_DISPLAY_CHANGED;
}

void clock_calendar_increment_selected(void)
{
    switch (clock_calendar_selected_field)
    {
        case CLOCK_FIELD_MINUTE:
            clock_calendar_time.minute++;
            if (clock_calendar_time.minute >= 60)
            {
                clock_calendar_time.minute = 0;
            }
            break;

        case CLOCK_FIELD_HOUR:
            clock_calendar_time.hour++;
            if (clock_calendar_time.hour >= 24)
            {
                clock_calendar_time.hour = 0;
            }
            break;

        case CLOCK_FIELD_DAY:
            clock_calendar_time.day++;
            if (clock_calendar_time.day > clock_calendar_days_in_month(clock_calendar_time.month))
            {
                clock_calendar_time.day = 1;
            }
            break;

        case CLOCK_FIELD_MONTH:
            clock_calendar_time.month++;
            if (clock_calendar_time.month > 12)
            {
                clock_calendar_time.month = 1;
            }
            if (clock_calendar_time.day > clock_calendar_days_in_month(clock_calendar_time.month))
            {
                clock_calendar_time.day = clock_calendar_days_in_month(clock_calendar_time.month);
            }
            break;
    }

    clock_calendar_last_second_bucket = clock_calendar_read_second_bucket();
    clock_calendar_flags |= CLOCK_CALENDAR_DISPLAY_CHANGED;
}

clock_calendar_time_t clock_calendar_get_time(void)
{
    return clock_calendar_time;
}

clock_field_t clock_calendar_get_selected_field(void)
{
    return (clock_field_t)clock_calendar_selected_field;
}

uint8_t clock_calendar_take_display_changed(void)
{
    uint8_t changed = (clock_calendar_flags & CLOCK_CALENDAR_DISPLAY_CHANGED) != 0;
    clock_calendar_flags &= (uint8_t)~CLOCK_CALENDAR_DISPLAY_CHANGED;
    return changed;
}

uint8_t clock_calendar_take_clean_due(void)
{
    uint8_t due = (clock_calendar_flags & CLOCK_CALENDAR_CLEAN_DUE) != 0;
    clock_calendar_flags &= (uint8_t)~CLOCK_CALENDAR_CLEAN_DUE;
    return due;
}

uint8_t clock_calendar_take_second_elapsed(void)
{
    uint8_t elapsed = (clock_calendar_flags & CLOCK_CALENDAR_SECOND_ELAPSED) != 0;
    clock_calendar_flags &= (uint8_t)~CLOCK_CALENDAR_SECOND_ELAPSED;
    return elapsed;
}

#endif
