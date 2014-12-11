/*
 * Motor.h
 *
 * Created: 12/5/2014 9:57:38 AM
 *  Author: avasquez
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_

// Include the necessary libraries
#include <avr/io.h>
#include <avr/interrupt.h>

#include "semphr.h"

// PORTABLE PIN DEFINITIONS
#define MOTOR_OUT_PORT PORTC // PORTC is the motor output port for demo and mtrCW_Rotate table must be coded accordingly
#define HALL_A PINB0
#define HALL_B PINB1
#define HALL_C PINB2  // These pins must be set previously in the Peripherals Library .h file
	// Remember to set the interrupt registers properly in the source code file <<--- IMPORTANT!!!!

#define HALL_0_Vect PCINT0_vect	// Interrupt vector for first set of input pin interrupts
#define HALL_1_Vect PCINT2_vect	// Interrupt vector for second set of input pin interrupts

// Function prototypes
void motor_setup(void);
void vStartMotorTask( UBaseType_t uxPriority);

// CW Motor Lookup Table
extern char mtrCW_Rotate[8];
extern TickType_t clkElapsed;
extern char intPhase;
	// Global declaration of the motor drive handles
extern TaskHandle_t VMOtorRamp_Handle;
extern SemaphoreHandle_t motor_semaphore;

#endif /* MOTOR_H_ */