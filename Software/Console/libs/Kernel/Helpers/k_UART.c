#include "UART.h"
#include "am_mcu_apollo.h"
#include "PinDefinitions.h"
#include "am_util_stdio.h"
#include "Misc.h"


void *phUART;

//__attribute__ ((section (".textA")))
uint8_t UART_RX_Buffer[10000];

const am_hal_uart_config_t g_sUartConfig =
{
    .ui32BaudRate = 115200,
    .eDataBits = AM_HAL_UART_DATA_BITS_8,
    .eParity = AM_HAL_UART_PARITY_NONE,
    .eStopBits = AM_HAL_UART_ONE_STOP_BIT,
    .eFlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

    .eRXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16,
    .eTXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16    
};

void am_uart_isr(void)
{
    uint32_t ui32Status;
    am_hal_uart_interrupt_status_get(phUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(phUART, ui32Status);
    am_hal_uart_interrupt_service(phUART, ui32Status);
}

void uart_print(char *pcStr)
{
    uint32_t ui32StrLen = 0;
    uint32_t ui32BytesWritten = 0;

    // Measure the length of the string.
    while (pcStr[ui32StrLen] != 0)
    {
        ui32StrLen++;
    }

    // Print the string via the UART.
    const am_hal_uart_transfer_t sUartWrite =
    {
        .eType = AM_HAL_UART_BLOCKING_WRITE,
        .pui8Data = (uint8_t *) pcStr,
        .ui32NumBytes = ui32StrLen,
        .pui32BytesTransferred = &ui32BytesWritten,
        .ui32TimeoutMs = 100,
        .pfnCallback = NULL,
        .pvContext = NULL,
        .ui32ErrorStatus = 0
    };

    am_hal_uart_transfer(phUART, &sUartWrite);

    if (ui32BytesWritten != ui32StrLen)
    { // Couldn't send the whole string!!       
        while(1);
        
    }
}

void SetupUART(){

    am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_TX =
    {
        .GP.cfg_b.uFuncSel             = AM_HAL_PIN_0_UART0TX,
        .GP.cfg_b.eDriveStrength       = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P5X,
    };

    am_hal_gpio_pincfg_t g_AM_BSP_GPIO_COM_UART_RX =
    {
        .GP.cfg_b.uFuncSel             = AM_HAL_PIN_32_UART0RX,
    };

    am_hal_uart_initialize(0, &phUART);
    am_hal_uart_power_control(phUART, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_uart_configure(phUART, &g_sUartConfig); 
    am_hal_uart_buffer_configure(phUART,NULL,0,UART_RX_Buffer,sizeof(UART_RX_Buffer));

    #ifdef DEVKIT
        am_hal_gpio_pinconfig(60, g_AM_BSP_GPIO_COM_UART_TX);
        am_hal_gpio_pinconfig(47, g_AM_BSP_GPIO_COM_UART_RX);
    #else
        am_hal_gpio_pinconfig(PIN_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
        am_hal_gpio_pinconfig(PIN_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);
    #endif
      
    am_hal_uart_interrupt_enable(phUART,AM_HAL_UART_INT_RX | AM_HAL_UART_INT_OVER_RUN);
    NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn));
    am_util_stdio_printf_init(uart_print);
    am_util_stdio_terminal_clear();
    am_hal_interrupt_master_enable();    
}