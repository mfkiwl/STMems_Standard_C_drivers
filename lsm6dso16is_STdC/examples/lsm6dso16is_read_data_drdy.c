/*
 ******************************************************************************
 * @file    read_data_drdy.c
 * @author  Sensors Software Solution Team
 * @brief   This file show how to get data from sensor.
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
 * - STEVAL_MKI109V3 + STEVAL-MKI196V1
 * - NUCLEO_F401RE + X_NUCLEO_IKS01A3
 * - DISCOVERY_SPC584B + STEVAL-MKI196V1
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F401RE - Host side: UART(COM) to USB bridge
 *                    - I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
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

#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "lsm6dso16is_reg.h"

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
#endif

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME      10

/* Private variables ---------------------------------------------------------*/
static int16_t data_raw_acceleration[3];
static int16_t data_raw_angular_rate[3];
static int16_t data_raw_temperature;
static float_t acceleration_mg[3];
static float_t angular_rate_mdps[3];
static float_t temperature_degC;
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
static void platform_init(void);

static   stmdev_ctx_t dev_ctx;

void lsm6dso16is_read_data_drdy_handler()
{
   uint8_t drdy;

  /* Read output only if new xl value is available */
  lsm6dso16is_xl_flag_data_ready_get(&dev_ctx, &drdy);

  if (drdy) {
    /* Read acceleration field data */
    memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
    lsm6dso16is_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
    acceleration_mg[0] =
      lsm6dso16is_from_fs2g_to_mg(data_raw_acceleration[0]);
    acceleration_mg[1] =
      lsm6dso16is_from_fs2g_to_mg(data_raw_acceleration[1]);
    acceleration_mg[2] =
      lsm6dso16is_from_fs2g_to_mg(data_raw_acceleration[2]);
    snprintf((char *)tx_buffer, sizeof(tx_buffer),
            "Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
            acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
    tx_com(tx_buffer, strlen((char const *)tx_buffer));
  }

  lsm6dso16is_gy_flag_data_ready_get(&dev_ctx, &drdy);

  if (drdy) {
    /* Read angular rate field data */
    memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
    lsm6dso16is_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate);
    angular_rate_mdps[0] =
      lsm6dso16is_from_fs2000dps_to_mdps(data_raw_angular_rate[0]);
    angular_rate_mdps[1] =
      lsm6dso16is_from_fs2000dps_to_mdps(data_raw_angular_rate[1]);
    angular_rate_mdps[2] =
      lsm6dso16is_from_fs2000dps_to_mdps(data_raw_angular_rate[2]);
    snprintf((char *)tx_buffer, sizeof(tx_buffer),
            "Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n",
            angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
    tx_com(tx_buffer, strlen((char const *)tx_buffer));
  }

  lsm6dso16is_temp_flag_data_ready_get(&dev_ctx, &drdy);

  if (drdy) {
    /* Read temperature data */
    memset(&data_raw_temperature, 0x00, sizeof(int16_t));
    lsm6dso16is_temperature_raw_get(&dev_ctx, &data_raw_temperature);
    temperature_degC =
      lsm6dso16is_from_lsb_to_celsius(data_raw_temperature);
    snprintf((char *)tx_buffer, sizeof(tx_buffer),
            "Temperature [degC]:%6.2f\r\n", temperature_degC);
    tx_com(tx_buffer, strlen((char const *)tx_buffer));
  }
}

/* Main Example --------------------------------------------------------------*/
void lsm6dso16is_read_data_drdy(void)
{
  lsm6dso16is_pin_int1_route_t val;

  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;
  /* Init test platform */
  platform_init();
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lsm6dso16is_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LSM6DSO16IS_ID)
    while (1);

  /* Restore default configuration */
  lsm6dso16is_software_reset(&dev_ctx);

  lsm6dso16is_pin_int1_route_get(&dev_ctx, &val);
  val.drdy_xl = 1;
  lsm6dso16is_pin_int1_route_set(&dev_ctx, val);

  /* Disable I3C interface */
  //lsm6dso16is_i3c_disable_set(&dev_ctx, LSM6DSO16IS_I3C_DISABLE);
  /* Enable Block Data Update */
  //lsm6dso16is_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set Output Data Rate */
  lsm6dso16is_xl_data_rate_set(&dev_ctx, LSM6DSO16IS_XL_ODR_AT_12Hz5_HP);
  lsm6dso16is_gy_data_rate_set(&dev_ctx, LSM6DSO16IS_GY_ODR_AT_12Hz5_HP);

  /* Set full scale */
  lsm6dso16is_xl_full_scale_set(&dev_ctx, LSM6DSO16IS_2g);
  lsm6dso16is_gy_full_scale_set(&dev_ctx, LSM6DSO16IS_2000dps);

  /* Configure filtering chain(No aux interface)
   * Accelerometer - LPF1 + LPF2 path
   */
  //lsm6dso16is_xl_hp_path_on_out_set(&dev_ctx, LSM6DSO16IS_LP_ODR_DIV_100);
  //lsm6dso16is_xl_filter_lp2_set(&dev_ctx, PROPERTY_ENABLE);

  /* Read samples in drdy handler */
  while (1);
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
  HAL_I2C_Mem_Write(handle, LSM6DSO16IS_I2C_ADD_L, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t*) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle,  LSM6DSO16IS_I2C_ADD_H & 0xFE, reg, (uint8_t*) bufp, len);
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
  HAL_I2C_Mem_Read(handle, LSM6DSO16IS_I2C_ADD_L, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LSM6DSO16IS_I2C_ADD_H & 0xFE, reg, bufp, len);
#endif
  return 0;
}

/*
 * @brief  Send buffer to console (platform dependent)
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
#if defined(NUCLEO_F401RE) | defined(STEVAL_MKI109V3)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
#endif
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}
