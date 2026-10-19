#include "uart_echo_probe.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_usart.h"

/* LL/CMSIS headers are copied unchanged from STM32Cube_FW_G0_V1.6.3.
 * This is a polling command transport trial, not the final vehicle protocol.
 * HSI16 is selected directly for USART2, independently of SYSCLK/APB dividers.
 */
#define VVVP_UART_CLOCK_HZ       16000000U
#define VVVP_UART_BAUD           115200U
#define VVVP_UART_READY_POLLS    100000U
#define VVVP_UART_QUEUE_SIZE     128U
#define VVVP_UART_ERRORS         (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)

volatile uint32_t vvvp_uart_init_status = VVVP_UART_NOT_STARTED;
volatile uint32_t vvvp_uart_brr = 0U;
volatile uint32_t vvvp_uart_banner_sent = 0U;
volatile uint32_t vvvp_uart_rx_count = 0U;
volatile uint32_t vvvp_uart_tx_count = 0U;
volatile uint32_t vvvp_uart_last_rx = 0U;
volatile uint32_t vvvp_uart_error_count = 0U;
volatile uint32_t vvvp_uart_last_error = 0U;
volatile uint32_t vvvp_uart_dropped_count = 0U;

static uint8_t tx_queue[VVVP_UART_QUEUE_SIZE];
static uint32_t queue_head;
static uint32_t queue_tail;
static uint32_t queue_count;
static int32_t rx_queue[VVVP_UART_QUEUE_SIZE];
static uint32_t rx_head;
static uint32_t rx_tail;
static uint32_t rx_count;

static void receive_fault(void)
{
    /* Lose pending, unparsed bytes conservatively rather than apply a damaged
     * command. Keep an ordered fault event ahead of any subsequently read byte.
     * This is trial overload/error recovery, not a final communication policy.
     */
    vvvp_uart_dropped_count += rx_count;
    rx_tail = 0U;
    rx_head = 1U;
    rx_count = 1U;
    rx_queue[0] = VVVP_UART_RX_FAULT;
}

int32_t vvvp_uart_read_event(void)
{
    int32_t result;
    if (rx_count == 0U) { return VVVP_UART_RX_EMPTY; }
    result = rx_queue[rx_tail];
    rx_tail = (rx_tail + 1U) % VVVP_UART_QUEUE_SIZE;
    --rx_count;
    return result;
}

uint32_t vvvp_uart_try_write(const char* text, uint32_t length)
{
    uint32_t i;
    if (vvvp_uart_init_status != VVVP_UART_READY ||
        length > VVVP_UART_QUEUE_SIZE - queue_count) { return 0U; }
    for (i = 0U; i < length; ++i) {
        tx_queue[queue_head] = (uint8_t)text[i];
        queue_head = (queue_head + 1U) % VVVP_UART_QUEUE_SIZE;
    }
    queue_count += length;
    return 1U;
}

