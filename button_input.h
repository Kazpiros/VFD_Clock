#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdint.h>
#include "timer1_timebase.h"

#define BUTTON_INPUT_GP2_MASK (1 << 2)
#define BUTTON_INPUT_DEBOUNCE_MS 25UL
#define BUTTON_INPUT_LONG_HOLD_MS 2000UL
#define BUTTON_INPUT_DEBOUNCE_TICKS \
    ((TIMER1_TICKS_PER_SECOND * BUTTON_INPUT_DEBOUNCE_MS) / 1000UL)
#define BUTTON_INPUT_LONG_HOLD_TICKS \
    ((TIMER1_TICKS_PER_SECOND * BUTTON_INPUT_LONG_HOLD_MS) / 1000UL)

#define BUTTON_INPUT_EDGE_PENDING 0x01
#define BUTTON_INPUT_CANDIDATE_PRESSED 0x02
#define BUTTON_INPUT_STABLE_PRESSED 0x04
#define BUTTON_INPUT_LONG_HOLD_REPORTED 0x08
#define BUTTON_INPUT_RELEASE_EVENT_PENDING 0x10
#define BUTTON_INPUT_LONG_HOLD_EVENT_PENDING 0x20

static volatile uint8_t button_input_flags;
static uint8_t button_input_candidate_since;
static uint8_t button_input_press_started_at;

void button_input_init(void)
{
    uint16_t timer_ticks;

    WPU2 = 1;
    OPTION_REGbits.nGPPU = 0;

    uint8_t initial_pressed = (GP2 == 0);
    uint8_t gpio_snapshot = GPIO;
    (void)gpio_snapshot;

    button_input_flags = 0;
    if (initial_pressed)
    {
        button_input_flags |= BUTTON_INPUT_CANDIDATE_PRESSED | BUTTON_INPUT_STABLE_PRESSED;
    }
    timer_ticks = timer1_timebase_read_ticks();
    button_input_candidate_since = (uint8_t)timer_ticks;
    button_input_press_started_at = (uint8_t)(timer_ticks >> 8);
    IOC = BUTTON_INPUT_GP2_MASK;
    GPIF = 0;
    GPIE = 1;
}

void button_input_handle_gpio_change_interrupt(void)
{
    button_input_flags |= BUTTON_INPUT_EDGE_PENDING;
}

void button_input_suspend_gpio_interrupt(void)
{
    GPIE = 0;
}

void button_input_resume_gpio_interrupt(void)
{
    GPIE = 1;
}

void button_input_service(void)
{
    uint8_t sampled_pressed = (GP2 == 0);
    uint16_t timer_ticks = timer1_timebase_read_ticks();
    uint8_t now_low = (uint8_t)timer_ticks;
    uint8_t now_high = (uint8_t)(timer_ticks >> 8);
    uint8_t candidate_pressed = (button_input_flags & BUTTON_INPUT_CANDIDATE_PRESSED) != 0;
    uint8_t stable_pressed = (button_input_flags & BUTTON_INPUT_STABLE_PRESSED) != 0;
    uint8_t edge_pending = (button_input_flags & BUTTON_INPUT_EDGE_PENDING) != 0;

    button_input_flags &= (uint8_t)~BUTTON_INPUT_EDGE_PENDING;

    if ((sampled_pressed != candidate_pressed) || edge_pending)
    {
        candidate_pressed = sampled_pressed;
        button_input_candidate_since = now_low;
        if (candidate_pressed)
        {
            button_input_flags |= BUTTON_INPUT_CANDIDATE_PRESSED;
        }
        else
        {
            button_input_flags &= (uint8_t)~BUTTON_INPUT_CANDIDATE_PRESSED;
        }
    }

    if ((candidate_pressed != stable_pressed) &&
        ((uint8_t)(now_low - button_input_candidate_since) >= BUTTON_INPUT_DEBOUNCE_TICKS))
    {
        stable_pressed = candidate_pressed;
        if (stable_pressed)
        {
            button_input_flags |= BUTTON_INPUT_STABLE_PRESSED;
        }
        else
        {
            button_input_flags &= (uint8_t)~BUTTON_INPUT_STABLE_PRESSED;
        }

        if (stable_pressed)
        {
            button_input_press_started_at = now_high;
            button_input_flags &= (uint8_t)~BUTTON_INPUT_LONG_HOLD_REPORTED;
        }
        else
        {
            button_input_flags |= BUTTON_INPUT_RELEASE_EVENT_PENDING;
        }
    }

    if (stable_pressed && !(button_input_flags & BUTTON_INPUT_LONG_HOLD_REPORTED) &&
        ((uint8_t)(now_high - button_input_press_started_at) >=
         (BUTTON_INPUT_LONG_HOLD_TICKS / 256U)))
    {
        button_input_flags |= BUTTON_INPUT_LONG_HOLD_REPORTED | BUTTON_INPUT_LONG_HOLD_EVENT_PENDING;
    }
}

uint8_t button_input_take_release_event(void)
{
    uint8_t pending = (button_input_flags & BUTTON_INPUT_RELEASE_EVENT_PENDING) != 0;
    button_input_flags &= (uint8_t)~BUTTON_INPUT_RELEASE_EVENT_PENDING;
    return pending;
}

uint8_t button_input_take_long_hold_event(void)
{
    uint8_t pending = (button_input_flags & BUTTON_INPUT_LONG_HOLD_EVENT_PENDING) != 0;
    button_input_flags &= (uint8_t)~BUTTON_INPUT_LONG_HOLD_EVENT_PENDING;
    return pending;
}

#endif
