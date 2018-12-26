/****************************************************************************
* This file contains the UART for laser communication
* and the USB, one day
****************************************************************************/

////tiy//

#include <sam.h>
#include "lasers.h"
#include "comm.h"
#include <stdbool.h>

/* Global variables */
volatile char laser0_response_array[20];
volatile char laser1_response_array[20];
volatile char *laser0_response_array_ptr;
volatile char *laser1_response_array_ptr;
volatile bool EIC_pinstate = 0;


/* Must have a space delimiting all arguments */
volatile char laser_command0[] = "l0\r";	//disable laser off
volatile char laser_command1[] = "@cob1\r";	//restart laser
volatile char laser_command2[] = "cf\r";	//clear fault


volatile char laser_Scommand0[] = "@cobas 0\r";	//disable auto start
volatile char laser_Scommand1[] = "@cobasdr 0\r";	//disable direct ctrl

volatile char laser_Icommand0[] = "hrs?\r";	//get system operating hours
volatile char laser_Icommand1[] = "f?\r";	//get operating fault
volatile char laser_Icommand2[] = "pa?\r";	//read output power
volatile char laser_Icommand3[] = "sn?\r";	//get serial number //gsn?
volatile char laser_Icommand4[] = "i?\r";	//get drive current
volatile char laser_Icommand5[] = "l?\r";	//get laser ON/OFF state
volatile char laser_Icommand6[] = "ilk?\r";	//get interlock state
volatile char laser_Icommand7[] = "p?\r";	//get setpoint output power
volatile char laser_Icommand8[] = "glm?\r";	//get model number

volatile char *laser_command_ptr;

void laser_port_setup(void){
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	//12MHz crystal on board selected mapped to PB22/PB23
	
	/* SERCOM2 Laser1 */
	porB->PMUX[13].bit.PMUXE = 2;	//PB26 pad0 Tx
	porB->PINCFG[26].bit.PMUXEN = 1;	
	porB->PMUX[13].bit.PMUXO = 2;	//PB27 pad1 Rx
	porB->PINCFG[27].bit.PMUXEN = 1;
		
	/* SERCOM1 Laser0 */
	porC->PMUX[13].bit.PMUXO = 2;	//PC27 pad0 Tx
	porC->PINCFG[27].bit.PMUXEN = 1;	
	porC->PMUX[14].bit.PMUXO = 2;	//PC28 pad1 Rxm
	
	porC->PINCFG[28].bit.PMUXEN = 1;
	
	/* EXTINT[11] for Interlock/key */
	porA->PMUX[13].bit.PMUXO = 0;	//PA27 EXTINT[11]
	porA->PINCFG[27].bit.PMUXEN = 1;

/*
	/////////this is for testing on Dev board ///////////
	//////sercom 7//////
	porD->PMUX[4].bit.PMUXE = 2;	//PD08 pad0
	porD->PINCFG[8].bit.PMUXEN = 1;
	porD->PMUX[4].bit.PMUXO = 2;	//PD09 pad1
	porD->PINCFG[9].bit.PMUXEN = 1;
	////////////////////////////////////////////////////*/
	
}

/* Setup of UART for laser 0 contorl */
void laser0_control_UART_setup(void){
	Sercom *ser = SERCOM1;
	SercomUsart *uart0 = &(ser->USART);
	uart0->CTRLA.reg = 0;	//enable protected regs
	while(uart0->SYNCBUSY.reg){}
	uart0->CTRLA.bit.DORD = 1;	//LSB transferred first
	uart0->CTRLA.bit.CMODE = 0;	//asynchronous mode
	uart0->CTRLA.bit.SAMPR = 0;	//16x oversampling using arithmetic
	uart0->CTRLA.bit.RXPO = 1;	//RX is pad1 PC28
	uart0->CTRLA.bit.TXPO = 2;	//TX is pad0 PC27
	uart0->CTRLA.bit.MODE = 1;	//uart with internal clock
	uart0->CTRLB.bit.RXEN = 1;	//enable RX
	uart0->CTRLB.bit.TXEN = 1;	//enable TX
	uart0->CTRLB.bit.PMODE = 0;	//even parity mode
	uart0->CTRLB.bit.SBMODE = 0;	//1 stop bit
	uart0->CTRLB.bit.CHSIZE = 0;	//8bit char size
	while(uart0->SYNCBUSY.reg){}
	uart0->BAUD.reg = 63858;	//for fbaud 19200 at 12Mhz fref
	uart0->INTENSET.bit.RXC = 1;	//receive complete interr
	NVIC->ISER[1] |= 1<<20;	//enable sercom1 RXC int
	uart0->CTRLA.reg |= 1<<1;	//enable
	while(uart0->SYNCBUSY.reg){}
	
}