void vvvp_uart_echo_init(void)
{
    uint32_t polls;

    vvvp_uart_init_status = VVVP_UART_NOT_STARTED;
    vvvp_uart_brr = 0U;
    vvvp_uart_banner_sent = 0U;
    vvvp_uart_rx_count = 0U;
    vvvp_uart_tx_count = 0U;
    vvvp_uart_last_rx = 0U;
    vvvp_uart_error_count = 0U;
    vvvp_uart_last_error = 0U;
    vvvp_uart_dropped_count = 0U;
    queue_head = 0U;
    queue_tail = 0U;
    queue_count = 0U;
    rx_head = 0U;
    rx_tail = 0U;
    rx_count = 0U;

    LL_RCC_HSI_Enable();
    for (polls = 0U; polls < VVVP_UART_READY_POLLS; ++polls)
    {
        if (LL_RCC_HSI_IsReady() != 0U)
        {
            break;
        }
    }
    /* This is an iteration budget, not a calibrated millisecond timeout. */
    if (LL_RCC_HSI_IsReady() == 0U)
    {
        vvvp_uart_init_status = VVVP_UART_HSI_TIMEOUT;
        return;
    }

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_USART2);
    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_USART2);
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_HSI);

    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_1);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_3, LL_GPIO_AF_1);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_2 | LL_GPIO_PIN_3, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_2, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_3, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_2, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_3, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_3, LL_GPIO_MODE_ALTERNATE);

    /* Reset leaves USART disabled: configure 115200, 8N1, TX/RX, no RTS/CTS. */
    LL_USART_SetPrescaler(USART2, LL_USART_PRESCALER_DIV1);
    LL_USART_SetOverSampling(USART2, LL_USART_OVERSAMPLING_16);
    LL_USART_SetDataWidth(USART2, LL_USART_DATAWIDTH_8B);
    LL_USART_SetParity(USART2, LL_USART_PARITY_NONE);
    LL_USART_SetStopBitsLength(USART2, LL_USART_STOPBITS_1);
    LL_USART_SetHWFlowCtrl(USART2, LL_USART_HWCONTROL_NONE);
    LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX_RX);
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_DisableFIFO(USART2);
    LL_USART_SetBaudRate(USART2, VVVP_UART_CLOCK_HZ, LL_USART_PRESCALER_DIV1,
                        LL_USART_OVERSAMPLING_16, VVVP_UART_BAUD);
    vvvp_uart_brr = USART2->BRR;
    LL_USART_Enable(USART2);

    for (polls = 0U; polls < VVVP_UART_READY_POLLS; ++polls)
    {
        if ((LL_USART_IsActiveFlag_TEACK(USART2) != 0U) &&
            (LL_USART_IsActiveFlag_REACK(USART2) != 0U))
        {
            vvvp_uart_init_status = VVVP_UART_READY;
            return;
        }
    }
    LL_USART_Disable(USART2);
    vvvp_uart_init_status = VVVP_UART_ENABLE_TIMEOUT;
}

void vvvp_uart_echo_poll(void)
{
    uint32_t status;
    uint8_t received;
    uint8_t outgoing;

    if (vvvp_uart_init_status != VVVP_UART_READY)
    {
        return;
    }

    /* At most one receive and one transmit per call; never wait for a byte. */
    status = USART2->ISR;
    if ((status & VVVP_UART_ERRORS) != 0U)
    {
        ++vvvp_uart_error_count;
        vvvp_uart_last_error = status & VVVP_UART_ERRORS;
        LL_USART_ClearFlag_PE(USART2);
        LL_USART_ClearFlag_FE(USART2);
        LL_USART_ClearFlag_NE(USART2);
        LL_USART_ClearFlag_ORE(USART2);
        if ((status & USART_ISR_RXNE_RXFNE) != 0U)
        {
            (void)LL_USART_ReceiveData8(USART2);
            ++vvvp_uart_dropped_count;
        }
        receive_fault();
    }
    else if ((status & USART_ISR_RXNE_RXFNE) != 0U)
    {
        received = LL_USART_ReceiveData8(USART2);
        vvvp_uart_last_rx = received;
        ++vvvp_uart_rx_count;
        if (rx_count < VVVP_UART_QUEUE_SIZE)
        {
            rx_queue[rx_head] = received;
            rx_head = (rx_head + 1U) % VVVP_UART_QUEUE_SIZE;
            ++rx_count;
        }
        else
        {
            ++vvvp_uart_dropped_count;
            receive_fault();
        }
    }

    if (LL_USART_IsActiveFlag_TXE_TXFNF(USART2) == 0U)
    {
        return;
    }
    if (queue_count != 0U)
    {
        outgoing = tx_queue[queue_tail];
        queue_tail = (queue_tail + 1U) % VVVP_UART_QUEUE_SIZE;
        --queue_count;
    }
    else
    {
        return;
    }
    LL_USART_TransmitData8(USART2, outgoing);
    ++vvvp_uart_tx_count;
}
