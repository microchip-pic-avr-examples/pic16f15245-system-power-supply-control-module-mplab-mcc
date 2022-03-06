/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef APPLICATION_H
#define	APPLICATION_H

#include <xc.h> // include processor files - each processor file is guarded.  

#define REGULATOR_PIN   TRISAbits.TRISA2        //External Wakeup pin
#define WATCHDOG_TIME_DURATION  8               //Currenty set watchdog time period
#define I2C_DATA_LENGTH         7               //I2C Received data length
#define WAIT_PERIOD_30_SECS     30              //Wait period 
#define S 'S'                                   //Character S
#define DATA_LENGTH 8                           //Data length 
 
typedef enum
{
    OFF = 0,
    ON
}On_Off;

enum
{
    FALSE = 0,
    TRUE
};

enum
{
    CLEAR = 0,
    SET
};

struct Flag
{
    uint8_t i2cUnknownCommand  : 1;
    uint8_t i2cNewSleepCommand : 1;
    uint8_t regulatorOff       : 1;
    uint8_t startTimerTick1    : 1;
    uint8_t continueSleepMode  : 1;
}Flags;

typedef enum
{
    CLIENT_7_BIT_ADDR = 0x0A,
} client_t;

client_t currentClient;

uint16_t NoOfSleepCounts;
volatile uint16_t timerTickCount1=0;
volatile uint8_t i2cStartFlag;
volatile uint8_t sleepFlag = 1;


bool WaitForTime1(uint16_t NoOfSeconds);
bool ProcessI2CCommand(uint8_t *dataBuf, uint8_t len);
bool I2C_ClientInterruptHandler(i2c_client_transfer_event_t event);
void Application(void);
void TimerInterruptHandler(void);
void PinChangeInterruptHandler(void);
void GoToSleep(void);
void RegulatorControl(On_Off State);
void ClockSwitchingToLowFrequency(void);
void ClockSwitchingToSystemClock(void);
void I2C_EnableInterruptControl(void);
void I2C_DisableInterruptControl(void);
void WDT_Enable(void);
void WDT_Disable(void);
void TimerClockSelect(uint8_t clock);
void I2CAndSleepHandler(void);

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */

