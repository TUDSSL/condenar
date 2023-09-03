#if HWREV == 1
#define PIN_LED1 30
#define PIN_LED2 90
#define PIN_LED3 91

#define PIN_BUTTON1 31
#define PIN_BUTTON2 2
#define PIN_BUTTON3 4
#define PIN_BUTTON4 27
#define PIN_BUTTON5 41
#define PIN_BUTTON6 28
#define PIN_BUTTON7 100
#define PIN_BUTTON_POWER 55
#define PIN_JOY_1 19
#define PIN_JOY_2 18
#define PIN_JOY_ON 101

#define PIN_VBAT 12

#define PIN_UART_RX 32
#define PIN_UART_TX 0

#define PIN_FORCE_1V8_EN 36


#define PIN_FLASH_SIO0_SI 64
#define PIN_FLASH_SIO1_SO 65
#define PIN_FLASH_SI02_WP 66
#define PIN_FLASH_SI03_RESET 67
#define PIN_FLASH_CK 72
#define PIN_FLASH_CS 68
#define FLASH_MSPI_NUM 0
#define FLASH_NCE_NUM 33

#define PIN_SCREEN_EXTCOMM 26
#define PIN_SCREEN_CLK 5
#define PIN_SCREEN_DT 6
#define PIN_SCREEN_CS 7

#define PIN_3V3_LA_EN 34
#define PIN_3V3_LA_DIS 33

#define PIN_SENSE_1 102
#define PIN_SENSE_2 103
#define PIN_SENSE_3 104

 
#define PIN_ENABLE_MEM_VCC 50

#elif HWREV == 2

#define PIN_BUTTON1 31
#define PIN_BUTTON2 2
#define PIN_BUTTON3 4
#define PIN_BUTTON4 27
#define PIN_BUTTON5 42
#define PIN_BUTTON6 28
#define PIN_BUTTON7 100
#define PIN_BUTTON_POWER 49
#define PIN_JOY_1 19
#define PIN_JOY_2 18

#define PIN_JOY_ON 101

#define PIN_VBAT 12

#define PIN_UART_RX 32
#define PIN_UART_TX 0

#define PIN_FORCE_1V8_EN 36

#define PIN_FLASH_SIO0_SI 37
#define PIN_FLASH_SIO1_SO 38
#define PIN_FLASH_SI02_WP 39
#define PIN_FLASH_SI03_RESET 40
#define PIN_FLASH_CK 45
#define PIN_FLASH_CS 41
#define FLASH_MSPI_NUM 1
#define FLASH_NCE_NUM 35

#define PIN_SCREEN_EXTCOMM 26
#define PIN_SCREEN_CLK 5
#define PIN_SCREEN_DT 6
#define PIN_SCREEN_CS 7

#define PIN_3V3_EN_LA 34
#define PIN_3V3_EN_AUTOON_LA 35
#define PIN_LATCH_STORE 33

#define PIN_SENSE_SOLAR 103
#define PIN_SENSE_CRANK 104
 
#define PIN_ENABLE_MEM_VCC 50
#define PIN_ENABLE_AUX_VCC 23
#define PIN_ENABLE_AUX_1V8_VCC 83

#define PIN_BL_DIM 43
//MSPI0

//IOM5
#define PIN_SSD1963_CS 55
#define PIN_SSD1963_RW 24
#define PIN_SSD1963_DC 52
#define PIN_SSD1963_RESET 51
#define PIN_SSD1963_TE 44
#define PIN_SSD1963_CLKIN 63

#define PIN_COM50_DI 48
#define PIN_COM50_CLK 47
#define PIN_COM50_CS 22

#define PIN_UART_LOG PIN_BUTTON7

#define PIN_LIGHTSENSOR_SDA 62
#define PIN_LIGHTSENSOR_SCL 61

#endif



