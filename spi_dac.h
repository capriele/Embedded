/*
 * spi_dac.h
 *
 *  Created on: Dec 23, 2016
 *      Author: albertopetrucci
 */

#ifndef SPI_DAC_H_
#define SPI_DAC_H_

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"

#include "inc/hw_ssi.h"
#include "driverlib/ssi.h"

typedef struct spi_dac
{
  uint8_t dac_selector;        // 0 => DAC_A, 1 => DAC_B
  uint8_t buffer;              // 0 => Input Unbuffered, 1 => Input Buffered
  uint8_t output_gain;         // 0 => 2x (VOUT = 2 * VREF * D/4096), 1 =>  1x (VOUT = VREF * D/4096)
  uint8_t output_power_control;// 0 => Output buffer disabled, Output is high impedance, 1 => Output Power Down Control bit
} spi_dac_config;

void SSI0IntHandler(void);
void uDMAErrorHandler(void);
void SpiDACInit(spi_dac_config *dac);
void SpiDACWrite(spi_dac_config *dac, uint16_t sample);

#endif /* SPI_DAC_H_ */
