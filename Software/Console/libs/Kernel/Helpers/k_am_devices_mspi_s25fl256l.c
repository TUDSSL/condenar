//*****************************************************************************
//
//! @file am_devices_mspi_s25fl256l.c
//!
//! @brief General Multibit SPI S25fl256l driver.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2021, Ambiq Micro, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision release_sdk_3_0_0-742e5ac27c of the AmbiqSuite Development Package.
//
//*****************************************************************************



#include "am_mcu_apollo.h"
#include "am_devices_mspi_s25fl256l.h"
#include "am_util_stdio.h"
#include "am_bsp.h"
#include "am_util_delay.h"
#include "Flash.h"
#include "Helpers/Misc.h"
#include "PinDefinitions.h"

//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************
#define AM_DEVICES_MSPI_S25Fl256l_TIMEOUT             1000000
#define AM_DEVICES_MSPI_S25Fl256l_ERASE_TIMEOUT       1000000

#define FLASH_MSPI_MODULE FLASH_MSPI_NUM //You also need to change the name of this method: am_mspiX_isr

uint32_t DMATCBBuffer[2560];
bool useQUIOinXIP = false;
void* mainMSPIHandle;

am_hal_mspi_dev_config_t MSPI_S25fl256l_Serial_CE1_MSPIConfig =
{
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
    .eClockFreq           = AM_HAL_MSPI_CLK_4MHZ,
    .ui8TurnAround        = 8,
    .eAddrCfg             = AM_HAL_MSPI_ADDR_3_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_SERIAL_CE1,
    .bSendInstr           = true,
    .bSendAddr            = true,
    .bTurnaround          = true,
    .ui16ReadInstr         = AM_DEVICES_MSPI_S25Fl256l_FAST_READ,
    .ui16WriteInstr        = AM_DEVICES_MSPI_S25Fl256l_PAGE_PROGRAM,
#if defined(AM_PART_APOLLO3P)
    .ui8WriteLatency      = 0,
    .bEnWriteLatency      = false,
    .bEmulateDDR          = false,
    .ui16DMATimeLimit     = 0,
    .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_NONE,
#endif    
   // .scramblingStartAddr  = 0,
   // .scramblingEndAddr    = 0,
};

am_hal_mspi_dev_config_t MSPI_S25fl256l_Quad_CE1_MSPIConfig =
{
  .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
  .eClockFreq           = AM_HAL_MSPI_CLK_4MHZ,
  .ui8TurnAround        = 6,
  .eAddrCfg             = AM_HAL_MSPI_ADDR_3_BYTE,
  .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
  .eDeviceConfig        = AM_HAL_MSPI_FLASH_QUAD_CE1_1_4_4,
  .bSendInstr           = true,
  .bSendAddr            = true,
  .bTurnaround          = true,
  .ui16ReadInstr         = 0xEB, //EB=4x, address=x4    6B=4x, address=1x
  .ui16WriteInstr        = AM_DEVICES_MSPI_S25Fl256l_PAGE_PROGRAM,
#if defined(AM_PART_APOLLO3P)
  .ui8WriteLatency      = 0,
  .bEnWriteLatency      = false,
  .bEmulateDDR          = false,
  .ui16DMATimeLimit     = 0,
  .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_NONE,
#endif  
 // .scramblingStartAddr  = 0,
  //.scramblingEndAddr    = 0,
};

struct
{
    am_hal_mspi_device_e eHalDeviceEnum;
    am_hal_mspi_dev_config_t *psDevConfig;
}g_MSPIS25fl256l_DevConfig[] =
{
    {AM_HAL_MSPI_FLASH_SERIAL_CE1,       &MSPI_S25fl256l_Serial_CE1_MSPIConfig},
    {AM_HAL_MSPI_FLASH_QUAD_CE1_1_1_4,         &MSPI_S25fl256l_Quad_CE1_MSPIConfig},
};


//
// Forward declarations.
//


bool SendWriteEnable(){
    return AM_HAL_STATUS_SUCCESS == am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_ENABLE, false, 0, NULL, 0);
}

bool SendWriteDisable(){
    return AM_HAL_STATUS_SUCCESS == am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_DISABLE, false, 0, NULL, 0);
}

