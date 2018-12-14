#ifndef TEMP_H_INCLUDED
#define TEMP_H_INCLUDED

/* Global variable declarations */
extern int RTD_array[8];
extern int *RTD_array_ptr;

/* Prototypes for the functions */
void rtd_port_setup(void);
void ADC_0_Setup(void);
void ADC_1_Setup(void);
void rtd_TC_Setup(void);
void display_RTDs(void);
void check_RTDs(void);

#endif