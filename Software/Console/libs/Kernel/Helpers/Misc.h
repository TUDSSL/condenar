#include "am_util.h"

#include "DebugUtils.h"
void LOG_X(char* prefix, char* data);

//Logs over UART with the desired severity

#define LOG_W(...) UART_OUT("[WARN] "); UART_OUT(__VA_ARGS__); UART_OUT("\n"); DelayMS(5);
#define LOG_E(...) UART_OUT("[ERROR] "); UART_OUT(__VA_ARGS__); UART_OUT("\n"); DelayMS(5);

//#define DEBUG_LOG_ENABLED
#define INFO_LOG_ENABLED

#ifdef DEBUG_LOG_ENABLED
#define LOG_DEBUG(...) UART_OUT("[DEBUG] "); UART_OUT(__VA_ARGS__); UART_OUT("\n"); DelayMS(5);
#else
#define LOG_DEBUG(...) ;
#endif

#ifdef INFO_LOG_ENABLED
#define LOG_I(...) UART_OUT("[INFO] "); UART_OUT(__VA_ARGS__); UART_OUT("\n"); DelayMS(5);
#else
#define LOG_I(...) ;
#endif

#ifndef MIN
    #define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef MAX
    #define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

//From https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


extern bool inDownloadMode;

//Outputs text over UART, no endline, printf compatible
#define UART_OUT(...) am_util_stdio_printf(__VA_ARGS__);

void GPIO_ModeOut(int pin);
void GPIO_ModeIn(int pin,bool pullUPEnabled,bool pullDownEnabled);
void GPIO_SetLevel(int pin, bool value);
bool GPIO_GetLevel(int pin);
void DelayMS(int ms);
void GPIO_ToggleLevel(int pin);

void SpinLoop();

void SetupIO();
uint8_t GetButtonState();

void PowerEverythingDown(bool autoEnable);

void SetAutoEnable(bool enable);

//Returns a calibrated value between -1 and 1 for both joysticks
void GetJoysticks(float* joystickX, float* joystickY);

float GetVBat();

void TurnOffIfNeccessary(); //Checks if it is being signaled to turn off and does it 

//From https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-bytes
uint8_t reverseByte(uint8_t b);

void LightSensor_Initialize();
float LightSensor_GetValue();

void ChangeBacklightPWMValue(uint8_t value);