bool SendWriteEnableVolatile(){
    return AM_HAL_STATUS_SUCCESS == am_devices_mspi_s25fsp64s_command_write(0x50, false, 0, NULL, 0);
}



//*****************************************************************************
//
// Cypress S25Fl256l Support
//
//*****************************************************************************
//
// Device specific initialization function.
//
static
uint32_t am_device_init_flash()
{  
    SendWriteEnable();

    uint8_t toSend[3];
    toSend[0]=0b01000000; //Enable quad mode   
    am_devices_mspi_s25fsp64s_command_write(0x01,false,0,(uint32_t*)&toSend,1);    
    SendWriteDisable();

    return AM_HAL_STATUS_SUCCESS;
}


//*****************************************************************************
//
// Generic Command Write function.
//
//*****************************************************************************
uint32_t am_devices_mspi_s25fsp64s_command_write(uint8_t ui8Instr, bool bSendAddr,
                                                 uint32_t ui32Addr, uint32_t *pData,
                                                 uint32_t ui32NumBytes)
{
    uint32_t ui32Status;
    am_hal_mspi_pio_transfer_t      stMSPIFlashPIOTransaction = {0};

    // Create the individual write transaction.
    stMSPIFlashPIOTransaction.ui32NumBytes       = ui32NumBytes;
    stMSPIFlashPIOTransaction.eDirection         = AM_HAL_MSPI_TX;
    stMSPIFlashPIOTransaction.bSendAddr          = bSendAddr;
    stMSPIFlashPIOTransaction.ui32DeviceAddr     = ui32Addr;
    stMSPIFlashPIOTransaction.bSendInstr         = true;
    stMSPIFlashPIOTransaction.ui16DeviceInstr    = ui8Instr;
    stMSPIFlashPIOTransaction.bTurnaround        = false;
#if 0 // A3DS-25 Deprecate MSPI CONT
    stMSPIFlashPIOTransaction.bContinue          = false;
#endif // A3DS-25

    //stMSPIFlashPIOTransaction.bQuadCmd         = false;

    stMSPIFlashPIOTransaction.pui32Buffer        = pData;

#if defined (MSPI_XIPMIXED)
    uint32_t u32Arg = (pFlash->stSetting.eDeviceConfig % 2) ? AM_HAL_MSPI_FLASH_SERIAL_CE1 : AM_HAL_MSPI_FLASH_SERIAL_CE0;
    am_hal_mspi_control(pHandle, AM_HAL_MSPI_REQ_DEVICE_CONFIG, &u32Arg);
#endif

    // Execute the transction over MSPI.
    ui32Status = am_hal_mspi_blocking_transfer(mainMSPIHandle, &stMSPIFlashPIOTransaction,
                                         AM_DEVICES_MSPI_S25Fl256l_TIMEOUT);
#if defined (MSPI_XIPMIXED)
    am_hal_mspi_control(pHandle, AM_HAL_MSPI_REQ_DEVICE_CONFIG, &pFlash->stSetting.eDeviceConfig);
#endif

    return ui32Status;
}

//*****************************************************************************
//
// Generic Command Read function.
//
//*****************************************************************************
uint32_t am_devices_mspi_s25fsp64s_command_read(uint8_t ui8Instr, bool bSendAddr,
                                                uint32_t ui32Addr, uint32_t *pData,
                                                uint32_t ui32NumBytes)
{
    uint32_t ui32Status;
    am_hal_mspi_pio_transfer_t      stMSPIFlashPIOTransaction = {0};

    // Create the individual write transaction.
    stMSPIFlashPIOTransaction.eDirection         = AM_HAL_MSPI_RX;
    stMSPIFlashPIOTransaction.bSendAddr          = bSendAddr;
    stMSPIFlashPIOTransaction.ui32DeviceAddr     = ui32Addr;
    stMSPIFlashPIOTransaction.bSendInstr         = true;
    stMSPIFlashPIOTransaction.ui16DeviceInstr    = ui8Instr;

    stMSPIFlashPIOTransaction.bTurnaround        = false;
#if 0 // A3DS-25 Deprecate MSPI CONT
    stMSPIFlashPIOTransaction.bContinue          = false;
#endif // A3DS-25

    stMSPIFlashPIOTransaction.ui32NumBytes     = ui32NumBytes;
    //stMSPIFlashPIOTransaction.bQuadCmd      = false;
    stMSPIFlashPIOTransaction.pui32Buffer        = pData;

#if defined (MSPI_XIPMIXED)
    uint32_t u32Arg = (pFlash->stSetting.eDeviceConfig % 2) ? AM_HAL_MSPI_FLASH_SERIAL_CE1 : AM_HAL_MSPI_FLASH_SERIAL_CE0;
    am_hal_mspi_control(pHandle, AM_HAL_MSPI_REQ_DEVICE_CONFIG, &u32Arg);
#endif

    // Execute the transction over MSPI.
    ui32Status = am_hal_mspi_blocking_transfer(mainMSPIHandle, &stMSPIFlashPIOTransaction,
                                         AM_DEVICES_MSPI_S25Fl256l_TIMEOUT);
#if defined (MSPI_XIPMIXED)
    am_hal_mspi_control(pHandle, AM_HAL_MSPI_REQ_DEVICE_CONFIG, &pFlash->stSetting.eDeviceConfig);
#endif

    return ui32Status;
}

