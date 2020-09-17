#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"


#define Si7021_I2C_ADDRESS  0x40

#define SW_RESET                0xFE
#define Humidity_Measurement_CMD  0xF5
#define Temparature_Measurement_CMD   0xF3

uint32_t g_ui32SysClock;

void ConfigureUART(void)
{

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, g_ui32SysClock);
}


void I2CWriteCommand(uint32_t ui32Command)
{
    MAP_I2CMasterSlaveAddrSet(I2C0_BASE, Si7021_I2C_ADDRESS, false);
    MAP_I2CMasterDataPut(I2C0_BASE, ui32Command);
    MAP_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    while(MAP_I2CMasterBusy(I2C0_BASE))
    {
    }
}

void I2CReadCommand(uint32_t * DataRx)
{
    MAP_I2CMasterSlaveAddrSet(I2C0_BASE, Si7021_I2C_ADDRESS, true);
    MAP_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

    while(!MAP_I2CMasterBusy(I2C0_BASE))
    {
    }
    while(MAP_I2CMasterBusy(I2C0_BASE))
    {
    }

    DataRx[0] = MAP_I2CMasterDataGet(I2C0_BASE);

    MAP_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);

    while(!MAP_I2CMasterBusy(I2C0_BASE))
    {
    }
    while(MAP_I2CMasterBusy(I2C0_BASE))
    {
    }


    DataRx[1] = MAP_I2CMasterDataGet(I2C0_BASE);


    MAP_I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
		
    while(!MAP_I2CMasterBusy(I2C0_BASE))
    {
    }
    while(MAP_I2CMasterBusy(I2C0_BASE))
    {
    }
    
    DataRx[2] = MAP_I2CMasterDataGet(I2C0_BASE);
}


int main(void)
{
    float Temparature, Humidity;
    int32_t i32IntegerPart;
    int32_t i32FractionPart;
    uint32_t DataRx[3] = {0};


    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    ConfigureUART();

    UARTprintf("Temparature and the Humidity Measurement:\n\n");

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    MAP_I2CMasterInitExpClk(I2C0_BASE, g_ui32SysClock, false);

    I2CWriteCommand(SW_RESET);

    MAP_SysCtlDelay(g_ui32SysClock / (50 * 3));

    while(1)
    {

        I2CWriteCommand(Humidity_Measurement_CMD);

        MAP_SysCtlDelay(g_ui32SysClock / (30 * 3));
        
				I2CReadCommand(&DataRx[0]);

			  DataRx[0] = ((DataRx[0] << 8) | (DataRx[1]));
//        Humidity = (-6 + 125 * DataRx[0] / 65536);	
        Humidity = (float)(-6 + 125 * (float)DataRx[0] / 65536);
			
        i32IntegerPart = (int32_t) Humidity;
        i32FractionPart = (int32_t) (Humidity * 1000.0f);
        i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
        
				if(i32FractionPart < 0)
        {
            i32FractionPart *= -1;
        }

        UARTprintf("Humidity %3d.%03d\t", i32IntegerPart, i32FractionPart);
        I2CWriteCommand(Temparature_Measurement_CMD);
        MAP_SysCtlDelay(g_ui32SysClock / (11 * 3));

        I2CReadCommand(&DataRx[0]);

				DataRx[0] = ((DataRx[0] << 8) | (DataRx[1]));
//				Temparature =(-46.85 + 175.72 * DataRx[0]/65536);
        Temparature = (float)(-46.85 + 175.72 * (float)DataRx[0]/65536);

        i32IntegerPart = (int32_t) Temparature;
        i32FractionPart = (int32_t) (Temparature * 1000.0f);
        i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
        
				if(i32FractionPart < 0)
        {
            i32FractionPart *= -1;
        }

        UARTprintf("Temperature %3d.%03d\n", i32IntegerPart, i32FractionPart);

        MAP_SysCtlDelay(g_ui32SysClock / 3);
    }
}
