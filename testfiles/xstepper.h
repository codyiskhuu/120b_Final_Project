#ifndef XSTEEPER_H
#define XSTEEPER_H

/*****************************************
Configuration area
******************************************/

/*
Port where stepper is connected
*/
#define STEPPER_PORT 	C



/*
Position where 1st coil is connected.
All other coils like 2nd,3rd and 4th
must be connected in sequence after the 1st.

For this configuration the connection is
like:-

PC0 -> Coil 1
PC1 -> Coil 2
PC2 -> Coil 3
PC3 -> Coil 4

Don't connect port pins directly to coil,
use a driver like ULN2003A etc.

*/
#define STEPPER_POS		



/*
Don't touch the line below
*/

#define _CONCAT(a,b) a##b
#define PORT(x) _CONCAT(PORT,x)
#define PIN(x) _CONCAT(PIN,x)
#define DDR(x) _CONCAT(DDR,x)

#define __STEPPER_PORT 	PORT(STEPPER_PORT)
#define __STEPPER_DDR	DDR(STEPPER_PORT)

void StepperInit();
void StepperStepCW();
void StepperStepCCW();

#endif
