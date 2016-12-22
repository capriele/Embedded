#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_adc.h"
#include "inc/hw_types.h"
#include "inc/hw_udma.h"
#include "inc/hw_emac.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/emac.h"
#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"

//UART
#include "inc/hw_uart.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include <string.h>

//SPI
#include "inc/hw_ssi.h"
#include "driverlib/ssi.h"

//I2C
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"

//USB
#include "usbstick.h"
uint32_t currentFile;

//----------------------------------------------------------------
// DEFINITIONS
#define TARGET_IS_BLIZZARD_RB1
#define ADC_SAMPLE_BUF_SIZE (128)
#define TXBUF_SIZE         1
#define I2C0_SLAVE_ADDRESS 0xBB
static uint16_t TxBuf[TXBUF_SIZE];
uint32_t udmaCtrlTable[1024 / sizeof(uint32_t)]__attribute__((aligned(1024)));
uint16_t ADC_OUT[8];

//*****************************************************************************
//
// This function sets up UART0 to be used for a console to display information
// as the example is running.
//
//*****************************************************************************
void InitConsole(void) {
	//
	// Enable GPIO port A which is used for UART0 pins.
	// TODO: change this to whichever GPIO port you are using.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	//
	// Configure the pin muxing for UART0 functions on port A0 and A1.
	// This step is not necessary if your part does not support pin muxing.
	// TODO: change this to select the port/pin you are using.
	//
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);

	//
	// Enable UART0 so that we can configure the clock.
	//
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	//
	// Use the internal 16MHz oscillator as the UART clock source.
	//
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

	//
	// Select the alternate (UART) function for these pins.
	// TODO: change this to select the port/pin you are using.
	//
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	//
	// Initialize the UART for console I/O.
	//
	UARTStdioConfig(0, 115200, 16000000);
}

//*****************************************************************************
//
// Initializes the uDMA software channel to perform a memory to memory uDMA
// transfer.
//
//*****************************************************************************
void SSI0IntHandler(void){
    uint32_t ui32Status;

    // Read the interrupt status of the SPI.
    ui32Status = ROM_SSIIntStatus(SSI0_BASE, 1);//eval board

    // Clear any pending status, even though there should be none since no SPI
    // interrupts were enabled.  If SPI error interrupts were enabled, then
    // those interrupts could occur here and should be handled.  Since uDMA is
    // used for both the RX and TX, then neither of those interrupts should be
    // enabled. solo DMA-Int è enabled
    SSIIntClear(SSI0_BASE, ui32Status);
}

void uDMAErrorHandler(void) {
    unsigned long ulStatus;

    // Check for uDMA error bit
    ulStatus = ROM_uDMAErrorStatusGet();

    // If there is a uDMA error, then clear the error and continue.  If we're
    // still debugging our project, we want to infinte loop here so we can
    // investiage the failure cause.
    if(ulStatus) uDMAErrorStatusClear();
}
void ADCprocess(uint32_t ch) {
	if ((((tDMAControlTable *) udmaCtrlTable)[ch].ui32Control & UDMA_CHCTL_XFERMODE_M) != UDMA_MODE_STOP)
		return;
	uDMAChannelTransferSet(ch, UDMA_MODE_PINGPONG, (void *) (ADC0_BASE + ADC_O_SSFIFO0), &ADC_OUT, ADC_SAMPLE_BUF_SIZE);
	uDMAChannelEnable(UDMA_CHANNEL_ADC0);
}
void ADC0IntHandler(void) {
	ADCIntClear(ADC0_BASE, 0);
	ADCprocess(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);
	ADCprocess(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);
	TxBuf[0] = ADC_OUT[0];
}
void SW1_IntHandler(void) {
	if (GPIOIntStatus(GPIO_PORTF_BASE, false) & GPIO_PIN_4) {
		// PF4 was interrupt cause
		UARTprintf("SW1_IntHandler\n");
		char filename[8 + 9];
		sprintf(filename, "current_%08d", currentFile);
		USBStickOpenLogFile(filename);
		USBStickRun();
		tLogRecord psRecord;
		psRecord.ui32Seconds = 0;
		psRecord.ui16Subseconds = 0;
		psRecord.ui16ItemMask = 0;
		psRecord.pi16Items = ADC_OUT[0];
		USBStickWriteRecord(&psRecord);
		USBStickCloseFile();
		++currentFile;
	}
	GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
}

