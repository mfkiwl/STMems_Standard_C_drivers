/*
 ******************************************************************************
 * @file    fsm_glance.c
 * @author  Sensors Software Solution Team
 * @brief   This file shows how to generate and handle a Free Fall event.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 +
 * - NUCLEO_F401RE + X-NUCLEO-IKS01A3
 * - DISCOVERY_SPC584B +
 * - NUCLEO_H503RB + X-NUCLEO-IKS4A1
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F401RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * NUCLEO_STM32H503RG - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I3C(Default)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */

//#define STEVAL_MKI109V3  /* little endian */
//#define NUCLEO_F401RE    /* little endian */
//#define SPC584B_DIS      /* big endian */

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */

#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2
/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3 915

#elif defined(NUCLEO_F401RE)
/* NUCLEO_F401RE: Define communication interface */
#define SENSOR_BUS hi2c1

#elif defined(SPC584B_DIS)
/* DISCOVERY_SPC584B: Define communication interface */
#define SENSOR_BUS I2CD1

#elif defined(NUCLEO_H503RB)
/* NUCLEO_H503RB: Define communication interface */
#define SENSOR_BUS hi3c1

#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "lsm6dsv320x_glance_detection.h"
#include "lsm6dsv320x_reg.h"

#if defined(NUCLEO_F401RE)
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"

#elif defined(STEVAL_MKI109V3)
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#elif defined(SPC584B_DIS)
#include "components.h"

#elif defined(NUCLEO_H503RB)
#include "usart.h"
#include "i3c.h"
#include "i3c_api.h"
#include <stdio.h>

static uint8_t i3c_dyn_addr = 0x0A;
#endif

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME            10 //ms

/* Private variables ---------------------------------------------------------*/
static uint8_t whoamI;
static uint8_t tx_buffer[1000];

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_delay(uint32_t ms);
static void platform_init(void *handle);

static stmdev_ctx_t dev_ctx;
static uint8_t glance_event_catched = 0;
static uint8_t deglance_event_catched = 0;

void lsm6dsv320x_fsm_glance_handler(void)
{
  lsm6dsv320x_all_sources_t status;
  lsm6dsv320x_fsm_out_t fsm_out;

  /* Read output only if new xl value is available */
  lsm6dsv320x_all_sources_get(&dev_ctx, &status);

  if (status.fsm1) {
    lsm6dsv320x_fsm_out_get(&dev_ctx, &fsm_out);

    switch(fsm_out.fsm_outs1) {
    case 0x20:
      glance_event_catched = 1;
      break;
    case 0x08:
      deglance_event_catched = 1;
      break;
    }
  }
}

/* Main Example --------------------------------------------------------------*/
void lsm6dsv320x_fsm_glance(void)
{
  lsm6dsv320x_reset_t rst;
  uint32_t i;

  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;

  /* Init test platform */
  platform_init(dev_ctx.handle);
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lsm6dsv320x_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LSM6DSV320X_ID)
    while (1);

  /* Restore default configuration */
  lsm6dsv320x_reset_set(&dev_ctx, LSM6DSV320X_RESTORE_CTRL_REGS);
  do {
    lsm6dsv320x_reset_get(&dev_ctx, &rst);
  } while (rst != LSM6DSV320X_READY);

#if defined(NUCLEO_H503RB)
  /* if I3C is used then INT pin must be explicitly enabled */
  lsm6dsv320x_i3c_int_en_set(&dev_ctx, 1);
#endif

  /* Start Machine Learning Core configuration */
  for ( i = 0; i < (sizeof(lsm6dsv320x_glance_detection) /
                    sizeof(ucf_line_t) ); i++ ) {
    lsm6dsv320x_write_reg(&dev_ctx, lsm6dsv320x_glance_detection[i].address,
                       (uint8_t *)&lsm6dsv320x_glance_detection[i].data, 1);
  }

  /* wait forever (FF event handle in irq handler) */
  while (1) {
    if (glance_event_catched) {
      glance_event_catched = 0;

      snprintf((char *)tx_buffer, sizeof(tx_buffer), "GLANCE event\r\n");
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }
    if (deglance_event_catched) {
      deglance_event_catched = 0;

      snprintf((char *)tx_buffer, sizeof(tx_buffer), "DEGLANCE event\r\n");
      tx_com(tx_buffer, strlen((char const *)tx_buffer));
    }
  }
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
#if defined(NUCLEO_F401RE)
  HAL_I2C_Mem_Write(handle, LSM6DSV320X_I2C_ADD_L, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t*) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle,  LSM6DSV320X_I2C_ADD_L & 0xFE, reg, (uint8_t*) bufp, len);
#elif defined(NUCLEO_H503RB)
  i3c_write(handle, i3c_dyn_addr, reg, (uint8_t*) bufp, len);
#endif
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
#if defined(NUCLEO_F401RE)
  HAL_I2C_Mem_Read(handle, LSM6DSV320X_I2C_ADD_L, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LSM6DSV320X_I2C_ADD_L & 0xFE, reg, bufp, len);
#elif defined(NUCLEO_H503RB)
  i3c_read(handle, i3c_dyn_addr, reg, bufp, len);
#endif
  return 0;
}

/*
 * @brief  platform specific outputs on terminal (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
#if defined(NUCLEO_F401RE)
  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
#elif defined(STEVAL_MKI109V3)
  CDC_Transmit_FS(tx_buffer, len);
#elif defined(SPC584B_DIS)
  sd_lld_write(&SD2, tx_buffer, len);
#elif defined(NUCLEO_H503RB)
  HAL_UART_Transmit(&huart3, tx_buffer, len, 1000);
#endif
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
#if defined(NUCLEO_F401RE) || defined(STEVAL_MKI109V3) || defined(NUCLEO_H503RB)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
#endif
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void *handle)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);

#elif defined(NUCLEO_H503RB)
  i3c_set_bus_frequency(handle, 1000000);
  i3c_rstdaa(handle);
  i3c_setdasa(handle, LSM6DSV320X_I2C_ADD_L, &i3c_dyn_addr, 1);
  i3c_set_bus_frequency(handle, 12500000);

#endif
}