/* Setup of UART for laser 1 contorl */
void laser1_control_UART_setup(void){
 	Sercom *ser = SERCOM2;
	SercomUsart *uart1 = &(ser->USART);
	uart1->CTRLA.reg = 0;	//enable protected regs
	while(uart1->SYNCBUSY.reg){}
	uart1->CTRLA.bit.DORD = 1;	//LSB transferred first
	uart1->CTRLA.bit.CMODE = 0;	//asynchronous mode
	uart1->CTRLA.bit.SAMPR = 0;	//16x oversampling using arithmetic
	uart1->CTRLA.bit.RXPO = 1;	//RX is pad1 PB31	//pB17(testing)
	uart1->CTRLA.bit.TXPO = 2;	//TX is pad0 PB30	//pB16(testing)
	uart1->CTRLA.bit.MODE = 1;	//uart with internal clock
	uart1->CTRLB.bit.RXEN = 1;	//enable RX
	uart1->CTRLB.bit.TXEN = 1;	//enable TX
	uart1->CTRLB.bit.PMODE = 0;	//even parity mode
	uart1->CTRLB.bit.SBMODE = 0;	//1 stop bit
	uart1->CTRLB.bit.CHSIZE = 0;	//8bit char size
	while(uart1->SYNCBUSY.reg){}
	uart1->BAUD.reg = 63858;	//for fbaud 19200 at 12Mhz fref
	uart1->INTENSET.bit.RXC = 1;	//receive complete interr
	NVIC->ISER[2] |= 1<<12;	//enable sercom7 RXC int
	uart1->CTRLA.reg |= 1<<1;	//enable
	while(uart1->SYNCBUSY.reg){}
	
}



/* Handler for laser 1 rxc */
void SERCOM2_2_Handler(void){
	Sercom *ser = SERCOM2;
	SercomUsart *uart1 = &(ser->USART);
	static int i = 0;
	if(uart1->DATA.reg != 13){
		laser1_response_array[i++] = uart1->DATA.reg;
	}
	else{
		i = 0;
		write_terminal(laser1_response_array_ptr);
	}
}

/* Handler for laser 0 rxc */
void SERCOM1_2_Handler(void){
	Sercom *ser = SERCOM1;
	SercomUsart *uart0 = &(ser->USART);
	static int i = 0;
	if(uart0->DATA.reg != 13){
		laser0_response_array[i++] = uart0->DATA.reg;
	}
	else{
		i = 0;
		write_terminal(laser0_response_array_ptr);
	}
}