static
void pfnMSPI_S25Fl256l_Callback(void *pCallbackCtxt, uint32_t status)
{
    // Set the DMA complete flag.
    *(volatile bool *)pCallbackCtxt = true;
}


#if FLASH_MSPI_MODULE == 0
void am_mspi0_isr(void)
#elif FLASH_MSPI_MODULE == 1
void am_mspi1_isr(void)
#elif FLASH_MSPI_MODULE == 2
void am_mspi2_isr(void)
#else
#error "Invalid MSPI num"
#endif
{
    uint32_t      ui32Status;
    am_hal_mspi_interrupt_status_get(mainMSPIHandle, &ui32Status, false);
    am_hal_mspi_interrupt_clear(mainMSPIHandle, ui32Status);
    am_hal_mspi_interrupt_service(mainMSPIHandle, ui32Status);
}


//*****************************************************************************
//
//! @brief Initialize the mspi_s25fl256l driver.
//!
//! @param psIOMSettings - IOM device structure describing the target spiflash.
//! @param pfnWriteFunc - Function to use for spi writes.
//! @param pfnReadFunc - Function to use for spi reads.
//!
//! This function should be called before any other am_devices_spiflash
//! functions. It is used to set tell the other functions how to communicate
//! with the external spiflash hardware.
//!
//! The \e pfnWriteFunc and \e pfnReadFunc variables may be used to provide
//! alternate implementations of SPI write and read functions respectively. If
//! they are left set to 0, the default functions am_hal_iom_spi_write() and
//! am_hal_iom_spi_read() will be used.
//!
//! @return None.
//
//*****************************************************************************


uint32_t am_devices_mspi_s25fl256l_init(am_hal_mspi_clock_e clockSpeed, bool _useQIOinXIP)
{
    uint32_t ui32Status;   

    useQUIOinXIP=_useQIOinXIP;
 
    MSPI_S25fl256l_Serial_CE1_MSPIConfig.eClockFreq = clockSpeed;
    MSPI_S25fl256l_Quad_CE1_MSPIConfig.eClockFreq = clockSpeed;
    
    // Enable fault detection.
    am_hal_fault_capture_enable();

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_initialize(FLASH_MSPI_MODULE, &mainMSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to initialize MSPI.\n");
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_power_control(mainMSPIHandle, AM_HAL_SYSCTRL_WAKE, false))
    {
        am_util_stdio_printf("Error - Failed to power on MSPI.\n");
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    am_hal_mspi_config_t generalConfig = {
        .bClkonD4=false,
        .pTCB=DMATCBBuffer,
        .ui32TCBSize=(sizeof(DMATCBBuffer) / sizeof(uint32_t))
    };
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_configure(mainMSPIHandle, &generalConfig))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }


    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_device_configure(mainMSPIHandle, &MSPI_S25fl256l_Serial_CE1_MSPIConfig))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_enable(mainMSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to enable MSPI.\n");
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    am_bsp_mspi_pins_enable(FLASH_MSPI_MODULE, MSPI_S25fl256l_Serial_CE1_MSPIConfig.eDeviceConfig);
        


    if (AM_HAL_STATUS_SUCCESS != am_devices_mspi_s25fl256l_reset())
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    DelayMS(1);

    //
    // Device specific MSPI Flash initialization.
    //
    ui32Status = am_device_init_flash();
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    DelayMS(10);

    // Enable MSPI interrupts.
    ui32Status = am_hal_mspi_interrupt_clear(mainMSPIHandle, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR );
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    ui32Status = am_hal_mspi_interrupt_enable(mainMSPIHandle, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR );
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    am_hal_mspi_xip_config_t cfg0 = {
        .eAPMode=AM_HAL_MSPI_AP_READ_ONLY,
        .ui32APBaseAddr=MSPI_XIP_BASE_ADDRESS,
        .eAPSize =AM_HAL_MSPI_AP_SIZE8M,
        .scramblingStartAddr=0,
        .scramblingEndAddr=0
    };
    ui32Status = am_hal_mspi_control(mainMSPIHandle,AM_HAL_MSPI_REQ_XIP_CONFIG,&cfg0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
  

    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}


uint32_t am_devices_mspi_s25fl256l_reset()
{
    uint32_t      ui32PIOBuffer[32] = {0};
   //
    // Send the command sequence to reset the device and return status.
    //
    if (AM_HAL_STATUS_SUCCESS != am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_RESET_ENABLE, false, 0, ui32PIOBuffer, 0))
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    if (AM_HAL_STATUS_SUCCESS != am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_RESET_MEMORY, false, 0, ui32PIOBuffer, 0))
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}


