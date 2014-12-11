/*
 * ZPDC_LED_Peripherals.h
 *
 * Created: 11/22/2014 12:26:32 PM
 *  Author: avasquez
 */ 


#ifndef ZPDC_LED_PERIPHERALS_H_
#define ZPDC_LED_PERIPHERALS_H_



#define partstDEFAULT_PORT_ADDRESS		( ( uint16_t ) 0x378 )

#define LED_TASK_RATE_BASE 0x0534
#define LED_TASK_RATE_ERROR 0x0133

void vParTestInitialise( void );
void vParTestSetLED( UBaseType_t uxLED, BaseType_t xValue );
void vParTestToggleLED( UBaseType_t uxLED );
void fnPrintString(char* lnMessage, int lnSize); // Function Prototype

extern TaskHandle_t ledTaskHandle;
extern TaskHandle_t comTaskHandle;
extern TickType_t ledTaskRate;
extern char txMode;

#endif /* ZPDC_LED_PERIPHERALS_H_ */

