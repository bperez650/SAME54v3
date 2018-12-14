#ifndef LASER_H_INCLUDED
#define LASER_H_INCLUDED

/* Global variable declarations */
extern volatile char laser0_response_array[];
extern volatile char *laser0_response_array_ptr;
extern volatile char laser1_response_array[];
extern volatile char *laser1_response_array_ptr;

/* Prototypes for the functions */
void laser0_control_UART_setup(void);
void laser1_control_UART_setup(void);
void laser_port_setup(void);
void laser_info_com(void);
void laser_com(void);
void laser_start_seq(void);
void laser_key_EIC_setup(void);
void laser_key_delay_timer_setup(void);
#endif