uint32_t am_devices_mspi_s25fl256l_id()
{
    uint32_t      ui32Status;
    uint32_t      ui32DeviceID;

    //
    // Send the command sequence to read the Device ID and return status.
    //

    ui32Status = am_devices_mspi_s25fsp64s_command_read(AM_DEVICES_MSPI_S25Fl256l_READ_ID, false, 0, &ui32DeviceID, 3);
    LOG_I("Flash ID is %d", ui32DeviceID);
    if ( ((ui32DeviceID & AM_DEVICES_MSPI_S25Fl256l_ID_MASK) == AM_DEVICES_MSPI_S25Fl256l_ID) &&
       (AM_HAL_STATUS_SUCCESS == ui32Status) )
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
    }
    else
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
}


uint32_t am_devices_mspi_s25fl256l_status(uint32_t *pStatus)
{
    uint32_t      ui32Status;

    //
    // Send the command sequence to read the device status.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_read( AM_DEVICES_MSPI_S25Fl256l_READ_STATUS, false, 0, pStatus, 1);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}

uint32_t am_devices_mspi_s25fl256l_read_adv(uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress, uint32_t ui32NumBytes, uint32_t ui32PauseCondition,uint32_t ui32StatusSetClr, am_hal_mspi_callback_t pfnCallback, void *pCallbackCtxt)
{
    am_hal_mspi_dma_transfer_t    Transaction;
    uint32_t                      ui32Status;

    // Set the DMA priority
    Transaction.ui8Priority = 1;

    // Set the transfer direction to RX (Read)
    Transaction.eDirection = AM_HAL_MSPI_RX;

    // Set the transfer count in bytes.
    Transaction.ui32TransferCount = ui32NumBytes;

    // Set the address to read data from.
    Transaction.ui32DeviceAddress = ui32ReadAddress;

    // Set the target SRAM buffer address.
    Transaction.ui32SRAMAddress = (uint32_t)pui8RxBuffer;

    // Clear the CQ stimulus.
    Transaction.ui32PauseCondition = ui32PauseCondition;
    // Clear the post-processing
    Transaction.ui32StatusSetClr = ui32StatusSetClr;

    // Check the transaction status.
    ui32Status = am_hal_mspi_nonblocking_transfer(mainMSPIHandle, &Transaction,
                                                  AM_HAL_MSPI_TRANS_DMA, pfnCallback, pCallbackCtxt);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Return the status.
    //
    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}

//*****************************************************************************
//
//! @brief Reads the contents of the external s25fl256l into a buffer.
//!
//! @param pui8RxBuffer - Buffer to store the received data from the s25fl256l
//! @param ui32ReadAddress - Address of desired data in external s25fl256l
//! @param ui32NumBytes - Number of bytes to read from external s25fl256l
//!
//! This function reads the external s25fl256l at the provided address and stores
//! the received data into the provided buffer location. This function will
//! only store ui32NumBytes worth of data.
//
//! @return 32-bit status
//
//*****************************************************************************
uint32_t am_devices_mspi_s25fl256l_read(uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress, uint32_t ui32NumBytes, bool bWaitForCompletion)
{
    am_hal_mspi_dma_transfer_t    Transaction;
    uint32_t                      ui32Status;

    // Set the DMA priority
    Transaction.ui8Priority = 1;

    // Set the transfer direction to RX (Read)
    Transaction.eDirection = AM_HAL_MSPI_RX;

    // Set the transfer count in bytes.
    Transaction.ui32TransferCount = ui32NumBytes;

    // Set the address to read data from.
    Transaction.ui32DeviceAddress = ui32ReadAddress;

    // Set the target SRAM buffer address.
    Transaction.ui32SRAMAddress = (uint32_t)pui8RxBuffer;

    // Clear the CQ stimulus.
    Transaction.ui32PauseCondition = 0;
    // Clear the post-processing
    Transaction.ui32StatusSetClr = 0;

    if (bWaitForCompletion)
    {
        // Start the transaction.
        volatile bool bDMAComplete = false;
        ui32Status = am_hal_mspi_nonblocking_transfer(mainMSPIHandle, &Transaction, AM_HAL_MSPI_TRANS_DMA, pfnMSPI_S25Fl256l_Callback, (void *)&bDMAComplete);

        // Check the transaction status.
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }

        // Wait for DMA Complete or Timeout
        for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_TIMEOUT/1000; i++)
        {
            if (bDMAComplete)
            {
                break;
            }
            //
            // Call the BOOTROM cycle function to delay for about 1 microsecond.
            //
            DelayMS(1);
           // am_hal_flash_delay( FLASH_CYCLES_US(1) );
        }

        // Check the status.
        if (!bDMAComplete)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
    }
    else
    {
        // Check the transaction status.
        ui32Status = am_hal_mspi_nonblocking_transfer(mainMSPIHandle, &Transaction, AM_HAL_MSPI_TRANS_DMA, NULL, NULL);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
    }

    //
    // Return the status.
    //
    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}

