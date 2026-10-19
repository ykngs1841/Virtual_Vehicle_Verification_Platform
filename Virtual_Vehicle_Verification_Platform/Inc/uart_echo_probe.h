#ifndef VVVP_UART_ECHO_PROBE_H
#define VVVP_UART_ECHO_PROBE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VVVP_UART_NOT_STARTED = 0,
    VVVP_UART_READY = 1,
    VVVP_UART_HSI_TIMEOUT = 2,
    VVVP_UART_ENABLE_TIMEOUT = 3
} VvvpUartInitStatus;

/* Debugger observations for this transport test, not a vehicle protocol. */
extern volatile uint32_t vvvp_uart_init_status;
extern volatile uint32_t vvvp_uart_brr;
extern volatile uint32_t vvvp_uart_banner_sent;
extern volatile uint32_t vvvp_uart_rx_count;
extern volatile uint32_t vvvp_uart_tx_count;
extern volatile uint32_t vvvp_uart_last_rx;
extern volatile uint32_t vvvp_uart_error_count;
extern volatile uint32_t vvvp_uart_last_error;
extern volatile uint32_t vvvp_uart_dropped_count;

void vvvp_uart_echo_init(void);
void vvvp_uart_echo_poll(void);

/* The historical echo probe now supplies buffered command transport, no echo. */
#define VVVP_UART_RX_EMPTY (-1)
#define VVVP_UART_RX_FAULT (-2)
int32_t vvvp_uart_read_event(void);
uint32_t vvvp_uart_try_write(const char* text, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
