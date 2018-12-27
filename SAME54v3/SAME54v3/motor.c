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
void wait_motor(volatile int d);
void motor_hone(void);

/*Global variables */
volatile bool motor_on = false;
volatile bool catch = false;
volatile bool hone_done = false;
volatile int state;
volatile int steps;
volatile current_pos;
volatile char Homing_message_Arr[] = "Homing\n";
volatile char Honing_done_message_Arr[] = "Done\n";
volatile char *Homing_message_Ptr;
volatile bool accel = false;
volatile static int count;

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
	state = 0;
}

/*************************************************************
*    1st CW 400 steps to get out of slot					 *
*    2nd Look for pitch and catch                            *
*    3rd move to correct aperture (accelerating optional)    *
*************************************************************/
void motor_hone(void){
	Homing_message_Ptr = Homing_message_Arr;
	write_terminal(Homing_message_Ptr);
	accel = false;
	motor_state_machine(1, 0x400);	//0x400 steps CW
	motor_state_machine(2, 0x2500); 	//CCW look for picth and catch
	if(catch){	//pitch and catch was detected
		motor_state_machine(1, 0x96);	//0x96 steps CW to 1st aper
	}
	current_pos = 1;
	Homing_message_Ptr = Honing_done_message_Arr;
	write_terminal(Homing_message_Ptr);
	hone_done = true;
}

void select_aper(char aper){
	int aper_diff;
	switch(aper){
		
		case '0':
		motor_hone();
		break;
		
		case '1':
		if(!hone_done){motor_hone();}
		accel = true;
		aper_diff = 1 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW
		current_pos = 1;
		break;
		
		case '2':
		if(!hone_done){motor_hone();}
		accel = true;	
		aper_diff = 2 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 2;	
		break;
		
		case '3':
		if(!hone_done){motor_hone();}
		accel = true;	
		aper_diff = 3 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW		
		current_pos = 3;
		break;
		
		case '4':
		if(!hone_done){motor_hone();}
		accel = true;	
		aper_diff = 4 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 4;	
		break;
		
		case '5':
		if(!hone_done){motor_hone();}
		accel = true;	
		aper_diff = 5 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW	
		current_pos = 5;	
		break;
		
		case '6':
		if(!hone_done){motor_hone();}
		accel = true;	
		aper_diff = 6 - current_pos;
		if(aper_diff == 0){
			break;
		}
		if(aper_diff < 0){
			aper_diff *= -1;
			motor_state_machine(2, (0x340 * aper_diff));	//0x steps CCW
			break;
		}
		motor_state_machine(1, (0x340 * aper_diff));	//0x steps CW	
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

void motor_state_machine(int a, int b){
	Port *por = PORT;
	PortGroup *porD = &(por->Group[3]);
	volatile static int i = 100;
	state = a;
	steps = b;
	motor_on = true;
	volatile int phase = 0;
	
	porD->OUTSET.reg = EN;

	while(motor_on){
		
		switch(state){
			
			/* Brake State */
			case 0:
			count = 0;
			porD->OUTSET.reg = BRAKE;
			wait_motor(10);
			porD->OUTCLR.reg = EN;
			state = 5;	//go to idle state
			break;
			
			/* CW State */
			
			case 1:
			if(accel){
				if(count < 100 && i > 10){i--;}		
				else if((steps - count) < 100 && i < 100){i++;}
			}
		
			switch(phase){
				
				case  0:
				porD->OUTSET.reg = A;
				porD->OUTCLR.reg = B;
				wait_motor(i);
				count++;
				if(count == steps){state = 0;}	//go to brake state
				phase = 1;
				break;
			
				case 1:
				porD->OUTCLR.reg = A;
				wait_motor(i);
				count++;
				if(count == steps){state = 0;}	
				phase = 2;
				break;
				
				case 2:
				porD->OUTSET.reg = B;
				wait_motor(i);
				count++;
				if(count == steps){state = 0;}	
				phase = 3;
				break;
				
				case 3:
				porD->OUTSET.reg = A;
				wait_motor(i);
				count++;
				if(count == steps){state = 0;}	
				phase = 0;
				break;
			}
			break;
			
			
			
			/* CCW State */
			case 2:
			if(accel){
				if(count < 100 && i > 10){i--;}
				else if((steps - count) < 100 && i < 100){i++;}
			}
			
			switch(phase){
				
				case 0:
				porD->OUTSET.reg = A;
				porD->OUTSET.reg = B;
				wait_motor(i);
				count ++;
				if(count == steps){state = 0;}	//go to brake state
				phase = 1;
				break;
			
				case 1:
				porD->OUTCLR.reg = A;
				wait_motor(i);
				count ++;
				if(count == steps){state = 0;}
				phase = 2;
				break;
			
				case 2:
				porD->OUTCLR.reg = B;
				wait_motor(i);
				count ++;
				if(count == steps){state = 0;}
				phase = 3;
				break;
	
				case 3:
				porD->OUTSET.reg = A;
				wait_motor(i);
				count++;
				if(count == steps){state = 0;}
				phase = 0;
				break;
			}
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


