#include "Kernel.h"
#include "Misc.h"
#include "PinDefinitions.h"

void UART_LOG_SendString(char* toSend);
void UART_LOG_SendByte(char toSend);




void UART_LOG_SendCommand(char* toSend){
    int checksum = 0;
    for(int i=0;i<strlen(toSend);i++){
        checksum += toSend[i];
    }    
    checksum = checksum % 256;

    char checksumStr[16] = {0};
    am_util_stdio_sprintf(checksumStr, "%03d", checksum);

    UART_LOG_SendString("TUD");
    UART_LOG_SendString(checksumStr);
    UART_LOG_SendString("|");
    UART_LOG_SendString(toSend);
    UART_LOG_SendString("\n");
}

void UART_LOG_SendString(char* toSend){
    while(*toSend != 0){
        UART_LOG_SendByte(*toSend);
        toSend++;
    }
}

void UART_LOG_SendByte(char toSend){
    int32_t toWait = 1000000/19200;
    GPIO_SetLevel(PIN_UART_LOG, !0);
    am_util_delay_us(toWait);

    for(int i=0;i<8;i++){
        GPIO_SetLevel(PIN_UART_LOG, !((toSend >> i) & 0x01));
        am_util_delay_us(toWait);
    }

    GPIO_SetLevel(PIN_UART_LOG, !1);
    am_util_delay_us(toWait);
}


void UART_LOG_StartExperiment(int experimentNumber){
    char toSendStr[64] = {0};
    am_util_stdio_sprintf(toSendStr, "EXPSTART,%d", experimentNumber);
    UART_LOG_SendCommand(toSendStr);
}

void UART_LOG_EndExperiment(){
    UART_LOG_SendCommand("EXPEND");
}

void UART_LOG_SendEvent(int eventNumber){
    char toSendStr[64] = {0};
    am_util_stdio_sprintf(toSendStr, "EVENT,%d", eventNumber);
    UART_LOG_SendCommand(toSendStr);
}

void UART_LOG_SetChannelData(int channelNumber,int channelData){
    char toSendStr[64] = {0};
    am_util_stdio_sprintf(toSendStr, "CHANNEL,%d,%d", channelNumber, channelData);
    UART_LOG_SendCommand(toSendStr);
}
