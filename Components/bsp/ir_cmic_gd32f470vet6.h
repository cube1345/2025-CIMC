
#ifndef MCU_CMIC_GD32F470VET6_H
#define MCU_CMIC_GD32F470VET6_H

#include "gd32f4xx.h"
#include "gd32f4xx_sdio.h"
#include "gd32f4xx_dma.h"
#include "systick.h"

#include "ebtn.h"
#include "oled.h"
#include "gd25qxx.h"
#include "sdio_sdcard.h"
#include "ff.h"
#include "diskio.h"

#include "sd_app.h"
#include "led_app.h"
#include "adc_app.h"
#include "oled_app.h"
#include "usart_app.h"
#include "rtc_app.h"
#include "btn_app.h"
#include "scheduler.h"

#include "perf_counter.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************************************************/
/* LED */
#define LED_PORT        GPIOD
#define LED_CLK_PORT    RCU_GPIOD

#define LED1_PIN        GPIO_PIN_8
#define LED2_PIN        GPIO_PIN_9
#define LED3_PIN        GPIO_PIN_10
#define LED4_PIN        GPIO_PIN_11
#define LED5_PIN        GPIO_PIN_12
#define LED6_PIN        GPIO_PIN_13

#define LED1_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED1_PIN; else GPIO_BC(LED_PORT) = LED1_PIN; } while(0)
#define LED2_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED2_PIN; else GPIO_BC(LED_PORT) = LED2_PIN; } while(0)
#define LED3_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED3_PIN; else GPIO_BC(LED_PORT) = LED3_PIN; } while(0)
#define LED4_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED4_PIN; else GPIO_BC(LED_PORT) = LED4_PIN; } while(0)
#define LED5_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED5_PIN; else GPIO_BC(LED_PORT) = LED5_PIN; } while(0)
#define LED6_SET(x)     do { if(x) GPIO_BOP(LED_PORT) = LED6_PIN; else GPIO_BC(LED_PORT) = LED6_PIN; } while(0)

#define LED1_TOGGLE     do { GPIO_TG(LED_PORT) = LED1_PIN; } while(0)
#define LED2_TOGGLE     do { GPIO_TG(LED_PORT) = LED2_PIN; } while(0)
#define LED3_TOGGLE     do { GPIO_TG(LED_PORT) = LED3_PIN; } while(0)
#define LED4_TOGGLE     do { GPIO_TG(LED_PORT) = LED4_PIN; } while(0)
#define LED5_TOGGLE     do { GPIO_TG(LED_PORT) = LED5_PIN; } while(0)
#define LED6_TOGGLE     do { GPIO_TG(LED_PORT) = LED6_PIN; } while(0)

#define LED1_ON         do { GPIO_BC(LED_PORT) = LED1_PIN; } while(0)
#define LED2_ON         do { GPIO_BC(LED_PORT) = LED2_PIN; } while(0)
#define LED3_ON         do { GPIO_BC(LED_PORT) = LED3_PIN; } while(0)
#define LED4_ON         do { GPIO_BC(LED_PORT) = LED4_PIN; } while(0)
#define LED5_ON         do { GPIO_BC(LED_PORT) = LED5_PIN; } while(0)
#define LED6_ON         do { GPIO_BC(LED_PORT) = LED6_PIN; } while(0)

#define LED1_OFF        do { GPIO_BOP(LED_PORT) = LED1_PIN; } while(0)
#define LED2_OFF        do { GPIO_BOP(LED_PORT) = LED2_PIN; } while(0)
#define LED3_OFF        do { GPIO_BOP(LED_PORT) = LED3_PIN; } while(0)
#define LED4_OFF        do { GPIO_BOP(LED_PORT) = LED4_PIN; } while(0)
#define LED5_OFF        do { GPIO_BOP(LED_PORT) = LED5_PIN; } while(0)
#define LED6_OFF        do { GPIO_BOP(LED_PORT) = LED6_PIN; } while(0)


// FUNCTION
void bsp_led_init(void);