//----------------------------------------------------------------
// ADC INITIALIZATION
void InitADC() {

	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PIOSC | ADC_CLOCK_RATE_FULL, 1);
	// ADCClockConfigSet(uint32_t ui32Base, uint32_t ui32Config, uint32_t ui32ClockDiv)
	// ui32Base is the base address of the ADC configure and must always be ADC0_BASE
	// ui32Config is the value used to configure the ADC clock input. For TM4C123 device
	//     ADC_CLOCK_SRC_PIOSC is PLL/25 always thus 400MHz/25 = 16 MHz
	// ui32ClockDiv is the input clock divider which is in the range of 1 to 64

	ADCHardwareOversampleConfigure(ADC0_BASE, 64);
	// Oversampling average multiple samples from same analog input. Rates suported are
	//     2x, 4x, 8x, 16x, 32x, 64x. If set to 0 hardware oversampling is disabled
	SysCtlDelay(10); // Time for the clock configuration to set

	IntDisable(INT_ADC0SS0);
	ADCIntDisable(ADC0_BASE, 0);
	ADCSequenceDisable(ADC0_BASE, 0);
	// With sequence disabled, it is now safe to load the new configuration parameters

	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0);
	// ADCSequenceConfigure(uint32_t ui32Base, uint32_t ui32SequenceNum, uint32_t ui32Trigger,
	//     uint32_t ui32Priority)
	// ui32Base is the base address of the ADC configure and must always be ADC0_BASE
	// ui32SequenceNum is the sample sequence number (depends on sequencer used)
	// ui32Trigger is the trigger source that innitiates the sampling sequence, in this case
	//     the ADC is always running
	// ui32Priority is the relative priority with respect to other sample sequences 0 being
	//     the highest priority and 3 being the lowest

	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH1);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH3);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH4);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH5);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH9);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 7,
	ADC_CTL_CH11 | ADC_CTL_END | ADC_CTL_IE);
	// ADCSequenceStepConfigure(uint32_t ui32Base, uint32_t ui32SequenceNum, uint32_t ui32Step, uint32_t ui32Config)
	// ui32Base is the base address of the ADC configure and must always be ADC0_BASE
	// ui32SequenceNum is the sample sequence number (depends on sequencer used)
	// ui32Step is the step to be configured
	// ui32Config is the configuration of this step. Routs it to an analog input. Also defines the last in the
	//     sequence with ADC_CTL_END and also fires an interrupt every 8 samples with ADC_CTL_IE

	ADCSequenceEnable(ADC0_BASE, 0); //Once configuration is set, re-enable the sequencer
	ADCIntClear(ADC0_BASE, 0);

	// Configures the base address of the channel control table. Table resides in system memory and holds control
	//     information for each uDMA channel. Table must be aligned on a 1024-byte boundary. Base address must be
	//     configured before any of the channel functions can be used

	ADCSequenceDMAEnable(ADC0_BASE, 0);
	// Allows DMA requests to be generated based on the FIFO level of the sample sequencer (SS0)
	uDMAChannelAssign(UDMA_CH14_ADC0_0);
	uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0,
	UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);
	// Start with Ping-pong PRI side, low priority, unmask

	uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST);
	// Only allow burst transfers

	uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
	UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128);
	uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
	UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_128);
	// uDMAChannelControlSet(uint32_t ui32ChannelStructIndex, uint32_t ui32Control)
	// ui32Control is the logical OR of five values: the data size, the source address incremen, the destination address
	//     increment, the arbitration size and the burst flag

	uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
	UDMA_MODE_PINGPONG, (void *) (ADC0_BASE + ADC_O_SSFIFO0), &ADC_OUT,
	ADC_SAMPLE_BUF_SIZE);
	uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
	UDMA_MODE_PINGPONG, (void *) (ADC0_BASE + ADC_O_SSFIFO0), &ADC_OUT,
	ADC_SAMPLE_BUF_SIZE);
	// uDMAChannelTransferSet(uint32_t ui32ChannelStructIndex, uint32_t ui32Mode, void *pvSrcAddr, void *pvDstAddr, uint32_t ui32TransferSize)
	// pvSrcAddr is the source address for the transfer in this case from hw_adc.h ADC_O_SSFIFO0 is the result of the FIFO 0 register
	// pvDstAddr is the destination address for the transfer
	// ui32TransferSize is the number of data items to transfer

	ADCIntEnable(ADC0_BASE, 0);
	uDMAChannelEnable(UDMA_CHANNEL_ADC0); // Enables DMA channel so it can perform transfers
	SysCtlDelay(10);
	IntEnable(INT_ADC0SS0);

}
void InitSW1(void) {
	currentFile = 0;
	// Pin F4 setup
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);        // Enable port F
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);  // Init PF4 as input
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);  // Enable weak pullup resistor for PF4

	// Interrupt setup
	GPIOIntDisable(GPIO_PORTF_BASE, GPIO_PIN_4); // Disable interrupt for PF4 (in case it was enabled)
	GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_4); // Clear pending interrupts for PF4
	GPIOIntRegister(GPIO_PORTF_BASE, SW1_IntHandler); // Register our handler function for port F
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE); // Configure PF4 for falling edge trigger
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4);     // Enable interrupt for PF4
}
void InitSPI(void) {
	/* Abilita l'SPI a 1Mhz, 16bit
	 * Sui PIN:
	 * 			PA2 => SCK
	 * 			PA3 => CS/SS
	 * 			PA4 => RX (MiSo) (Non utilizzato)
	 * 			PA5 => Tx (MoSi)
	 */

	// SSI0 enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_SSI0);

	// GPIOA pins for SSI0 enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);
	GPIOPinTypeSSI(GPIO_PORTA_BASE,GPIO_PIN_5 | GPIO_PIN_3 | GPIO_PIN_2);

	// SSI0 config
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 1000000, 16);

	// SSI0 enable
	SSIEnable(SSI0_BASE);

	SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);

	uDMAChannelAttributeDisable(UDMA_CHANNEL_SSI0TX, UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);
	uDMAChannelAttributeEnable(UDMA_CHANNEL_SSI0TX, UDMA_ATTR_USEBURST);
	uDMAChannelControlSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_16 | UDMA_DST_INC_NONE | UDMA_ARB_4);
	uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT, UDMA_MODE_BASIC, TxBuf, (void *)(SSI0_BASE + SSI_O_DR), sizeof(TxBuf));
	uDMAChannelAssign(UDMA_CH11_SSI0TX);
	uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);

	// Enable the SPI peripheral interrupts.
	SSIIntEnable(SSI0_BASE, SSI_DMATX);
	IntEnable(INT_SSI0);
}
void InitI2C0(void)
{
    //enable I2C module 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    //reset module
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);

    //enable GPIO peripheral that contains I2C 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Configure the pin muxing for I2C0 functions on port B2 and B3.
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Select the I2C function for these pins.
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    IntEnable(INT_I2C0);
    I2CSlaveIntEnableEx(I2C0_BASE, I2C_SLAVE_INT_DATA);
    //I2CSlaveIntEnable(I2C0_BASE);
    I2CSlaveEnable(I2C0_BASE);
    I2CSlaveInit(I2C0_BASE, I2C0_SLAVE_ADDRESS);
}
void I2C0SlaveIntHandler(void)
{
    //I2CSlaveIntClear(I2C0_BASE);
	static uint8_t i2cCall = 0;
    // Clear the I2C0 interrupt flag.
    uint32_t status = I2CSlaveStatus(I2C0_BASE);
    //switch(status){
	//case I2C_SLAVE_ACT_RREQ:
	//case I2C_SLAVE_ACT_TREQ:
		if(i2cCall == 0){
			I2CSlaveDataPut(I2C0_BASE, (uint8_t)((ADC_OUT[0] >> 8) & 0xFF));
			i2cCall = 1;
		} else {
			I2CSlaveDataPut(I2C0_BASE, (uint8_t)((ADC_OUT[0] >> 0) & 0xFF));
			i2cCall = 0;
		}
	//	break;
    //}
}


