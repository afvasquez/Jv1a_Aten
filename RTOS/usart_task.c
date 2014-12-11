/*
 * usart_task.c
 *
 * Created: 11/22/2014 11:44:15 PM
 *  Author: avasquez
 */ 

/* Scheduler include files. */
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

/* Demo program include files. */
#include "ZPDC_LED_Peripherals.h"
#include "serial.h"
#include "usart_task.h"
#include "Motor.h"

/* The sequence transmitted is from comFIRST_BYTE to and including comLAST_BYTE. */
#define comFIRST_BYTE				( 'A' )
#define comLAST_BYTE				( 'D' )
#define comBUFFER_LEN				( ( UBaseType_t ) ( comLAST_BYTE - comFIRST_BYTE ) + ( UBaseType_t ) 1 )
#define menLINE_SIZE_HEADER			( 55  )
#define menLINE_SIZE_BODY			( 41  )
#define menCLEAR					(  8  )
#define menTextReady				(  4  )

/* We should find that each character can be queued for Tx immediately and we
don't have to block to send. */
#define comNO_BLOCK					( ( TickType_t ) 0 )

/* The Rx task will block on the Rx queue for a long period. */
#define comRX_BLOCK_TIME			( ( TickType_t ) 0xffff )

/* Handle to the com port used by both tasks. */
static xComPortHandle xPort = NULL; // Necessary to be able to send messages through the port
TaskHandle_t comTaskHandle;

/* The transmit task as described at the top of the file. */
static portTASK_FUNCTION_PROTO( vWelcomeTask, pvParameters );
//void fnPrintString(char* lnMessage, int lnSize); // Function Prototype
char txMode = 'N'; // Create and initialize a com port directive of NO_MODE
char txClearReset[7] = "\e[2J\e[H";
char txStartMessA[22] = "## Light Bastian RTOS\n";
char txStartMessB[28] = "R: Run\tS: Stop (Resets PWM)\n";
char txStartMessC[25] = "+: Speed UP\t-: Slow Down\n";
char txWater[5];
// This next variable holds the header to the UI. 

//////////////////////////////////////////////////////////////////////////
// Start the necessary tasks in the following function
void vStartSerialTask( UBaseType_t uxPriority, uint32_t ulBaudRate, UBaseType_t uxLED)
{	
	// Store the LED and Initialize the USART port - THIS was modded so that the LED is not needed
	(void) uxLED;
	xSerialPortInitMinimal(ulBaudRate, comBUFFER_LEN);
	
	// Start the Task
	xTaskCreate(vWelcomeTask,"T",configMINIMAL_STACK_SIZE*2,NULL,uxPriority, &comTaskHandle );
}

//////////////////////////////////////////////////////////////////////////
// WELCOME TASK Definition
// This function will clear and display the welcome screen on the terminal
static portTASK_FUNCTION( vWelcomeTask, pvParameters )
{
	// Just to stop compiler warnings.
	( void ) pvParameters;
	// UBaseType_t ubtWM;	// Only Neceesary when debugging through COM port
	
	fnPrintString(txClearReset,7);
	fnPrintString(txStartMessA,22);
	fnPrintString(txStartMessB,28);
	fnPrintString(txStartMessC,25);
	
	char rxChar;
		
	for( ;; )
	{
		// This is can also be the Rx code to interface with the board
		// Block until the first byte is received
		if (xSerialGetChar(xPort, &rxChar, comRX_BLOCK_TIME))
		{
			// DO the following ONLY if the motor is present and functional
			if ( intPhase < 7 && intPhase > 0 ) {
				if ( rxChar == 'r' || rxChar == 'R' ) {	// If the character was R-r (RUN)
					if ( txMode == 'N' ) {	// If the Motor is at Rest
						txMode = 'r';	// Switch Motor mode to RAMP up
						clkElapsed = 0;	// Reset the RAMP timer
					}
				} else if ( rxChar == 's' || rxChar == 'S' ) {	// If the character was S-s (STOP)
					if ( txMode == 'R' )	{	// If the motor is Running
						txMode = 's';	// Switch the mode to RAMP down
						clkElapsed = 0;	// Reset the RAMP timer
					}
				}
				
				// ####### Resume the Ramp Up Task
				vTaskResume( VMOtorRamp_Handle );
			} else {	// The motor is either not present OR it is not functional
				PORTC = mtrCW_Rotate[7];	// Clamp the Motor
				ledTaskRate = LED_TASK_RATE_ERROR;	// Set the Error Status Light
				txMode = 'N';	// Emphasize Motor Status of not-running
			}
		}
		
		//ubtWM = uxTaskGetStackHighWaterMark(NULL);  // GET watermark DATA
		//fnPrintWaterMark(ubtWM,2);
	}	
}

//////////////////////////////////////////////////////////////////////////
// The following is not a task, this is a function that will print the
//     given string on RS232
void fnPrintString(char* lnMessage, int lnSize)
{
	int i; // Loop counter
	for( i=0 ; i<lnSize ; i++ )
	{
		vTaskDelay(1);
		if( xSerialPutChar( xPort, *(lnMessage + i), comNO_BLOCK ) == pdPASS )
		{
			vParTestToggleLED((portBASE_TYPE) 6); // Blink while activity
		}
	}
	
	// Turn the LED off while we are not doing anything.
	vParTestSetLED((portBASE_TYPE) 6, pdFALSE);
}
char chTaskA[4] = "NOR\t";
char chTaskB[4] = "DPo\t";
char chTaskC[4] = "LPo\t";
char chTaskD[4] = "OPW\t";
char chTaskE[4] = "STO\t";
char chNL[2] = " \n";
void fnPrintWaterMark(UBaseType_t ubtValue, UBaseType_t ubtTask) {
	// Print the name
	if ( ubtTask == 1 ) {
		fnPrintString(chTaskA,4);
	} else if ( ubtTask == 2 ) {
		fnPrintString(chTaskB,4);
	} else if ( ubtTask == 3 ) {
		fnPrintString(chTaskC,4);
	} else if ( ubtTask == 4 ) {
		fnPrintString(chTaskD,4);
	} else if ( ubtTask == 5 ) {
	fnPrintString(chTaskE,4);
	}
	
	sprintf(txWater,"%d",ubtValue);
	if (ubtValue < 10)
	{
		fnPrintString(txWater,1);
	} else if (ubtValue < 100)
	{
		fnPrintString(txWater,2);
	} else if (ubtValue < 1000)
	{
		fnPrintString(txWater,3);
	}
	fnPrintString(chNL, 2);
}
