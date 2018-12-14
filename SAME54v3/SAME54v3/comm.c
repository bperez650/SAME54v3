/****************************************************************************
* This file contains the UART for terminal communication 
* and the SPI communication for DACs
****************************************************************************/


#include "sam.h"
#include "comm.h"

/* Global variables */
volatile int receive_count = 0;	//receive array counter
volatile char terminal_input_Array[10] = {"0000000000"};
volatile char *terminal_input_array_ptr;
volatile char receive_key;
volatile char DAC_array[2];
volatile char *DAC_array_ptr;
volatile int slaveSel;

	
void COMM_Port_Setup(void){
	
	Port *por = PORT;
	PortGroup *porA = &(por->Group[0]);
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	PortGroup *porD = &(por->Group[3]);
	
	//12MHz crystal on board selected mapped to PB22/PB23
	
	/* SERCOM0 Uart Terminal */
	porA->PMUX[2].bit.PMUXE = 3;	//PA04 pad0 Tx
	porA->PINCFG[4].bit.PMUXEN = 1;	
	porA->PMUX[2].bit.PMUXO = 3;	//PA05 pad1 Rx
	porA->PINCFG[5].bit.PMUXEN = 1;
		
	/* SERCOM6 SPI to DACs */
	porC->PMUX[2].bit.PMUXE = 2;	//PC04 pad0
	porC->PMUX[2].bit.PMUXO = 2;	//PC05 pad1
	porC->PMUX[3].bit.PMUXE = 2;	//PC06 pad2
	porC->PINCFG[4].bit.PMUXEN = 1;
	porC->PINCFG[5].bit.PMUXEN = 1;
	porC->PINCFG[6].bit.PMUXEN = 1;
	porC->DIRSET.reg |= SS0;	//SS0 for 1st DAC
	porC->DIRSET.reg |= SS1;	//SS1 for 2nd DAC
	porC->OUTSET.reg |= SS0;	//initialize SS0 high
	porC->OUTSET.reg |= SS1;	//initialize SS1 high
}

/* Setup UART for terminal
*/
void terminal_UART_setup(void){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	uart->CTRLA.reg = 0;	//enable protected regs
	while(uart->SYNCBUSY.reg){}
	uart->CTRLA.bit.DORD = 1;	//LSB transferred first
	uart->CTRLA.bit.CMODE = 0;	//asynchronous mode
	uart->CTRLA.bit.SAMPR = 0;	//16x oversampling using arithmetic
	uart->CTRLA.bit.RXPO = 1;	//RX is pad1 PA05
	uart->CTRLA.bit.TXPO = 2;	//TX is pad0 PA04
	uart->CTRLA.bit.MODE = 1;	//uart with internal clock
	uart->CTRLB.bit.RXEN = 1;	//enable RX
	uart->CTRLB.bit.TXEN = 1;	//enable TX
	uart->CTRLB.bit.PMODE = 0;	//even parity mode
	uart->CTRLB.bit.SBMODE = 0;	//1 stop bit
	uart->CTRLB.bit.CHSIZE = 0;	//8bit char size
	while(uart->SYNCBUSY.reg){}
	uart->BAUD.reg = 55470;	//for fbaud 9600 at 1Mhz fref
	uart->INTENSET.bit.RXC = 1;	//receive complete interr
	NVIC->ISER[1] |= 1<<16;	//enable sercom0 RXC int
	uart->CTRLA.reg |= 1<<1;	//enable
	while(uart->SYNCBUSY.reg){}
}

/* Handler for receiving from terminal UART */
void SERCOM0_2_Handler(void){	//for recieving
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	receive_key = uart->DATA.reg;
	if(receive_key != 13){
		terminal_input_Array[receive_count++] = receive_key;
	}

}

/* Setup SPI for DACs*/
void SPI_setup(void){
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->CTRLA.reg = 0<<1;	//disable first
	while(spi->SYNCBUSY.reg){}
	spi->CTRLA.bit.DORD = 0;	//MSB first needed for AD5308
	//spi->CTRLA.bit.DORD = 1;	//LSB first needed for AD5308
	spi->CTRLA.bit.DOPO = 0;	//DO=pad0 PC04, SCK=pad1 PC05, SS=pad2 PC06
	spi->CTRLA.bit.FORM = 0;	//SPI frame form
	spi->CTRLA.bit.MODE = 3;	//master mode
	spi->CTRLB.bit.MSSEN = 0;	//software controlled SS
	spi->CTRLB.bit.CHSIZE = 0;	//8 bit char size
	while(spi->SYNCBUSY.reg){}
	//spi->BAUD.reg = 55470;	//9600bps at 1MHz
	spi->BAUD.reg = 51;	//9600bps at 1MHz
	spi->INTENSET.bit.TXC = 1;	//transmit complete
	//NVIC->ISER[2] |= 1<<7;	//enable sercom6 TXC int
	spi->CTRLA.reg |= 1<<1;	//enable
	while(spi->SYNCBUSY.reg){}
	
}

