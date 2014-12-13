/*
 * Motor.c
 *
 * Created: 12/5/2014 10:05:35 AM
 *  Author: avasquez
 */

// Motor Control Definitions
#define MOTOR_POWER_BASE 255.0	// Starting power
#define MOTOR_MAX_POWER  0.0
#define MOTOR_MIN_POWER  0.0	// Minimum Running Power
#define MOTOR_TIME_DELTA 3000.0	// Number of time ticks
#define MOTOR_TIME_STEPS 1		// Ramp-up Time Resolution
#define MOTOR_POWER_STEP 1		// Rate at which the Power is delivered


/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "ZPDC_LED_Peripherals.h"

// Include the necessary pin name libraries 
#include "Motor.h"

// AUX function prototypes
// Motor Driving Task Function Prototype
static portTASK_FUNCTION_PROTO( vMotorDriveTask, pvParameters );
static portTASK_FUNCTION_PROTO( vMotorRampTask, pvParameters );

	// Local declaration of the motor drive handles
TaskHandle_t VMOtorRamp_Handle;

// Function Prototypes
void getPhaseCode(void);

// CW Motor Lookup Table
char mtrCW_Rotate[8] = { 0x3F, 0x26, 0x0D, 0x25, 0x13, 0x16, 0x0B, 0x3F };
// Current Motor Phase status
char intPhase = 0xFF;	// FF being the starting CODE
char motPower = MOTOR_MIN_POWER;
TickType_t clkElapsed;
float rate = ((MOTOR_POWER_BASE-MOTOR_MIN_POWER)/MOTOR_TIME_DELTA);
SemaphoreHandle_t motor_semaphore;
	
// Initialization function
void motor_setup(void) {
	PORTC = mtrCW_Rotate[7];
	// Check for the First HALL effect sensor state
	getPhaseCode();
	
	// Create the Binary semaphore
	motor_semaphore = xSemaphoreCreateBinary();
	vQueueAddToRegistry(motor_semaphore, "Motor Semaphore");
	
	// Reset the elapsed time (in thousands of a second)
	clkElapsed = 0x0000;	
	
	// Set the necessary features to make the motor run
	// Set the corresponding Fast PWM Pin
	// Enable the Port as an output
	DDRD |= (1 << PORTD3);
	
	// TIMER STUFF
	// Set the FAST PWM as non-inverting
	TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	// Set the Pre-Scaler
	TCCR2B = (1 << CS21);
	
	OCR2B = MOTOR_POWER_BASE; // Duty Cycle - Initialize to the very lowest base speed
	
	// Set the necessary PCINT settings for PB0-2
	PCICR |= (1 << PCIE0);    // set PCIE0 to enable PCMSK0 scan
	PCMSK0 |= 0b00000111;  // set PCINT0 to trigger an interrupt on state change
}

//////////////////////////////////////////////////////////////////////////
// MOTOR Drive Functions
// 
//////////////////////////////////////////////////////////////////////////
void vStartMotorTask( UBaseType_t uxPriority) {	
	// Start the Motor Function
	xTaskCreate( vMotorDriveTask, "M", configMINIMAL_STACK_SIZE*2, NULL, uxPriority+1, NULL );
	xTaskCreate( vMotorRampTask, "R", configMINIMAL_STACK_SIZE*2, NULL, uxPriority, &VMOtorRamp_Handle );
}

static portTASK_FUNCTION( vMotorDriveTask, pvParameters )
{
	char cInputs; // Getting the Input that governs the OVERCURRENT
	
	// Start into the infinite loop right away
	for (;;)
	{
		// Yield through its dedicated semaphore
		if (xSemaphoreTake(motor_semaphore, portMAX_DELAY))
		{
			cInputs = PIND & 0b10100000; // Get the Over-Current Flag
			if ((txMode == 'R' || txMode == 'r' || txMode == 's') && (cInputs & 0b00100000))
			{	// And Check for Overpowering!! -------------------------^
				// The motor is about to run in this routine ( CW Direction Only )
				PORTC = mtrCW_Rotate[( PINB & 0b00000111 )];
				} else if ( txMode == 'N' ) {	// If (In any case) the Motor State is N, Clamp down and set base PWM
				PORTC = mtrCW_Rotate[7];
				OCR2B = MOTOR_POWER_BASE;
				} else {	// OVERPOWERING SCENARIO
				PORTC = mtrCW_Rotate[7];	// CLAMP ASAP!!
				OCR2B = MOTOR_POWER_BASE;	// Reset to Base Power
				ledTaskRate = LED_TASK_RATE_ERROR;	// Clamp and throw error light
				txMode = 'N'; // Clamped state
			}
		}	
	}
}

