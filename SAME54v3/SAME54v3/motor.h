#ifndef MOTOR_H_INCLUDED
#define MOTOR_H_INCLUDED

/* Global variable declarations */
// extern int RTD_array[8];
// extern int *RTD_array_ptr;

/* Prototypes for the functions */
void motor_EIC_setup(void);
void motor_port_setup(void);
void select_aper(char aper);

#endif