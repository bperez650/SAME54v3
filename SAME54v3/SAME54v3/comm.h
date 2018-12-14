#ifndef COMM_H_INCLUDED
#define COMM_H_INCLUDED

/* Slave select lines for SPI */
#define SS0 PORT_PC06
#define SS1 PORT_PC07

/* Global variable declarations */
extern volatile int receive_count;	//receive array counter
extern volatile char terminal_input_Array[10];
extern volatile char receive_key;
extern volatile char DAC_array[2];
extern volatile char *DAC_array_ptr;
extern volatile char *terminal_input_array_ptr;

/* Prototypes for the functions */
void terminal_UART_setup(void);	//USART
void SPI_setup(void);	//SPI
void write_terminal(char *a);
void write_SPI(char *a);
void DAC_select(void);
void dacValue(void);
void COMM_Port_Setup(void);
void write_menu(char *a);

#endif