//////////////////////////////////////////////////////////////////////////
// Motor Ramping Task
static portTASK_FUNCTION( vMotorRampTask, pvParameters ) {
	// Initialize the last time this task was woken
	TickType_t rampLastTick;
	const TickType_t xFrequency = 1;	// Ramp Algorithm Frequency when Ramping
	
	// ########## Suspend SELF
	vTaskSuspend( NULL );
	
	rampLastTick = xTaskGetTickCount();	// Record the last wake time
	
	for (;;)	// Loop this task FOREVER
	{
		if ( txMode == 'r' ) // This code does the ramp up
		{
			// Limit the giving of the semaphore to the first entry
			if (((OCR2B > motPower) && (OCR2B > MOTOR_MAX_POWER)) && OCR2B == MOTOR_POWER_BASE ) { 
				OCR2B = MOTOR_POWER_BASE - (clkElapsed * rate);
				xSemaphoreGive(motor_semaphore);	// Give the Semaphore
				vTaskDelayUntil(&rampLastTick, xFrequency);	// vTaskDelayUntil to the next millisecond
			} else if ((OCR2B > motPower) && (OCR2B > MOTOR_MAX_POWER)) { // (clkElapsed < MOTOR_TIME_DELTA) &&
				OCR2B = MOTOR_POWER_BASE - (clkElapsed * rate);
				vTaskDelayUntil(&rampLastTick, xFrequency);	// vTaskDelayUntil to the next millisecond
			} else if ( OCR2B < motPower ) {
				txMode = 'R';
					// If the motor is already at set speed, suspend the task
				vTaskSuspend( NULL );	// Suspend Self
			} else if ( OCR2B < MOTOR_MAX_POWER ) {
				txMode = 'R';
				// If the motor is already at set speed, suspend the task
				vTaskSuspend( NULL );	// Suspend Self
			} else {		// If the Ramp just simply timed out
				txMode = 'R';
				// If the motor is already at set speed, suspend the task
				vTaskSuspend( NULL );	// Suspend Self
			}
		} else if (txMode == 's') {	// If the motor is being stopped
			if (OCR2B >= MOTOR_POWER_BASE) {
				txMode = 'N';
				vTaskSuspend( NULL );
			} else {
				OCR2B = MOTOR_MIN_POWER + (clkElapsed * rate);
				vTaskDelayUntil(&rampLastTick, xFrequency);	// vTaskDelayUntil to the next millisecond
			}
		}   // If the motor is already running [R]; Don't do anything as of yet!
		
		
		
		// Instead, sent to COM
		//fnPrintWaterMark(OCR2B, exitMode);
		
		// %%%%%%%%%%% Debugging Purposes
		//ubtWM = uxTaskGetStackHighWaterMark(NULL);
		//fnPrintWaterMark(ubtWM,3);
	}
}

// This function builds the code sent by the motor on the different pins
void getPhaseCode(void) {
	intPhase = PINB & 0b00000111; // Clear the previous status
}

//////////////////////////////////////////////////////////////////////////
// ISR Functions
ISR (HALL_0_Vect) {
	// Define the woken task variable
	int motor_task_woken = 0;
	
	// Skip if in the "N" State
	if ( txMode != "N" ) {
		// Give the semaphore
		xSemaphoreGiveFromISR(motor_semaphore, &motor_task_woken);
		
		// If the task was woken
		if (motor_task_woken) {
			taskYIELD();
		}
	}
}

