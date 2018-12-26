/****************************************************************************
* This file contains everything for the Motor controls
****************************************************************************/


#include "sam.h"
#include "motor.h"
#include <stdbool.h>

#define  A				PORT_PD08
#define  B				PORT_PD09
#define  BRAKE		    PORT_PD10
#define  EN				PORT_PD11

/* Prototypes */
void motor_state_machine(int a, int b);
void motor_accel(void);
void wait_motor(volatile int d);
void motor_hone(void);

/*Global variables */
volatile bool motor_on = false;
volatile bool catch = false;
volatile int state;
volatile int steps;
volatile current_pos;
volatile char Honing_message_Arr[] = "\nHoning\n";
volatile char Honing_done_message_Arr[] = "Done\n";
volatile char *Honing_message_Ptr;

/* Setup ports for Motors */
void motor_port_setup(void){
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	//PortGroup *porB = &(por->Group[1]);
	//PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	/* EIC pitch and catch */
	porA->PMUX[3].bit.PMUXE = 0;	//PA06 EXTINT[6]
	porA->PINCFG[6].bit.PMUXEN = 1;
	
	/* Motor */
	porD->DIRSET.reg = A | B | EN | BRAKE;
	porD->OUTCLR.reg = A | B | BRAKE | EN;

}

/* Pitch and catch EIC for Honing */
void motor_EIC_setup(void){
	EIC->CTRLA.reg = 0;
	while(EIC->SYNCBUSY.reg){}
	EIC->CTRLA.bit.CKSEL = 0;	//EIC is clocked by GCLK
	EIC->INTENSET.reg |= 1<<6;
	EIC->ASYNCH.reg |= 1<<6;	//asynchronous mode
	EIC->CONFIG[0].bit.SENSE6 = 4;	//high level detection
	EIC->CTRLA.reg = 1<<1;	//enable
	while(EIC->SYNCBUSY.reg){}
	NVIC->ISER[0] |= 1<<18;	//enable the NVIC handler
}

/* Pitch and catch EIC handler for Honing */
void EIC_6_Handler(void){
	EIC->INTFLAG.reg = 1<<6;	//clear int flag
	catch = true;
	steps = 0;
}

/*************************************************************
*    1st CW 400 steps to get out of slot					 *
*    2nd Look for pitch and catch                            *
*    3rd move to correct aperture (accelerating optional)    *
*************************************************************/
void motor_hone(void){
	Honing_message_Ptr = Honing_message_Arr;
	write_terminal(Honing_message_Ptr);
	motor_state_machine(1, 0x400);	//0x400 steps CW
	motor_state_machine(2, 0x2500); 	//CCW look for picth and catch
	if(catch){	//pitch and catch was detected
		motor_state_machine(1, 0x96);	//0x96 steps CW to 1st aper
	}
	current_pos = 1;
	Honing_message_Ptr = Honing_done_message_Arr;
	write_terminal(Honing_message_Ptr);
}

void motor_accel(void){
	
}

void motor_state_machine(int a, int b){
	Port *por = PORT;
	PortGroup *porD = &(por->Group[3]);
	volatile static int count;
	volatile static int i = 100;
	state = a;
	steps = b;
	motor_on = true;
	
	while(motor_on){
		
		switch(state){
			
			/* Brake State */
			case 0:
			porD->OUTSET.reg = BRAKE;
			wait_motor(10);
			porD->OUTCLR.reg = EN;
			state = 5;	//go to idle state
			break;
			
			/* CW State */
			case 1:
			porD->OUTSET.reg = EN;
			while(count < (steps - 3)){
				porD->OUTSET.reg = A;
				porD->OUTCLR.reg = B;
				wait_motor(i);
				porD->OUTCLR.reg = A;
				wait_motor(i);
				porD->OUTSET.reg = B;
				wait_motor(i);
				porD->OUTSET.reg = A;
				wait_motor(i);
				count += 4;
			}
			if(count != steps){
				porD->OUTSET.reg = A;
				porD->OUTCLR.reg = B;
				wait_motor(i);
				count++;
			}
			if(count != steps){
				porD->OUTCLR.reg = A;
				wait_motor(i);
				count++;
			}
			if(count != steps){
				porD->OUTSET.reg = B;
				wait_motor(i);
				count++;
			}
			if(count != steps){
				porD->OUTSET.reg = A;
				wait_motor(i);
				count++;
			}
			
			state = 0;	//go to brake state
			count = 0;
			break;
			
			/* CCW State */
			case 2:
			porD->OUTSET.reg = EN;
			while(count < (steps - 3)){
				porD->OUTSET.reg = A;
				porD->OUTSET.reg = B;
				wait_motor(i);
				porD->OUTCLR.reg = A;
				wait_motor(i);
				porD->OUTCLR.reg = B;
				wait_motor(i);
				porD->OUTSET.reg = A;
				wait_motor(i);
				count += 4;
			}
			if(count != steps){
				porD->OUTSET.reg = A;
				porD->OUTSET.reg = B;
				wait_motor(i);
				count ++;
			}
			if(count != steps){
				porD->OUTCLR.reg = A;
				wait_motor(i);
				count ++;
			}
			if(count != steps){
				porD->OUTCLR.reg = B;
				wait_motor(i);
				count ++;
			}
			if(count != steps){
				porD->OUTSET.reg = A;
				wait_motor(i);
			}
			
			count = 0;
			state = 0;	//go to brake state
			break;
			
			/* Acceleration CW State */
			case 3:
			while(i >= 10){
				
				porD->OUTSET.reg = A;
				porD->OUTCLR.reg = B;
				wait_motor(i);
				i--;
				porD->OUTCLR.reg = A;
				wait_motor(i);
				i--;
				porD->OUTSET.reg = B;
				wait_motor(i);
				i--;
				porD->OUTSET.reg = A;
				wait_motor(i);
				i--;
				count += 4;
			}
			state = 1;
			break;
		
		
			
			/* Decceleration CW State */
			case 4:
			while(i <= 100){
				
				porD->OUTSET.reg = A;
				porD->OUTCLR.reg = B;
				wait_motor(i);
				i++;
				porD->OUTCLR.reg = A;
				wait_motor(i);
				i++;
				porD->OUTSET.reg = B;
				wait_motor(i);
				i++;
				porD->OUTSET.reg = A;
				wait_motor(i);	
				i++;
				count += 4;
			}
			state = 1;
			break;
			
			
			/* Idle State */
			case 5:
			motor_on = false;
			break;
			
			default:
			break;
		}
	}
	
}



void select_aper(char aper){
	int aper_diff;
	switch(aper){
		
		case '0':
		motor_hone();
		break;
		
		case '1':
		aper_diff = 1 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW
		break;
		
		case '2':
		aper_diff = 2 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 2;	
		break;
		
		case '3':
		aper_diff = 3 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW		
		current_pos = 3;
		break;
		
		case '4':
		aper_diff = 4 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 4;	
		break;
		
		case '5':
		aper_diff = 5 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 5;	
		break;
		
		case '6':
		aper_diff = 6 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CW
		}
		motor_state_machine(3, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 6;	
		break;
		
		default:
		break;
	}
}

void wait_motor(volatile int d){
	int count = 0;
	while (count < d*10){
		count++;
	}
}