void data_transfer(){
	//SPI
    uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
    						  UDMA_MODE_BASIC, TxBuf,
					      (void *) (SSI0_BASE + SSI_O_DR),
						  sizeof(TxBuf));
    uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);
}


int main(void) {

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	SysCtlDelay(20);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);
	IntEnable(INT_UDMAERR);
	uDMAEnable(); // Enables uDMA
	uDMAControlBaseSet(udmaCtrlTable);
	SysCtlDelay(10);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); //Enable the clock to ADC module
	SysCtlDelay(10); // Time for the peripheral enable to set

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x01);
	SysCtlDelay(30);

	InitConsole();
	UARTprintf("Init UART\n");

	UARTprintf("Init ADC\n");
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_1);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3);
	SysCtlDelay(80);
	InitADC();

	UARTprintf("Init SPI\n");
	InitSPI();

	UARTprintf("Init I2C\n");
	InitI2C0();

	// Initialize the stack to be used with USB stick.
	UARTprintf("Init USB Write\n");
	USBStickInit();
	InitSW1();

	IntMasterEnable();

	while (1) {

		//SPI
		data_transfer();
		//SSIDataPutNonBlocking(SSI0_BASE, ADC_OUT[0]);

		//UART
		UARTprintf("%d\r\n", ADC_OUT[0]);
	}
}
