#include "mcc_generated_files/system/system.h"
#include "application.h"

volatile uint8_t timerFlag;
volatile uint8_t i2cStopFlag;
uint16_t regulatorOffDuration;
uint8_t i2c1RdData[DATA_LENGTH];
uint8_t i2c1RdDataIndex;

/*******************************************************************************
 * void Application(void)
 *
 * Wakes up from the sleep, when i2c command is received from the host, if i2c  
 * command is not received, enters sleep mode
 * @param void 
 * @return none
 ******************************************************************************/
void Application(void)
{
    //Check I2C command is received from host
    if(i2cStartFlag)
    {  
        sleepFlag = CLEAR;
        I2CAndSleepHandler();  
    }
      
    //Enter into sleep mode if I2C command is not received
    if(sleepFlag == SET)
    {  
       GoToSleep();
    } 
}

/*******************************************************************************
 * void I2CAndSleepHandler(void)
 *
 * Wakes up from the sleep, when i2c command is received from the host, if i2c  
 * command is not received, enters sleep mode
 * @param void 
 * @return none
 ******************************************************************************/
void I2CAndSleepHandler(void)
{
    //check if any I2C frame is received
    if(i2cStopFlag)				
    {
        i2cStopFlag = FALSE;
        //Process I2C data received and extract the sleep duration
        Flags.i2cNewSleepCommand = ProcessI2CCommand(i2c1RdData, i2c1RdDataIndex);
        i2c1RdDataIndex = CLEAR; //Clear the counter
        TimerClockSelect(4);
    }
        
    //Check if sleep command is received
    if(Flags.i2cNewSleepCommand)
    {
        //Timer Interrupt is generated for every 100ms and set the timerFlag
        if(timerFlag == 1)
        {
            timerFlag = CLEAR;    //Clear the timerFlag
             //wait for 60 seconds before turning of regulator
            Flags.regulatorOff = WaitForTime1(WAIT_PERIOD_30_SECS);  //60 seconds
        }
        else
        {
            GoToSleep();     //Sleep mode
        }
    }

    //initiate regulator off sequence
    if(Flags.regulatorOff)
    {
        Flags.i2cNewSleepCommand = FALSE;
        Flags.regulatorOff = FALSE;
        NoOfSleepCounts = CLEAR;    //Reset sleep counter
        I2C_DisableInterruptControl();           //Disable I2C peripherals to avoid wakeup
        RegulatorControl(OFF);  //Turn off the regulator
        Flags.continueSleepMode = TRUE;     //Set the sleep mode flag
        ClockSwitchingToLowFrequency();
        TimerClockSelect(0);
        WDT_Enable();
        CLRWDT();
    }
    
    //Put MCU into sleep mode till regulatorOffDuration is elapsed
    //As the Regulator off duration is of large magnitude repeated wakeup and sleep cycles are used
    //Watchdog is currently set at 8sec
    if(Flags.continueSleepMode)
    {
        GoToSleep();            //Sleep mode
        NoOfSleepCounts++;      //Count the number of wakeup instances to calculate total time consumed, after every 8 secs it will be incremented
        if(NoOfSleepCounts >= (regulatorOffDuration/WATCHDOG_TIME_DURATION)) // (regulatorOffDuration/WATCHDOG_TIME_DURATION) = (32/8) = 4
        {
            WDT_Disable();
            ClockSwitchingToSystemClock();
            Flags.continueSleepMode = FALSE;    //Sleep period is over
            RegulatorControl(ON);   //Turn on the regulator
            I2C_EnableInterruptControl();            //Turn on the I2C again
            NoOfSleepCounts = CLEAR;    //Reset sleep counter
            sleepFlag = SET;
            i2cStartFlag = CLEAR;
        }
    }
}

/*******************************************************************************
 * bool WaitForTime1(uint16_t NoOfSeconds)
 *
 * A non-blocking delay function
 * @param an integer argument for number of seconds delay needed
 * @return True on completion of delay, else false
 ******************************************************************************/