//*****************************************************************************
//
//! @brief Reads the contents of the external s25fl256l into a buffer.
//!
//! @param pui8RxBuffer - Buffer to store the received data from the s25fl256l
//! @param ui32ReadAddress - Address of desired data in external s25fl256l
//! @param ui32NumBytes - Number of bytes to read from external s25fl256l
//!
//! This function reads the external s25fl256l at the provided address and stores
//! the received data into the provided buffer location. This function will
//! only store ui32NumBytes worth of data.
//
//! @return 32-bit status
//
//*****************************************************************************
uint32_t
am_devices_mspi_s25fl256l_read_hiprio(uint8_t *pui8RxBuffer,
                                      uint32_t ui32ReadAddress,
                                      uint32_t ui32NumBytes,
                                      bool bWaitForCompletion)
{
    am_hal_mspi_dma_transfer_t    Transaction;
    uint32_t                      ui32Status;

    // Set the DMA priority
    Transaction.ui8Priority = 1;

    // Set the transfer direction to RX (Read)
    Transaction.eDirection = AM_HAL_MSPI_RX;

    // Set the transfer count in bytes.
    Transaction.ui32TransferCount = ui32NumBytes;

    // Set the address to read data from.
    Transaction.ui32DeviceAddress = ui32ReadAddress;

    // Set the target SRAM buffer address.
    Transaction.ui32SRAMAddress = (uint32_t)pui8RxBuffer;

    // Clear the CQ stimulus.
    Transaction.ui32PauseCondition = 0;
    // Clear the post-processing
    Transaction.ui32StatusSetClr = 0;

    if (bWaitForCompletion)
    {
        // Start the transaction.
        volatile bool bDMAComplete = false;
        ui32Status = am_hal_mspi_highprio_transfer(mainMSPIHandle, &Transaction, AM_HAL_MSPI_TRANS_DMA, pfnMSPI_S25Fl256l_Callback, (void*)&bDMAComplete);

        // Check the transaction status.
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }

        // Wait for DMA Complete or Timeout
        for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_TIMEOUT/1000; i++)
        {
            if (bDMAComplete)
            {
                break;
            }
            //
            // Call the BOOTROM cycle function to delay for about 1 microsecond.
            //
            DelayMS(1);
        }

        // Check the status.
        if (!bDMAComplete)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
    }
    else
    {
        // Check the transaction status.
        ui32Status = am_hal_mspi_highprio_transfer(mainMSPIHandle, &Transaction,
                                                      AM_HAL_MSPI_TRANS_DMA, NULL, NULL);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
    }

    //
    // Return the status.
    //
    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}



//*****************************************************************************
//
//! @brief Programs the given range of s25fl256l addresses.
//!
//! @param ui32DeviceNumber - Device number of the external s25fl256l
//! @param pui8TxBuffer - Buffer to write the external s25fl256l data from
//! @param ui32WriteAddress - Address to write to in the external s25fl256l
//! @param ui32NumBytes - Number of bytes to write to the external s25fl256l
//!
//! This function uses the data in the provided pui8TxBuffer and copies it to
//! the external s25fl256l at the address given by ui32WriteAddress. It will copy
//! exactly ui32NumBytes of data from the original pui8TxBuffer pointer. The
//! user is responsible for ensuring that they do not overflow the target s25fl256l
//! memory or underflow the pui8TxBuffer array
//
//! @return 32-bit status
//
//*****************************************************************************
uint32_t am_devices_mspi_s25fl256l_write(uint8_t *pui8TxBuffer, uint32_t ui32WriteAddress,uint32_t ui32NumBytes,bool bWaitForCompletion)
{
    am_hal_mspi_dma_transfer_t    Transaction;
    bool                          bWriteComplete = false;
    uint32_t                      ui32BytesLeft = ui32NumBytes;
    uint32_t                      ui32PageAddress = ui32WriteAddress;
    uint32_t                      ui32BufferAddress = (uint32_t)pui8TxBuffer;
    uint32_t                      ui32Status;
    uint32_t      ui32PIOBuffer[32] = {0};

    while (ui32BytesLeft > 0)
    {
        //
        // Send the command sequence to enable writing.
        //
        ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_ENABLE, false, 0, ui32PIOBuffer, 0);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }

        // Set the DMA priority
        Transaction.ui8Priority = 1;

        // Set the transfer direction to TX (Write)
        Transaction.eDirection = AM_HAL_MSPI_TX;

        if (ui32BytesLeft > AM_DEVICES_MSPI_S25Fl256l_PAGE_SIZE)
        {
            // Set the transfer count in bytes.
            Transaction.ui32TransferCount = AM_DEVICES_MSPI_S25Fl256l_PAGE_SIZE;
            ui32BytesLeft -= AM_DEVICES_MSPI_S25Fl256l_PAGE_SIZE;
        }
        else
        {
            // Set the transfer count in bytes.
            Transaction.ui32TransferCount = ui32BytesLeft;
            ui32BytesLeft = 0;
        }

        // Set the address configuration for the read
        //    Transaction.eAddrCfg = AM_HAL_MSPI_ADDR_3_BYTE;

        //    Transaction.eAddrExtSize = AM_HAL_MSPI_ADDR_EXT_0_BYTE;
        //    Transaction.ui8AddrExtValue = 0;

        // Set the address to read data to.
        Transaction.ui32DeviceAddress = ui32PageAddress;
        ui32PageAddress += AM_DEVICES_MSPI_S25Fl256l_PAGE_SIZE;

        // Set the source SRAM buffer address.
        Transaction.ui32SRAMAddress = ui32BufferAddress;
        ui32BufferAddress += AM_DEVICES_MSPI_S25Fl256l_PAGE_SIZE;

        // Clear the CQ stimulus.
        Transaction.ui32PauseCondition = 0;
        // Clear the post-processing
        Transaction.ui32StatusSetClr = 0;

        // Start the transaction.
        volatile bool bDMAComplete = false;
        //am_hal_mspi_state_t           *pMSPIState = (am_hal_mspi_state_t *)mainMSPIHandle;
        ui32Status = am_hal_mspi_nonblocking_transfer(mainMSPIHandle, &Transaction, AM_HAL_MSPI_TRANS_DMA, pfnMSPI_S25Fl256l_Callback, (void*)&bDMAComplete);

        // Check the transaction status.
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }

        // Wait for DMA Complete or Timeout
        for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_TIMEOUT/1000; i++)
        {
            if (bDMAComplete)
            {
                break;
            }
            //
            // Call the BOOTROM cycle function to delay for about 1 microsecond.
            //
            DelayMS(1);
        }

        // Check the status.
        if (!bDMAComplete)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }

        //
        // Wait for the Write In Progress to indicate the erase is complete.
        //
        for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_TIMEOUT; i++)
        {
          ui32Status = am_devices_mspi_s25fsp64s_command_read(AM_DEVICES_MSPI_S25Fl256l_READ_STATUS, false, 0, ui32PIOBuffer, 1);
          /*if ((AM_HAL_MSPI_FLASH_QUADPAIRED == pFlash->stSetting.eDeviceConfig) ||
              (AM_HAL_MSPI_FLASH_QUADPAIRED_SERIAL == pFlash->stSetting.eDeviceConfig))
          {
            bWriteComplete = ((0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP)) &&
                              (0 == ((ui32PIOBuffer[0] >> 8) & AM_DEVICES_MSPI_S25Fl256l_WIP)));
          }
          else
          {*/
            bWriteComplete = (0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP));
         // }
          am_util_delay_us(100);
          if (bWriteComplete)
          {
            break;
          }
        }

        //
        // Send the command sequence to disable writing.
        //
        ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_DISABLE, false, 0, ui32PIOBuffer, 0);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
    }

  //
  // Return the status.
  //
  return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}

