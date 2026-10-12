#ifndef UART_TRANSMITTER_H
#define UART_TRANSMITTER_H

#include <stdint.h>

#define UART_TX_GP1_MASK (1 << 1)
#define UART_TX_PIN GP1

/* Fosc/4 = 8192 Hz; Timer0's 1:2 prescaler and ISR overhead give ~303 baud. */
#define UART_TX_TIMER0_RELOAD (256 - 4)

static volatile uint16_t uart_tx_shift_register = 0x0000;
static volatile uint8_t uart_tx_bits_remaining = 0;

void uart_transmitter_init(void)
{
    TRISIO &= ~UART_TX_GP1_MASK;
    UART_TX_PIN = 1;

    /* GPPU=0 enables weak pull-ups; T0CS=0 selects Fosc/4; PSA=0, PS=000. */
    OPTION_REG = 0b01000000;

    TMR0 = UART_TX_TIMER0_RELOAD;
    T0IF = 0;
    T0IE = 1;
    GIE = 1;
}

void uart_transmitter_send_byte(char data)
{
    while (uart_tx_bits_remaining)
        ;

    uart_tx_shift_register = ((uint16_t)(uint8_t)data << 1) | 0x0200;
    uart_tx_bits_remaining = 10;

    TMR0 = UART_TX_TIMER0_RELOAD;
    T0IF = 0;
}

uint8_t uart_transmitter_is_busy(void)
{
    return uart_tx_bits_remaining != 0;
}

void uart_transmitter_handle_timer0_interrupt(void)
{
    TMR0 = UART_TX_TIMER0_RELOAD;
    T0IF = 0;

    if (uart_tx_bits_remaining)
    {
        UART_TX_PIN = uart_tx_shift_register & 0x01;
        uart_tx_shift_register >>= 1;
        uart_tx_bits_remaining--;
    }
    else
    {
        UART_TX_PIN = 1;
    }
}

#endif
