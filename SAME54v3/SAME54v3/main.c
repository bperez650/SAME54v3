/*
 * i changed double to float for temp calculation and eliminated the timer also for temp calculation
 * changed the menu, added row and deleted the '#' should not need it, so also deleted stuff in writeUart accordingly
 * changed DAC SSs might have a problem with these
 *
 * Must change digital to initialize all ports 
 * Must change DAC stuff for different IC 
 *
 * Using Termite Terminal
 * USing SAME54P20A mcu on XPlained Pro developement board
 *
 * Created: 10/15/2018 11:49:12 AM
 * Author : bryant
 */ 

#include "sam.h"
#include "temp.h"
#include "comm.h"
#include "lasers.h"
#include "motor.h"

/* Digital IO for port control */
#define D00 PORT_PB26
#define D01 PORT_PB27
#define D02 PORT_PB28
#define D03 PORT_PB29

/* Prototypes */
void clock_setup(void);
void port_setup(void);
void wait(volatile int d);
void port_control(void);
void convert(int *a);
void EIC_setup(void);
void DIP_switch_decode(void);

/************Terminal menu do not change or mess with ***************/
volatile char menu_array[19][90] = {	//DO NOT FUCK WITH THIS 
									{"\n\n_________________________________________________________                             \n\n"},
									{"                Artium Technologies, Inc Gen 3.0 Rev 0.0                                 \n"},
									{"_________________________________________________________                               \n\n"},
									{"M=> Show Menu                                                                            \n"},
									{"DxxXXX D=Analog Out x=Cannel Number 0 to 15 XXX=000 to 255                               \n"},
									{"KxxX K=Port K xx-Bit 0 to 15 X= State H or L                                             \n"},
									{"Fx  F=Aperture wheel  x=Position  1=25 2=50 3=100 4=250 5=500 6=1000                     \n"},
									{"Txxx  T=Tilt stage  xxx = Angle  015 to 165, 090 is Normal                               \n"},
									{"LxX  L=Laser  x=Laser 0 or 1  X=Enable or Disable 1 or 0                                 \n"},
									{"Bx B= Cal Board x=L or H  On or Off                                                      \n"},
									{"PxXXXXX  P=Laser Power  x=Laser 0 or 1  XXXXX=Laser Power 200mW = 0.200                  \n"},
									{"Gx  G=Mask Wheel  x=Position  ND;CR;TRANS                                                \n"},
									{"1=0.0;1.0;0.0;1.0 2=0.0;2.6;0.5;1.0 3=0.0;2.6;0.5;2.0 4=0.0;2.6;0.5;3.0 5=0.0;2.6;0.5;4.0\n"},
									{"Sx S=Beam Spacing x=0 Normal x=1 Reduced                                                 \n"},
									{"ExX E=Laser Information x=Laser 0 or 1 X=0 Hours 1=Fault 2=Power 3=Serial number         \n"},
									{"4=Drive current 5=On/Off state 6=Interlock state 7=Output power set point                \n"},
									{"8=Model number                                                                           \n"},
									{"T=> Show Current Board Temperature Status                                                \n"},
									{"C=> Show Current RTD Temperature Status\n\n#                                               "},
									};//DO NOT FUCK WITH THIS 
							
									
