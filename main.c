/* -----------------------------------------------------------------------
 * PIC12F675, MPLAB XC8
 * LED on GP0, bit-banged UART TX on GP1, active-low button on GP2
 * External 32.768kHz low-frequency crystal
 * ----------------------------------------------------------------------- */


 //xc8-cc -mcpu=12F675 -mdfp="C:\Program Files\Microchip\MPLABX\v6.35\packs\Microchip\PIC10-12Fxxx_DFP\1.9.189\xc8" -O2 main.c
#include <xc.h>
#include <stdint.h>

#include "timer1_timebase.h"
#include "uart_transmitter.h"
#include "led_indicator.h"
#include "button_input.h"
#include "clock_calendar.h"
#include "clock_display.h"

// Configuration bits
#pragma config FOSC = LP      // low-frequency crystal on GP4/OSC2 and GP5/OSC1
#pragma config WDTE = OFF     // watchdog timer off
#pragma config PWRTE = ON     // power-up timer on
#pragma config MCLRE = ON     // GP3/MCLR pin is MCLR
#pragma config BOREN = ON     // brown-out reset on
#pragma config CP = OFF       // no code protect
#pragma config CPD = OFF      // no data code protect

#define _XTAL_FREQ 32768

#define APPLICATION_DISPLAY_DIRTY 0x01
#define APPLICATION_CLEANING_DUE 0x02
#define APPLICATION_BLINK_VISIBLE 0x04

static uint8_t application_flags;

static uint8_t application_read_blink_phase(void)
{
    return (timer1_timebase_read_ticks() & 0x1FFF) >= (TIMER1_TICKS_PER_SECOND / 2);
}

static void application_service_inputs(void)
{
    button_input_service();
    clock_calendar_service();
    led_indicator_service();

    if (button_input_take_long_hold_event())
    {
        clock_calendar_set_adjustment_active(1);
        clock_calendar_increment_selected();
        application_flags |= APPLICATION_DISPLAY_DIRTY;
    }

    if (button_input_take_release_event())
    {
        if (clock_calendar_is_adjusting())
        {
            clock_calendar_set_adjustment_active(0);
        }
        else
        {
            clock_calendar_select_next_field();
        }
    }

    if (clock_calendar_take_display_changed())
    {
        application_flags |= APPLICATION_DISPLAY_DIRTY;
    }

    if ((application_read_blink_phase() != 0) !=
        ((application_flags & APPLICATION_BLINK_VISIBLE) != 0))
    {
        application_flags ^= APPLICATION_BLINK_VISIBLE;
        application_flags |= APPLICATION_DISPLAY_DIRTY;
    }
}

static void application_send_byte(char data)
{
    button_input_suspend_gpio_interrupt();
    uart_transmitter_send_byte(data);

    while (uart_transmitter_is_busy())
    {
        application_service_inputs();
    }

    button_input_resume_gpio_interrupt();
    application_service_inputs();
}

void __interrupt() isr(void)
{
    if (T0IF)
    {
        uart_transmitter_handle_timer0_interrupt();
    }

    if (GPIF)
    {
        volatile uint8_t gpio_snapshot = GPIO;
        (void)gpio_snapshot;
        GPIF = 0;
        button_input_handle_gpio_change_interrupt();
    }
}

void main(void)
{
    ANSEL = 0x00;
    CMCON = 0x07;
    TRISIO = 0b111100; // GP0, GP1 outputs; GP2-GP5 inputs
    GPIO = 0x00;

    timer1_timebase_init();
    led_indicator_init();
    button_input_init();
    clock_calendar_init();
    uart_transmitter_init();

    if (application_read_blink_phase())
    {
        application_flags |= APPLICATION_BLINK_VISIBLE;
    }
    application_flags |= APPLICATION_DISPLAY_DIRTY;

    application_send_byte((char)CLOCK_DISPLAY_AUTOMATIC_RETURN);
    application_send_byte((char)CLOCK_DISPLAY_CURSOR_INVISIBLE);
    application_send_byte(CLOCK_DISPLAY_CLEAN_COMMAND);

    while (1)
    {
        application_service_inputs();

        if (clock_calendar_take_clean_due())
        {
            application_flags |= APPLICATION_CLEANING_DUE | APPLICATION_DISPLAY_DIRTY;
        }

        if (application_flags & APPLICATION_DISPLAY_DIRTY)
        {
            clock_calendar_time_t time = clock_calendar_get_time();
            clock_field_t selected_field = clock_calendar_get_selected_field();
            uint8_t blink_visible = (application_flags & APPLICATION_BLINK_VISIBLE) != 0;
            uint8_t cleaning_sequence = (application_flags & APPLICATION_CLEANING_DUE) != 0;

            application_flags &= (uint8_t)~(APPLICATION_DISPLAY_DIRTY | APPLICATION_CLEANING_DUE);
            clock_display_render(
                time,
                selected_field,
                blink_visible,
                cleaning_sequence,
                application_send_byte);
        }
    }
}
