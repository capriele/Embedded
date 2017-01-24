/*
 * usbdisk.h
 *
 *  Created on: Jan 8, 2017
 *      Author: albertopetrucci
 */

#ifndef USBDISKCONTROLLER_H_
#define USBDISKCONTROLLER_H_

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/udma.h"
#include "driverlib/rom.h"
#include "driverlib/pin_map.h"
#include "utils/ustdlib.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include <string.h>

static bool FileInit(void);
void SysTickHandler(void);
static const char * StringFromFresult(FRESULT fresult);
static void MSCCallback(tUSBHMSCInstance *ps32Instance, uint32_t ui32Event, void *pvData);
void USBHCDEvents(void *pvData);
static int printFileStructure (void);
int usbEnableCreateFile(const TCHAR *path, char *content, UINT contentSize, int newFile);
int usbCreateFile();
void usbDiskInit(void);
void usbDiskMainLoop();

#endif /* USBDISKCONTROLLER_H_ */