/* Global variables */									
volatile char *menu_ptr;
volatile char convert_array[4];
volatile char *convert_array_ptr;

	
int main(void){

	/* Initializing functions*/
    SystemInit();	
	clock_setup();
	port_setup();
	terminal_UART_setup();	
	SPI_setup();	
	ADC_0_Setup();
	ADC_1_Setup();
	rtd_TC_Setup();
	rtd_port_setup();
	COMM_Port_Setup();
	laser0_control_UART_setup();
	laser1_control_UART_setup();
 	laser_port_setup();
	laser_key_EIC_setup ();
	laser_key_delay_timer_setup();
 	laser_start_seq();
	motor_port_setup();
	motor_EIC_setup();
	 
	
	/* Writes "start" to terminal upon reset */

	volatile char start_array[] = "\nStart\n";
	volatile char *start_array_ptr;
	start_array_ptr = start_array;
	write_terminal(start_array_ptr);
	
	/* Decode the DIP switches unfinished*/
	//DIP_switch_decode();
	
	/* Hone Motors */
	motor_hone();

	/* Assign pointers */
	RTD_array_ptr = RTD_array;
	terminal_input_array_ptr = terminal_input_Array;
	menu_ptr = menu_array;
	DAC_array_ptr = DAC_array;
	convert_array_ptr = convert_array;
	laser0_response_array_ptr = laser0_response_array;
	laser1_response_array_ptr = laser1_response_array;
	
	/* Polling loop looking for Terminal request */
	while(1){	
	
		if(receive_key == 13){	//look for carriage return
		
			/* Menu Selection */
			if(((*terminal_input_array_ptr == 'm') || (*terminal_input_array_ptr == 'M')) && (receive_count = 1)){	
				write_menu(menu_ptr);
				receive_count = 0;
				receive_key = 0;
			}
			
			/*  BoardTemperature */
			else if(((*terminal_input_array_ptr == 't') || (*terminal_input_array_ptr == 'T')) && (receive_count = 1)){	
				ADC1->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS_AIN6;	//AIN6=PB04
				while (ADC1->SYNCBUSY.bit.INPUTCTRL){}	//wait for sync to complete
				ADC1->SWTRIG.reg = 1;	//trigger ADC to start conversion	//CHANGED
				while(ADC1->INTFLAG.bit.RESRDY == 0){}	//wait for conversion
				volatile int ADC_result = ADC1->RESULT.reg;	//read ADC conversion result
				volatile float result = (((float)ADC_result / 255) * 5);	//CHANGED float got changed from double
				result = (result - 1.375)/ .0225;
				int temp = (int)result;
				int *temp1 = &temp;
				convert(temp1);
				receive_count = 0;
				receive_key = 0;
			}
			
			/* RTD Display */
			else if(((*terminal_input_array_ptr == 'c') || (*terminal_input_array_ptr == 'C')) && (receive_count = 1)){
				display_RTDs();
				receive_count = 0;
				receive_key = 0;
			}
			
			/* Digital Out */
			else if(((*terminal_input_array_ptr == 'k') || (*terminal_input_array_ptr == 'K')) && (receive_count = 4)){	
				receive_count = 0;
				receive_key = 0;
				port_control();
			}
			
			/* Analog Out */
			else if(((*terminal_input_array_ptr == 'd') || (*terminal_input_array_ptr == 'D')) && (receive_count = 5)){	
				receive_count = 0;
				receive_key = 0;
				DAC_select();
			}
			
			/* Laser Information */
			else if(((*terminal_input_array_ptr == 'e') || (*terminal_input_array_ptr == 'E')) && (receive_count = 3)){
				receive_count = 0;
				receive_key = 0;
				laser_info_com();
			}
			
			/* Laser Control */
			else if(((*terminal_input_array_ptr == 'l') || (*terminal_input_array_ptr == 'L')) && (receive_count = 3)){
				receive_count = 0;
				receive_key = 0;
				laser_com();
			}
			
			/* Invalid Entry '?' */
			else{	
				volatile char Invalid_message_Arr[] = "\n?\n";
				volatile char *Invalid_message_Ptr;
				Invalid_message_Ptr = Invalid_message_Arr;
				write_terminal(Invalid_message_Ptr);
				receive_count = 0;
				receive_key = 0;
			}
		}
	}
}

/* CLock source is 12MHz divided to 1MHz */
void clock_setup(void){
	/* 12MHz crystal on board selected mapped to PB22/PB23 */
	OSCCTRL->XOSCCTRL[1].bit.ENALC = 1;	//enables auto loop ctrl to control amp of osc
	OSCCTRL->XOSCCTRL[1].bit.IMULT = 4;
	OSCCTRL->XOSCCTRL[1].bit.IPTAT = 3;
	OSCCTRL->XOSCCTRL[1].bit.ONDEMAND = 1;
	OSCCTRL->XOSCCTRL[1].bit.RUNSTDBY = 1;
	OSCCTRL->XOSCCTRL[1].bit.XTALEN = 1;	//select ext crystal osc mode
	OSCCTRL->XOSCCTRL[1].bit.ENABLE = 1;
	
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_XOSC1 | GCLK_GENCTRL_RUNSTDBY | !(GCLK_GENCTRL_DIVSEL) | GCLK_GENCTRL_OE | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_DIV(12);	//divide by 12 = 1MHz
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC_XOSC1 | GCLK_GENCTRL_RUNSTDBY | !(GCLK_GENCTRL_DIVSEL) | GCLK_GENCTRL_OE | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_DIV(1);	//divide by 1 = 12MHz
	while(GCLK->SYNCBUSY.reg){}	//wait for sync
	
	GCLK->PCHCTRL[7].bit.CHEN = 0;	//disable for safety first
	GCLK->PCHCTRL[7].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM0
	GCLK->PCHCTRL[36].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//SERCOM6
	GCLK->PCHCTRL[30].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//TC4/TC5
	GCLK->PCHCTRL[40].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//ADC0
	GCLK->PCHCTRL[41].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//ADC1
	GCLK->PCHCTRL[4].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;	//EIC
	GCLK->PCHCTRL[8].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1;	//SERCOM1
	GCLK->PCHCTRL[23].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1;	//SERCOM2


	MCLK->CPUDIV.reg = 1;	//divide by 1
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM0;	//unmask sercom0
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_SERCOM6;	//unmask sercom6
	MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC4;	//unmask TC4
	MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC5;	//unmask TC5
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_ADC1 | MCLK_APBDMASK_ADC0;//unmask ADC1
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_EIC;	//unmask EIC
	MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM1;	//unmask sercom1
	MCLK->APBBMASK.reg |= MCLK_APBBMASK_SERCOM2;	//unmask sercom2
	

}