/* Laser Info Communication using UART */
void laser_info_com(void){
	char temp = *(terminal_input_array_ptr+2);
	
	/* select the laser 0 */
	if(*(terminal_input_array_ptr+1) == '0'){	
		switch(temp){
			case '0':
			laser_command_ptr = laser_Icommand0;
			write_laser('0', laser_command_ptr);
			break;
			
			case '1':
			laser_command_ptr = laser_Icommand1;
			write_laser('0', laser_command_ptr);
			break;
			
			case '2':
			laser_command_ptr = laser_Icommand2;
			write_laser('0', laser_command_ptr);
			break;
			
			case '3':
			laser_command_ptr = laser_Icommand3;
			write_laser('0', laser_command_ptr);
			break;
			
			case '4':
			laser_command_ptr = laser_Icommand4;
			write_laser('0', laser_command_ptr);
			break;
			
			case '5':
			laser_command_ptr = laser_Icommand5;
			write_laser('0', laser_command_ptr);
			break;
			
			case '6':
			laser_command_ptr = laser_Icommand6;
			write_laser('0', laser_command_ptr);
			break;
			
			case '7':
			laser_command_ptr = laser_Icommand7;
			write_laser('0', laser_command_ptr);
			break;
			
			case '8':
			laser_command_ptr = laser_Icommand8;
			write_laser('0', laser_command_ptr);
			break;
			
			default:
			break;
			
		}
	}
	/* select the laser 1 */
	if(*(terminal_input_array_ptr+1) == '1'){	
		switch(temp){
			case '0':
			laser_command_ptr = laser_Icommand0;
			write_laser('1', laser_command_ptr);
			break;
			
			case '1':
			laser_command_ptr = laser_Icommand1;
			write_laser('1', laser_command_ptr);
			break;
			
			case '2':
			laser_command_ptr = laser_Icommand2;
			write_laser('1', laser_command_ptr);
			break;
			
			case '3':
			laser_command_ptr = laser_Icommand3;
			write_laser('1', laser_command_ptr);
			break;
			
			case '4':
			laser_command_ptr = laser_Icommand4;
			write_laser('1', laser_command_ptr);
			break;
			
			case '5':
			laser_command_ptr = laser_Icommand5;
			write_laser('1', laser_command_ptr);
			break;
			
			case '6':
			laser_command_ptr = laser_Icommand6;
			write_laser('1', laser_command_ptr);
			break;
			
			case '7':
			laser_command_ptr = laser_Icommand7;
			write_laser('1', laser_command_ptr);
			break;
			
			case '8':
			laser_command_ptr = laser_Icommand8;
			write_laser('1', laser_command_ptr);
			break;
			
			default:
			break;
			
		}
	}
}

/* Laser Control Communication using UART */
void laser_com(void){
	
	/* select the laser 0 */
	if(*(terminal_input_array_ptr+1) == '0'){
		
		/* Disable Laser Command */
		if(*(terminal_input_array_ptr+2) == '0'){
			laser_command_ptr = laser_command0;
			write_laser('0', laser_command_ptr);
		}
		
		/* Enable Laser Command */
		else if(*(terminal_input_array_ptr+2) == '1'){
			laser_command_ptr = laser_command1;
			write_laser('0', laser_command_ptr);
		}
	}
	/* select the laser 1 */
	else if(*(terminal_input_array_ptr+1) == '1'){
		
		/* Disable Laser Command */
		if(*(terminal_input_array_ptr+2) == '0'){
			laser_command_ptr = laser_command0;
			write_laser('1', laser_command_ptr);
		}
		
		/* Enable Laser Command */
		else if(*(terminal_input_array_ptr+2) == '1'){
			laser_command_ptr = laser_command1;
			write_laser('1', laser_command_ptr);
		}
	}
}