/*
void SERCOM6_1_Handler(void){	//obsolete right now
	//pull SS up again and end transaction
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	spi->INTFLAG.bit.TXC = 1;
}*/

/* Writes to terminal using UART */
void write_terminal(char *a){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	
	while(*a){
		while(!(uart->INTFLAG.bit.DRE)){}
		uart->DATA.reg = *a++;
		while((uart->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
	}
	uart->DATA.reg = 10;
}

void write_menu(char *a){
	Sercom *ser = SERCOM0;
	SercomUsart *uart = &(ser->USART);
	
	while(*a != '#'){
		while(!(uart->INTFLAG.bit.DRE)){}
		uart->DATA.reg = *a++;
		while((uart->INTFLAG.bit.TXC)==0){}	// waiting for transmit to complete
	}
	uart->DATA.reg = 10;
}

/* Writes through SPI protocol to DACs */
void write_SPI(char *a){
	//int SS;	//which dac
	volatile static int j = 0;	//counter
	Sercom *ser = SERCOM6;
	SercomSpi *spi = &(ser->SPI);
	Port *por = PORT;
	PortGroup *porB = &(por->Group[1]);
	PortGroup *porC = &(por->Group[2]);
	
	while( j<2 ){
		
		spi->DATA.reg = DAC_array[j];
		//spi->DATA.reg = 0xab;	//test
		while(spi->INTFLAG.bit.DRE == 0){}	//wait for DATA reg to be empty
		while(spi->INTFLAG.bit.TXC == 0){}	//wait for tx to finish
		j++;	//increment counter
		//if(j == 2){
		//spi->INTENCLR.bit.DRE = 1;	//clear the DRE flag
		//Port *por = PORT;
		//PortGroup *porA = &(por->Group[0]);
		//porA->OUTCLR.reg = SS;	//pull SS down
		//wait(1);
		//porA->OUTSET.reg = SS;	//pull SS up
		//}
	}
	wait(1);
	////// pulse SS (load) to clk data into dacs for TLV only
	if(slaveSel == 0){
		porC->OUTCLR.reg = SS0;
		//wait(1);
		porC->OUTSET.reg = SS0;
	}
	else if(slaveSel == 1){
		porB->OUTCLR.reg = SS0;
		//wait(1);
		porB->OUTSET.reg = SS0;
	}
	j = 0;
}


/* Selects which DAC out of 16 will be used */
void DAC_select(void){
	if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			volatile int zone = (*(terminal_input_array_ptr+1) - 48) * 10;
			zone += *(terminal_input_array_ptr+2) - 48;
			if(zone <= 7){
				DAC_array[0] = 2 * zone;	// 2 * zone is to accomodate TLV only
				slaveSel = 0;
			}
			else{
				DAC_array[0]= 2 * (zone - 8);	// 2 * zone is to accomodate TLV only
				slaveSel = 1;
			}
			dacValue();
			
		}
	}
}

/* Selects the value written to the DAC after it has been selected */
void dacValue(void){
	if((*(terminal_input_array_ptr+1) >= 48) && (*(terminal_input_array_ptr+1) <= 57)){	//looking for number keys only
		if((*(terminal_input_array_ptr+2) >= 48) && (*(terminal_input_array_ptr+2) <= 57)){	//looking for number keys only
			if((*(terminal_input_array_ptr+3) >= 48) && (*(terminal_input_array_ptr+3) <= 57)){	//looking for number keys only
				volatile int value = (*(terminal_input_array_ptr+3) - 48) * 100;
				value += (*(terminal_input_array_ptr+4) - 48) * 10;
				value += *(terminal_input_array_ptr+5) - 48;
				DAC_array[1] = value;
				//arrDACptr = arrDAC;
				write_SPI(DAC_array_ptr);
			}
		}
	}
	
}