void port_setup(void){
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	//12MHz crystal on board selected mapped to PB22/PB23

	//PORTs
	porB->DIRSET.reg = D00 | D01 | D02 | D03;	//PB26, PB27, PB28, PB29 //digital outputs
	
}

/* Selects digital port and gives it value */
void port_control(void){
	
		Port *por = PORT;
		PortGroup *porB = &(por->Group[1]);
		
	if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(terminal_input_array_ptr+1) - 48) * 10;
			zone += *(terminal_input_array_ptr+2) - 48;
			
			switch(zone){
				case 0:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D00;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D00;
				}
				break;
				
				case 1:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D01;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D01;
				}
				break;
				
				case 2:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D02;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D02;
				}
				break;
				
				case 3:
				if(*(terminal_input_array_ptr+3) == 'L' || *(terminal_input_array_ptr+3) == 'l'){
					porB->OUTCLR.reg = D03;
				}
				else if(*(terminal_input_array_ptr+3) == 'H' || *(terminal_input_array_ptr+3) == 'h'){
					porB->OUTSET.reg = D03;
				}
				break;
				
				case 4:
				break;
				case 5:
				break;
				case 6:
				break;
				case 7:
				break;
				case 8:
				break;
				case 9:
				break;
				case 10:
				break;
				case 11:
				break;
				case 12:
				break;
				case 13:
				break;
				case 14:
				break;
				case 15:
				break;
				
				default:
				break;
			}
		}
	}
}

void wait(volatile int d){
	int count = 0;
	while (count < d*1000){
		count++;
	}
}

/* Converts an int into a char so that it can be displayed*/

void convert(int *a){
	int y = *a;
	int i = 100;   //divisor
	int j = 0;  //array counter
	int m = 1;  //counter
	int n = 100;    //increment to divisor

	while(j <= 3){
		int b = y % i;
		if(b == y) {
			int p = (m-1);
			switch(p) {
				case 0:
				convert_array[j++] = '0';
				break;
				case 1:
				convert_array[j++] = '1';
				break;
				case 2:
				convert_array[j++] = '2';
				break;
				case 3:
				convert_array[j++] = '3';
				break;
				case 4:
				convert_array[j++] = '4';
				break;
				case 5:
				convert_array[j++] = '5';
				break;
				case 6:
				convert_array[j++] = '6';
				break;
				case 7:
				convert_array[j++] = '7';
				break;
				case 8:
				convert_array[j++] = '8';
				break;
				case 9:
				convert_array[j++] = '9';
				break;
				default:
				convert_array[j++] = 'A';
				break;
			}
			y = y - (n*(m-1));
			m = 1;

			if(j == 1){
				i = 10;
				n = 10;
			}
			if(j == 2){
				i = 1;
				n = 1;
			}
			
		}
		else{
			m++;
			i = i + n;
		}
	}
	convert_array[3] = 0;	//force pointer to end here
	write_terminal(convert_array_ptr);
}	

/* Decodes DIP swithces*/
void DIP_switch_decode(void){
	int n = 0;	//counter
	int total = 0;
	/* pulls enables Low, then checks the outputs for High values */
	//pull PJ0 low
	//check PB0-PB7 if high than add 2^n to total, incrementing n counter each time
	//at n = 7 and cheking PB7 pull pj0 high then pull pj1 low
	//check PB0-PB7 again for second DIP if high than add 2^n to total incrementing n counter each time
	//might need ot change to hex or something
	while(n != 16){
		
	}

}