uint32_t am_devices_mspi_s25fl256l_mass_erase()
{
    bool          bEraseComplete = false;
    uint32_t      ui32Status;
    uint32_t      ui32PIOBuffer[32] = {0};

    //
    // Send the command sequence to enable writing.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_ENABLE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Send the command sequence to do the mass erase.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_BULK_ERASE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    //
    // Wait for the Write In Progress to indicate the erase is complete.
    //
    for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_ERASE_TIMEOUT; i++)
    {
        ui32PIOBuffer[0] = 0;
        am_devices_mspi_s25fsp64s_command_read(AM_DEVICES_MSPI_S25Fl256l_READ_STATUS, false, 0, ui32PIOBuffer, 1);
       /* if ((AM_HAL_MSPI_FLASH_QUADPAIRED == pFlash->stSetting.eDeviceConfig) ||
            (AM_HAL_MSPI_FLASH_QUADPAIRED_SERIAL == pFlash->stSetting.eDeviceConfig))
        {
            bEraseComplete = ((0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP)) &&
                              (0 == ((ui32PIOBuffer[0] >> 8) & AM_DEVICES_MSPI_S25Fl256l_WIP)));
        }
        else
        {*/
            bEraseComplete = (0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP));
       // }
        if (bEraseComplete)
        {
            break;
        }
        am_util_delay_ms(10);
    }

    //
    // Check the status.
    //
    if (!bEraseComplete)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Send the command sequence to disable writing.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_DISABLE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Return the status.
    //
    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}


