#define SSD1963_ID 0x1615701 //Also the response to the GET_DDB command

#define SSD1963_REG_NOP 0x00 //No operation
#define SSD1963_REG_SOFT_RESET 0x01 //Software Reset
#define SSD1963_REG_GET_POWER_MODE 0x0A //Get the current power mode
#define SSD1963_REG_GET_ADDRESS_MODE 0x0B //Get the frame buffer to the display panel read order
#define SSD1963_REG_GET_DISPLAY_MODE 0x0D //The SSD1963 returns the Display Image Mode.
#define SSD1963_REG_GET_TEAR_EFFECT_STATUS 0x0E //Get the Tear Effect status
#define SSD1963_REG_ENTER_SLEEP_MODE 0x10 //Turn off the panel. This command will pull high the GPIO0. If GPIO0 is configured as normal GPIO or LCD miscellaneous signal with command set_gpio_conf, this command will be ignored.
#define SSD1963_REG_EXIT_SLEEP_MODE 0x11 //Turn on the panel. This command will pull low the GPIO0. If GPIO0 is configured as normal GPIO or LCD miscellaneous signal with command set_gpio_conf, this command will be ignored.
#define SSD1963_REG_ENTER_PARTIAL_MODE 0x12 //Part of the display area is used for image display.
#define SSD1963_REG_ENTER_NORMAL_MODE 0x13 //The whole display area is used for image display.
#define SSD1963_REG_EXIT_INVERT_MODE 0x20 //Displayed image colors are not inverted.
#define SSD1963_REG_ENTER_INVERT_MODE 0x21 //Displayed image colors are inverted.
#define SSD1963_REG_SET_GAMMA_CURVE 0x26 //Selects the gamma curve used by the display panel.
#define SSD1963_REG_SET_DISPLAY_OFF 0x28 //Blanks the display panel
#define SSD1963_REG_SET_DISPLAY_ON 0x29 //Show the image on the display panel
#define SSD1963_REG_SET_COLUMN_ADDRESS 0x2A //Set the column address
#define SSD1963_REG_SET_PAGE_ADDRESS 0x2B //Set the page address
#define SSD1963_REG_WRITE_MEMORY_START 0x2C //Transfer image information from the host processor interface to the SSD1963 starting at the location provided by set_column_address and set_page_address
#define SSD1963_REG_READ_MEMORY_START 0x2E //Transfer image data from the SSD1963 to the host processor interface starting at the location provided by set_column_address and set_page_address
#define SSD1963_REG_SET_PARTIAL_AREA 0x30 //Defines the partial display area on the display panel
#define SSD1963_REG_SET_SCROLL_AREA 0x33 //Defines the vertical scrolling and fixed area on display area
#define SSD1963_REG_SET_TEAR_OFF 0x34 //Synchronization information is not sent from the SSD1963 to the host processor
#define SSD1963_REG_SET_TEAR_ON 0x35 //Synchronization information is sent from the SSD1963 to the host processor at the start of VFP
#define SSD1963_REG_SET_ADDRESS_MODE 0x36 //Set the read order from frame buffer to the display panel
#define SSD1963_REG_SET_SCROLL_START 0x37 //Defines the vertical scrolling starting point
#define SSD1963_REG_EXIT_IDLE_MODE 0x38 //Full color depth is used for the display panel
#define SSD1963_REG_ENTER_IDLE_MODE 0x39 //Reduce color depth is used on the display panel.
#define SSD1963_REG_WRITE_MEMORY_CONTINUE 0x3C //Transfer image information from the host processor interface to the SSD1963 from the last written location
#define SSD1963_REG_READ_MEMORY_CONTINUE 0x3E //Read image data from the SSD1963 continuing after the last read_memory_continue or read_memory_start
#define SSD1963_REG_SET_TEAR_SCANLINE 0x44 //Synchronization information is sent from the SSD1963 to the host processor when the display panel refresh reaches the provided scanline
#define SSD1963_REG_GET_SCANLINE 0x45 //Get the current scan line
#define SSD1963_REG_READ_DDB 0xA1 //Read the DDB from the provided location
#define SSD1963_REG_SET_LCD_MODE_ 0xB0 //Set the LCD panel mode and resolution
#define SSD1963_REG_GET_LCD_MODE 0xB1 //Get the current LCD panel mode, pad strength and resolution
#define SSD1963_REG_SET_HORI_PERIOD 0xB4 //Set front porch
#define SSD1963_REG_GET_HORI_PERIOD 0xB5 //Get current front porch settings
#define SSD1963_REG_SET_VERT_PERIOD 0xB6 //Set the vertical blanking interval between last scan line and next LFRAME pulse
#define SSD1963_REG_GET_VERT_PERIOD 0xB7 //Set the vertical blanking interval between last scan line and next LFRAME pulse
#define SSD1963_REG_SET_GPIO_CONF 0xB8 //Set the GPIO configuration. If the GPIO is not used for LCD, set the direction. Otherwise, they are toggled with LCD signals.
#define SSD1963_REG_GET_GPIO_CONF 0xB9 //Get the current GPIO configuration
#define SSD1963_REG_SET_GPIO_VALUE 0xBA //Set GPIO value for GPIO configured as output
#define SSD1963_REG_GET_GPIO_STATUS 0xBB //Read current GPIO status. If the individual GPIO was configured as input, the value is the status of the corresponding pin. Otherwise, it is the programmed value.
#define SSD1963_REG_SET_POST_PROC 0xBC //Set the image post processor
#define SSD1963_REG_GET_POST_PROC 0xBD //Set the image post processor
#define SSD1963_REG_SET_PWM_CONF 0xBE //Set the image post processor
#define SSD1963_REG_GET_PWM_CONF 0xBF //Set the image post processor
#define SSD1963_REG_SET_LCD_GEN0 0xC0 //Set the rise, fall, period and toggling properties of LCD signal generator 0
#define SSD1963_REG_GET_LCD_GEN0 0xC1 //Get the current settings of LCD signal generator 0
#define SSD1963_REG_SET_LCD_GEN1 0xC2 //Set the rise, fall, period and toggling properties of LCD signal generator 1
#define SSD1963_REG_GET_LCD_GEN1 0xC3 //Get the current settings of LCD signal generator 1
#define SSD1963_REG_SET_LCD_GEN2 0xC4 //Set the rise, fall, period and toggling properties of LCD signal generator 2
#define SSD1963_REG_GET_LCD_GEN2 0xC5 //Get the current settings of LCD signal generator 2
#define SSD1963_REG_SET_LCD_GEN3 0xC6 //Set the rise, fall, period and toggling properties of LCD signal generator 3
#define SSD1963_REG_GET_LCD_GEN3 0xC7 //Get the current settings of LCD signal generator 3
#define SSD1963_REG_SET_GPIO0_ROP 0xC8 //Set the GPIO0 with respect to the LCD signal generators using ROP operation. No effect if the GPIO0 is configured as general GPIO.
#define SSD1963_REG_GET_GPIO0_ROP 0xC9 //Get the GPIO0 properties with respect to the LCD signal generators.
#define SSD1963_REG_SET_GPIO1_ROP 0xCA //Set the GPIO1 with respect to the LCD signal generators using ROP operation. No effect if the GPIO1 is configured as general GPIO.
#define SSD1963_REG_GET_GPIO1_ROP 0xCB //Get the GPIO1 properties with respect to the LCD signal generators.
#define SSD1963_REG_SET_GPIO2_ROP 0xCC //Set the GPIO2 with respect to the LCD signal generators using ROP operation. No effect if the GPIO2 is configured as general GPIO.
#define SSD1963_REG_GET_GPIO2_ROP 0xCD //Get the GPIO2 properties with respect to the LCD signal generators.
#define SSD1963_REG_SET_GPIO3_ROP 0xCE //Set the GPIO3 with respect to the LCD signal generators using ROP operation. No effect if the GPIO3 is configured as general GPIO.
#define SSD1963_REG_GET_GPIO3_ROP 0xCF //Get the GPIO3 properties with respect to the LCD signal generators.
#define SSD1963_REG_SET_DBC_CONF 0xD0 //Set the dynamic back light configuration
#define SSD1963_REG_GET_DBC_CONF 0xD1 //Get the current dynamic back light configuration
#define SSD1963_REG_SET_DBC_TH 0xD4 //Set the threshold for each level of power saving
#define SSD1963_REG_GET_DBC_TH 0xD5 //Get the threshold for each level of power saving
#define SSD1963_REG_SET_PLL 0xE0 //Start the PLL. Before the start, the system was operated with the crystal oscillator or clock input
#define SSD1963_REG_SET_PLL_MN 0xE2 //Set the PLL
#define SSD1963_REG_GET_PLL_MN 0xE3 //Get the PLL settings
#define SSD1963_REG_GET_PLL_STATUS 0xE4 //Get the current PLL status
#define SSD1963_REG_SET_DEEP_SLEEP 0xE5 //Set deep sleep mode
#define SSD1963_REG_SET_LSHIFT_FREQ 0xE6 //Set the LSHIFT (pixel clock) frequency
#define SSD1963_REG_GET_LSHIFT_FREQ 0xE7 //Get current LSHIFT (pixel clock) frequency setting
#define SSD1963_REG_SET_PIXEL_DATA_INTERFACE 0xF0 //Set the pixel data format of the parallel host processor interface
#define SSD1963_REG_GET_PIXEL_DATA_INTERFACE 0xF1 //Get the current pixel data format settings