/***************************************************************************************************************/
/* KEY */
#define KEYE_PORT        GPIOE
#define KEYB_PORT        GPIOB
#define KEYA_PORT        GPIOA
#define KEYE_CLK_PORT    RCU_GPIOE
#define KEYB_CLK_PORT    RCU_GPIOB
#define KEYA_CLK_PORT    RCU_GPIOA

#define KEY1_PIN        GPIO_PIN_15
#define KEY2_PIN        GPIO_PIN_13
#define KEY3_PIN        GPIO_PIN_11
#define KEY4_PIN        GPIO_PIN_9
#define KEY5_PIN        GPIO_PIN_7
#define KEY6_PIN        GPIO_PIN_0
#define KEYW_PIN        GPIO_PIN_0

#define KEY1_READ       gpio_input_bit_get(KEYE_PORT, KEY1_PIN)
#define KEY2_READ       gpio_input_bit_get(KEYE_PORT, KEY2_PIN)
#define KEY3_READ       gpio_input_bit_get(KEYE_PORT, KEY3_PIN)
#define KEY4_READ       gpio_input_bit_get(KEYE_PORT, KEY4_PIN)
#define KEY5_READ       gpio_input_bit_get(KEYE_PORT, KEY5_PIN)
#define KEY6_READ       gpio_input_bit_get(KEYB_PORT, KEY6_PIN)
#define KEYW_READ       gpio_input_bit_get(KEYA_PORT, KEYW_PIN)

// FUNCTION
void bsp_btn_init(void);

/***************************************************************************************************************/

/* OLED */
#define I2C0_OWN_ADDRESS7      0x72
#define I2C0_SLAVE_ADDRESS7    0x82
#define I2C0_DATA_ADDRESS      (uint32_t)&I2C_DATA(I2C0)

#define OLED_PORT        GPIOB
#define OLED_CLK_PORT    RCU_GPIOB
#define OLED_DAT_PIN     GPIO_PIN_9
#define OLED_CLK_PIN     GPIO_PIN_8

// FUNCTION
void bsp_oled_init(void);

/***************************************************************************************************************/

/* gd25qxx */

#define SPI_PORT              GPIOB
#define SPI_CLK_PORT          RCU_GPIOB

#define SPI_NSS               GPIO_PIN_12
#define SPI_SCK               GPIO_PIN_13
#define SPI_MISO              GPIO_PIN_14
#define SPI_MOSI              GPIO_PIN_15

// FUNCTION
void bsp_gd25qxx_init(void);

/***************************************************************************************************************/

/* USART */
#define DEBUG_USART               (USART0)
#define USART0_RDATA_ADDRESS      ((uint32_t)&USART_DATA(USART0))

#define USART_PORT                GPIOA
#define USARTI_CLK_PORT           RCU_GPIOA

#define USART_TX                  GPIO_PIN_9
#define USART_RX                  GPIO_PIN_10

// FUNCTION
void bsp_usart_init(void);

/***************************************************************************************************************/

/* ADC */
#define ADC1_PORT       GPIOC
#define ADC1_CLK_PORT   RCU_GPIOC

#define ADC1_PIN        GPIO_PIN_0

// FUNCTION
void bsp_adc_init(void);

/***************************************************************************************************************/

/* DAC */
#define CONVERT_NUM                     (1)
#define DAC0_R12DH_ADDRESS              (0x40007408)  /* 12位右对齐DAC数据寄存器地址 */

#define DAC1_PORT       GPIOA
#define DAC1_CLK_PORT   RCU_GPIOA

#define DAC1_PIN        GPIO_PIN_4

// FUNCTION
void bsp_dac_init(void);

/***************************************************************************************************************/

/* RTC */
//#define RTC_CLOCK_SOURCE_LXTAL  // 注释掉LXTAL，使用IRC32K
#define RTC_CLOCK_SOURCE_IRC32K
#define BKP_VALUE    0x32F0

// FUNCTION
int bsp_rtc_init(void);

/***************************************************************************************************************/

#ifdef __cplusplus
  }
#endif

#endif /* MCU_CMIC_GD32F470VET6_H */