uint32_t am_devices_mspi_s25fl256l_sector_erase(uint32_t ui32SectorAddress)
{
    bool          bEraseComplete = false;
    uint32_t      ui32Status;
    uint32_t      ui32PIOBuffer[32] = {0};

    //
    // Send the command sequence to enable writing.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write(AM_DEVICES_MSPI_S25Fl256l_WRITE_ENABLE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Send the command sequence to do the sector erase.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write(0x20, true, ui32SectorAddress, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Wait for the Write In Progress to indicate the erase is complete.
    //
    for (uint32_t i = 0; i < AM_DEVICES_MSPI_S25Fl256l_ERASE_TIMEOUT; i++)
    {
        ui32PIOBuffer[0] = 0;
        am_devices_mspi_s25fsp64s_command_read(AM_DEVICES_MSPI_S25Fl256l_READ_STATUS, false, 0, ui32PIOBuffer, 1);
       /* if ((AM_HAL_MSPI_FLASH_QUADPAIRED == pFlash->stSetting.eDeviceConfig) ||
            (AM_HAL_MSPI_FLASH_QUADPAIRED_SERIAL == pFlash->stSetting.eDeviceConfig))
        {
            bEraseComplete = ((0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP)) &&
                              (0 == ((ui32PIOBuffer[0] >> 8) & AM_DEVICES_MSPI_S25Fl256l_WIP)));
        }
        else
        {*/
            bEraseComplete = (0 == (ui32PIOBuffer[0] & AM_DEVICES_MSPI_S25Fl256l_WIP));
        //}
        if (bEraseComplete)
        {
            break;
        }
        am_util_delay_ms(10);
    }

    //
    // Check the status.
    //
    if (!bEraseComplete)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    uint32_t    ui32EraseStatus = 0;
    //
    // Send the command sequence to check the erase status.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write( AM_DEVICES_MSPI_S25Fl256l_EVAL_ERASE_STATUS, true, ui32SectorAddress, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Send the command sequence to read the device status.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_read( AM_DEVICES_MSPI_S25Fl256l_READ_STATUS2, false, 0, &ui32EraseStatus, 1);
    if ( (AM_HAL_STATUS_SUCCESS != ui32Status) || ((ui32EraseStatus & 0x60) > 0))
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Send the command sequence to disable writing.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write( AM_DEVICES_MSPI_S25Fl256l_WRITE_DISABLE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
    //
    // Return the status.
    //
    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}


uint32_t am_devices_mspi_s25fl256l_enable_xip()
{
    uint32_t ui32Status;

    //
    // Enable XIP on the MSPI.
    //
    ui32Status = am_hal_mspi_control(mainMSPIHandle, AM_HAL_MSPI_REQ_XIP_EN, NULL);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }



#if !MSPI_USE_CQ
    // Disable the DMA interrupts.
    ui32Status = am_hal_mspi_interrupt_disable(mainMSPIHandle,
                                               AM_HAL_MSPI_INT_DMAERR |
                                               AM_HAL_MSPI_INT_DMACMP );
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }
#endif

    if(useQUIOinXIP){
        // Disable MSPI defore re-configuring it
        ui32Status = am_hal_mspi_disable(mainMSPIHandle);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
        //
        // Re-Configure the MSPI for the requested operation mode.
        //
        ui32Status = am_hal_mspi_device_configure(mainMSPIHandle, &MSPI_S25fl256l_Quad_CE1_MSPIConfig);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
        // Re-Enable MSPI
        ui32Status = am_hal_mspi_enable(mainMSPIHandle);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }    
        // Configure the MSPI pins.
        am_bsp_mspi_pins_enable(FLASH_MSPI_MODULE, MSPI_S25fl256l_Quad_CE1_MSPIConfig.eDeviceConfig);
    }
    

    //am_hal_cachectrl_disable();
    am_hal_cachectrl_enable();
    am_hal_cachectrl_control(AM_HAL_CACHECTRL_CONTROL_MRAM_CACHE_INVALIDATE,0);

    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}


uint32_t am_devices_mspi_s25fl256l_disable_xip(void *pHandle)
{
    uint32_t ui32Status;
    uint32_t      ui32PIOBuffer[32] = {0};

    //
    // Send the command to enable writing.
    //
    ui32Status = am_devices_mspi_s25fsp64s_command_write( AM_DEVICES_MSPI_S25Fl256l_WRITE_ENABLE, false, 0, ui32PIOBuffer, 0);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    //
    // Disable XIP on the MSPI.
    //
    ui32Status = am_hal_mspi_control(mainMSPIHandle, AM_HAL_MSPI_REQ_XIP_DIS, NULL);
    if (AM_HAL_STATUS_SUCCESS != ui32Status)
    {
        return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
    }

    if(useQUIOinXIP){

        // Disable MSPI defore re-configuring it
        ui32Status = am_hal_mspi_disable(mainMSPIHandle);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
        //
        // Re-Configure the MSPI for the requested operation mode.
        //
        ui32Status = am_hal_mspi_device_configure(mainMSPIHandle, &MSPI_S25fl256l_Serial_CE1_MSPIConfig);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }
        // Re-Enable MSPI
        ui32Status = am_hal_mspi_enable(mainMSPIHandle);
        if (AM_HAL_STATUS_SUCCESS != ui32Status)
        {
            return AM_DEVICES_MSPI_S25Fl256l_STATUS_ERROR;
        }    
        // Configure the MSPI pins.
        am_bsp_mspi_pins_enable(FLASH_MSPI_MODULE, MSPI_S25fl256l_Serial_CE1_MSPIConfig.eDeviceConfig);
    }

    return AM_DEVICES_MSPI_S25Fl256l_STATUS_SUCCESS;
}

