#ifndef __time_h__
#define __time_h__

void TimerOn();
void TimerOff();
void TimerISR ();
ISR(TIMER1_COMPA_vect);
void TimerSet(unsigned long M);
#endif