bool WaitForTime1(uint16_t NoOfSeconds)
{
    Flags.startTimerTick1 = TRUE;
    if(timerTickCount1 >= (NoOfSeconds * 10))
    {
        timerTickCount1 = CLEAR;
        Flags.startTimerTick1 = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
 * void GoToSleep(void)
 * 
 * Enters into sleep mode, by executing SLEEP() 
 * @param void
 * @return none
 ******************************************************************************/
void GoToSleep(void)
{  
    SLEEP();
}

/*******************************************************************************
 * void RegulatorControl(On_Off State)
 * 
 * Turn ON or OFF the regulator
 * @param ON/OFF enum variable
 * @return none
 ******************************************************************************/
void RegulatorControl(On_Off State)
{
    if(State == ON)
    {
        REGULATOR_PIN_SetLow();      //LED is used for simulation
    }
    else
    {
        REGULATOR_PIN_SetHigh();      //LED is used for simulation
    }
}

/*******************************************************************************
 * bool ProcessI2CCommand(uint8_t *dataBuf, uint8_t len)
 * 
 * Function to process I2C data received and extract the sleep duration
 * @param pointer to the I2C Rx data buffer
 * @param lenght of the I2C Rx data buffer
 * @return True if valid Sleep command is received, else false
 ******************************************************************************/
bool ProcessI2CCommand(uint8_t *dataBuf, uint8_t len)
{
    int i ;
    uint8_t tempDigit = 0;
    uint16_t tempTime = 0;
	//check if first byte received is "S" and length is less than 8
    if((S != *dataBuf) || (len > DATA_LENGTH))		//Invalid command
    {
        Flags.i2cUnknownCommand = TRUE;
    }
    else									//Valid command
    {
        Flags.i2cUnknownCommand = FALSE;
        regulatorOffDuration = CLEAR;
        for (i=1; i<len; i++)			//Convert sleep time from ASCII to Int
        {
            tempDigit = *(dataBuf+i);
            if(tempDigit >= '0' && tempDigit <= '9')//Check sleep time is between numbers range
            {
                tempDigit = tempDigit - '0'; 
                tempTime = tempTime*10 + tempDigit;
            }
        }
        regulatorOffDuration = tempTime;
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
 * void TimerInterruptHandler(void)
 * 
 * Timer 0 Interrupt callback function
 * Timer 0 is set at 100ms
 * @param none
 * @return none
 ******************************************************************************/
void TimerInterruptHandler(void)
{
    if(Flags.startTimerTick1) timerTickCount1++;	//Increment Tick Count if needed
    timerFlag = SET;                                  // Timer Interrupt Flag
}

/*******************************************************************************
 * void PinChangeInterruptHandler(void)
 * 
 * EXTI Interrupt callback function, 
 * PC2 (Switch Input) is trigger source
 * @param none
 * @return none
 ******************************************************************************/
void PinChangeInterruptHandler(void)
{
    //Ignition ON Interrupt, issue immediate wakeup to the HOST board
    Flags.continueSleepMode = FALSE;
    RegulatorControl(ON);   //Turn on the regulator
    I2C_EnableInterruptControl();     //Enable I2C and Interrupt again
    NoOfSleepCounts = CLEAR;    //Reset sleep counter
}

/*******************************************************************************
 * bool I2C_ClientInterruptHandler(i2c_client_transfer_event_t event)
 * 
 * I2C Client Read function, checks i2c address is matched and reads the data sent from the host
 * @param i2c_client_transfer_event_t event
 * @return True or False
 ******************************************************************************/
bool I2C_ClientInterruptHandler(i2c_client_transfer_event_t event)
{
    switch (event)
    {
    case I2C_CLIENT_TRANSFER_EVENT_ADDR_MATCH:
        
        if (I2C1_TransferDirGet() == I2C_CLIENT_TRANSFER_DIR_WRITE)
        {
            if (I2C1_ReadAddr() == CLIENT_7_BIT_ADDR)
            {
                currentClient = CLIENT_7_BIT_ADDR;
            }
        }
        break;

    case I2C_CLIENT_TRANSFER_EVENT_RX_READY:
            if (currentClient == CLIENT_7_BIT_ADDR)
            {
                i2c1RdData[i2c1RdDataIndex++] = I2C1_ReadByte();
            }
            if (i2c1RdDataIndex >= I2C_DATA_LENGTH)
            {
               i2cStartFlag = SET;
               i2cStopFlag = SET;
            }
        break;

    case I2C_CLIENT_TRANSFER_EVENT_STOP_BIT_RECEIVED:
        i2c1RdDataIndex = 0x00;
        break;

    default:
        break;
    }
    return true;
}

/*******************************************************************************
 * void ClockSwitchingToLowFrequency(void)
 * 
 * System clock switching to low frequency
 * @param void
 * @return none
 ******************************************************************************/
void ClockSwitchingToLowFrequency(void)
{
    OSCENbits.HFOEN = CLEAR;
    #define _XTAL_FREQ 31000
    OSCENbits.LFOEN = SET; 
}

/*******************************************************************************
 * void ClockSwitchingToSystemClock(void)
 * 
 * System clock switching to high frequency
 * @param void
 * @return none
 ******************************************************************************/
void ClockSwitchingToSystemClock(void)
{
    OSCENbits.LFOEN = CLEAR;
    OSCENbits.HFOEN = SET;
    OSCFRQ = 0x03;      
    #define _XTAL_FREQ 8000000
}

/*******************************************************************************
 * void I2C_DisableInterruptControl(void)
 * 
 * Disables I2C start and interrupt function
 * @param void
 * @return none
 ******************************************************************************/
void I2C_DisableInterruptControl(void)
{
    SSP1CON2 = 0x0;  //SEN disabled
    /* Disable Interrupts */
    PIE1bits.SSP1IE = CLEAR;
    PIE1bits.BCL1IE = CLEAR;
}

/*******************************************************************************
 * void I2C_EnableInterruptControl(void)
 * 
 * Enables I2C start and interrupt function
 * @param void
 * @return none
 ******************************************************************************/
void I2C_EnableInterruptControl(void)
{
    SSP1CON2 = 0x01;  //SEN enabled
    /* Enable Interrupts */
    PIE1bits.SSP1IE = SET;
    PIE1bits.BCL1IE = SET;
    
}

/*******************************************************************************
 * void WDT_Enable(void)
 * 
 * WDT Software Enable function
 * @param void
 * @return none
 ******************************************************************************/
void WDT_Enable(void)
{
    WDTCONbits.WDTSEN = SET;      //SWDTEN bit is enabled
}

/*******************************************************************************
 * void WDT_Disable(void)
 * 
 * WDT Software Disable function
 * @param void
 * @return none
 ******************************************************************************/
void WDT_Disable(void)
{
   WDTCONbits.WDTSEN = CLEAR;      //SWDTEN bit is disabled
}

/*******************************************************************************
 * void TimerClockSelect(uint8_t clock)
 * 
 * Selecting the clock source to the Timer 0 peripheral
 * @param void
 * @return none
 ******************************************************************************/
void TimerClockSelect(uint8_t clock)
{
   T0CON1bits.T0CS = clock;     //Clock source selection
}