/*
* Writes commands to lasers 
* Note all commands to laser are terminated by carriage return ASCI 13 
* Note all arguments are delimited by space ASCI 32 
* Note there are no arguments when getting laser info
*/
void write_laser(char a, char *b){
	if(a == '0'){
		Sercom *ser = SERCOM1;
		SercomUsart *uart0 = &(ser->USART);
		
		while(*b){
			while(!(uart0->INTFLAG.bit.DRE)){}
			uart0->DATA.reg = *b++;
			while((uart0->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
		}
		//uart0->DATA.reg = 13;
	}
	
	else if(a == '1'){
 		Sercom *ser = SERCOM2;
		SercomUsart *uart1 = &(ser->USART);
		 
		while(*b){
			while(!(uart1->INTFLAG.bit.DRE)){}
			uart1->DATA.reg = *b++;
			while((uart1->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
		}
		//uart1->DATA.reg = 13;
	}
	
} 
/* Laser Start Sequence */
void laser_start_seq(void){
	laser_command_ptr = laser_Scommand0;	//disable auto start
	write_laser('0', laser_command_ptr);
	write_laser('1', laser_command_ptr);
	laser_command_ptr = laser_Scommand1;	//disable direct ctrl
	write_laser('0', laser_command_ptr);
	write_laser('1', laser_command_ptr);
	laser_command_ptr = laser_command0;	//turn off lasers as a precautionary measure
	write_laser('0', laser_command_ptr);
	write_laser('1', laser_command_ptr);
}

/******************************************************
*  If laser is beig turned on or interlock is set
*  key must be toggled 
*  ***************************************************/
void laser_key_EIC_setup(void){
	EIC->CTRLA.reg = 0;
	while(EIC->SYNCBUSY.reg){}
	EIC->CTRLA.bit.CKSEL = 0;	//EIC is clocked by GCLK
	EIC->INTENSET.reg |= 1<<11;	
	EIC->ASYNCH.reg |= 1<<11;	//asynchronous mode
	//EIC->CONFIG[1].bit.SENSE3 = 1;	//rising edge detection
	
	/* Debouncing */
	EIC->CONFIG[1].bit.SENSE3 = 3;	//both edge detection
	EIC->ASYNCH.reg |= 0;	//synchronous mode
	EIC->DEBOUNCEN.reg |= 1<<11;
	EIC->DPRESCALER.bit.TICKON = 1;	//use prescaled clk (low freq)
	EIC->DPRESCALER.bit.STATES1 = 1;	//use 3 low freq samples to validate transistion
	EIC->DPRESCALER.bit.PRESCALER1 = 6;	//divide EIC by 64
	
	EIC->CTRLA.reg = 1<<1;	//enable
	while(EIC->SYNCBUSY.reg){}	
	NVIC->ISER[0] |= 1<<23;	//enable the NVIC handler 
}

/******************************************************
*  Handler acknowledges key toggling to turn on lasers
*  Writes commands to clear fault and turn laser ON
*  Delay ~ 1s after turn ON/OFF command to actual laser turn ON/OFF
*  ***************************************************/
void EIC_11_Handler(void){
	EIC->INTFLAG.reg = 1<<11;	//clear int flag
	Tc *tc = TC5;
	TcCount16 *tc5 = &tc->COUNT16;
	tc5->CTRLBSET.bit.CMD = 1;	//force retrigger of 1s timer
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
		
	/* Check pinstate of EXT[11], If high then turn on laser, If low then turn off laser */
// 	if(EIC->PINSTATE.reg == 0x800){
// 		EIC_pinstate = true;
// 	}
// 	else if(EIC->PINSTATE.reg == 0){
// 		EIC_pinstate = false;
// 	}
}

/* TC5 Creates a delay of ~1s when laser key is toggled until turn ON/OFF */
void laser_key_delay_timer_setup(void){	
	Tc *tc = TC5;
	TcCount16 *tc5 = &tc->COUNT16;
	tc5->CTRLA.reg = 0;	//disable the TC5
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
	tc5->CTRLA.bit.PRESCALER = 4;	//divide by 2^n;
	tc5->CTRLA.bit.MODE = 0;	//16 bit mode
	tc5->CTRLBSET.bit.ONESHOT = 1;	//turn on one shot mode
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
	tc5->INTENSET.bit.OVF = 1;	//enable the overflow interrupt
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
	tc5->CTRLA.reg |= 1<<1;	//enable the TC5
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
	tc5->CTRLBSET.bit.CMD = 2;	//stop. This is here because it starts counting right away and it needs to start by EIC
	NVIC->ISER[3] |= 1<<16;	//enable the NVIC handler for TC5
}

void TC5_Handler(void){
	Tc *tc = TC5;
	TcCount16 *tc5 = &tc->COUNT16;
	tc5->INTFLAG.bit.OVF = 1;	//clear the int flag
	while(tc5->SYNCBUSY.reg){}	//wait for sync of disable
		
	/* Write to lasers to restart */
	//if(EIC_pinstate){
		if(EIC->PINSTATE.reg == 0x800){
		/* Clear Fault */
		laser_command_ptr = laser_command2;
		write_laser('0', laser_command_ptr);
		write_laser('1', laser_command_ptr);		
		/* Turn Laser ON */
		laser_command_ptr = laser_command1;
		write_laser('0', laser_command_ptr);
		write_laser('1', laser_command_ptr);
	}
	
	else{
		/* Turn Laser OFF */
		laser_command_ptr = laser_command0;
		write_laser('0', laser_command_ptr);
		write_laser('1', laser_command_ptr);
	}

}