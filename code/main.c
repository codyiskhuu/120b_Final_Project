#include <avr/io.h>      //IO header
#define F_CPU 8000000UL			/* Define CPU Frequency e.g. here 8MHz */
#define F_CPU 11059200UL //defining crystal frequency
#include <avr/interrupt.h>
#include <util/delay.h>			/* Include Delay header file */
#include "avr/eeprom.h"


typedef enum {False, True} bool;


//======================Struct_Varibales======================//
typedef struct task {
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

//======================TaskSetting======================//
const unsigned char tasksNum = 5;
const unsigned long tasksPeriodGCD = 2;
task tasks[5];

//======================Task Periods======================//
const unsigned long matrixPeriod = 8;
const unsigned long sound = 870;
const unsigned long buttsPeriod = 100;
const unsigned long acceptPeriod = 8;
const unsigned long lcdPeriod = 8;

//======================Shared Variables======================//

//these are for the confirmation of button presses
static int once = 0;
static int tuwice = 0;
static int triple = 0;
static int quadra = 0;

static int onceTwice = 0;
static int onceTriple = 0;
static int onceQuadra = 0;

static int twiceTriple = 0;
static int twiceQuadra = 0;

static int tripleQuadra = 0;

static int confirmed = 0;


static int starter = 0;

static int scores = 48;
static int scores_1= 48;
static int scores_2= 48;
static int scores_3= 48;
static int scores_4= 48;

//======================Timer/Task scheduler======================//
volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	OCR1A = 15;    // Timer interrupt will be generated when TCNT1==OCR1A
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt
	TCNT1=0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}
//void TimerISR() { TimerFlag = 1; }
void TimerISR() {
	unsigned char i;
	for (i = 0; i < tasksNum; ++i) {                     // Heart of the scheduler code
		if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0;
		}
		tasks[i].elapsedTime += tasksPeriodGCD;
	}
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

//======================PWM Frequency======================//
void set_PWM(double frequency) {

	static double current_frequency; // Keeps track of the currently set frequency

	// Will only update the registers when the frequency changes, otherwise allows

	// music to play uninterrupted.

	if (frequency != current_frequency) {

		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter

		else { TCCR3B |= 0x03; } // resumes/continues timer/counter

		

		// prevents OCR3A from overflowing, using prescaler 64

		// 0.954 is smallest frequency that will not result in overflow

		if (frequency < 0.954) { OCR3A = 0xFFFF; }

		

		// prevents OCR0A from underflowing, using prescaler 64     // 31250 is largest frequency that will not result in underflow

		else if (frequency > 31250) { OCR3A = 0x0000; }

		

		// set OCR3A based on desired frequency

		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		

		TCNT3 = 0; // resets counter

		current_frequency = frequency; // Updates the current frequency

	}

}



void PWM_on() {

	TCCR3A = (1 << COM3A0);

	// COM3A0: Toggle PB3 on compare match between counter and OCR0A

	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);

	// WGM02: When counter (TCNT0) matches OCR0A, reset counter

	// CS01 & CS30: Set a prescaler of 64

	set_PWM(0);

}



void PWM_off() {

	TCCR3A = 0x00;

	TCCR3B = 0x00;

}
//======================LCD SCREEN DEFINES======================//
#define LCD_Direction  DDRD			/* Define LCD data port direction */
#define LCD_PortD PORTD 			/* Define LCD data port */
#define RS PD2				/* Define Register Select pin */
#define EN PD3 				/* Define Enable signal pin */
//======================LCD SCREEN======================//

void LCD_Commands( unsigned char cmnd )
{
	LCD_PortD = (LCD_PortD & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
	LCD_PortD &= ~ (1<<RS);		/* RS=0, command reg. */
	LCD_PortD |= (1<<EN);		/* Enable pulse */
	_delay_us(1);
	LCD_PortD &= ~ (1<<EN);

	_delay_us(200);

	LCD_PortD = (LCD_PortD & 0x0F) | (cmnd << 4);  /* sending lower nibble */
	LCD_PortD |= (1<<EN);
	_delay_us(1);
	LCD_PortD &= ~ (1<<EN);
	_delay_ms(2);
}


void LCD_Char( unsigned char data )
{
	LCD_PortD = (LCD_PortD & 0x0F) | (data & 0xF0); /* sending upper nibble */
	LCD_PortD |= (1<<RS);		/* RS=1, data reg. */
	LCD_PortD|= (1<<EN);
	_delay_us(1);
	LCD_PortD &= ~ (1<<EN);

	_delay_us(200);

	LCD_PortD = (LCD_PortD & 0x0F) | (data << 4); /* sending lower nibble */
	LCD_PortD |= (1<<EN);
	_delay_us(1);
	LCD_PortD &= ~ (1<<EN);
	//_delay_ms(2);
}

void LCD_Init(void)			/* LCD Initialize function */
{
	LCD_Direction = 0xFF;			/* Make LCD port direction as o/p */
	_delay_ms(20);			/* LCD Power ON delay always >15ms */
	
	LCD_Commands(0x02);		/* send for 4 bit initialization of LCD  */
	LCD_Commands(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Commands(0x0c);              /* Display on cursor off*/
	LCD_Commands(0x06);              /* Increment cursor (shift cursor to right)*/
	LCD_Commands(0x01);              /* Clear display screen*/
	_delay_ms(2);
}


void LCD_String (char *str)		/* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)		/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)	/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16){
	LCD_Commands((pos & 0x0F)|0x80);
	}	/* Command of first row and required position<16 */
	else if (row == 1 && pos<16){
	LCD_Commands((pos & 0x0F)|0xC0);
	}	/* Command of first row and required position<16 */
	LCD_Char(str);		/* Call LCD string function */
}
void LCD_String_xyz (char row, char pos, char *str)	/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16){
		LCD_Commands((pos & 0x0F)|0x80);
		}	/* Command of first row and required position<16 */
		else if (row == 1 && pos<16){
			LCD_Commands((pos & 0x0F)|0xC0);
			}	/* Command of first row and required position<16 */
			LCD_String(str);		/* Call LCD string function */
		}

void LCD_Clear()
{
	LCD_Commands (0x01);		/* Clear display */
	_delay_ms(2);
	LCD_Commands (0x80);		/* Cursor at home position */
}
void curser(int row, int call, int asci){
	LCD_String_xy(row, call, 9);
	LCD_Char(asci);
}


//======================Defines======================//
#define button_start  ~PINB & 0x10
#define button_next ~PINB & 0x20



#define one 0x03
#define two 0x0C
#define three 0x30
#define four 0xC0

#define button_1 ~PINB & 0x01
#define button_2 ~PINB & 0x02
#define button_3 ~PINB & 0x04
#define button_4 ~PINB & 0x08

bool ones = False;
//======================Matrix_Counters======================//
static int x=0;
static int a=0;
static int i=0;
//======================Scripting Notes======================//																			              //392.00, 329.63,        392.00,      329.63 ,   392.00,   0,    392.00, 440.00, 349.22, 349.22
																															 //33     //7 rest                                                             /11
char ALPHA[]= {0,0,0,0,0,0,0,0,0,0,0,0,  two, two,four, four, two, two, four, four, two, two, 0, two, two, two,two,one,one,one,one, 0,0,0,0,0,0,0,0,0,0,one, one,three, three, one, one, three, three, one, one, 0,  one, one, two,two, three,three,three,three,
	0,0,0,0,0,0,0,0,0,0,0

};			//0x01, 0x03
char PORT[8] = {1,2,4,8,16,32,64,128}; //pin values of a port 2^0,2^1,2^2……2^7
const unsigned int matrix_length =63;//63


static int succ_1=0;
static int succ_2=0;
static int succ_3=0;
static int succ_4=0;
enum LED_Display {start, wait, rows, speed, notes} matrix;
void LED_Tick(){
	switch(matrix){
		case start:
		matrix = wait;
		break;
		
		case wait:

			
			if(starter){
				starter = 1;
				matrix = rows;
				
			}
			else{
				starter = 0;
				matrix = wait;
			}
		break;
		
		case rows:
			if(x<matrix_length)//estimate on the number of the "notes"
			{
				matrix = speed;
			}
			else{
				starter = 0;
				x=0;
				matrix = wait;
			}
		break;
		
		case speed:
				if(a<6) //how fast this goes
				{
					matrix = notes;
				}
				else{
					a=0;
					++x;
					matrix = rows;
				}
				
		break;
		
		case notes:
			if(i<8)//8 rows
			{
				PORTC = PORT[i];    //ground the PORTC pin rows
				char temp = ALPHA[i+x];
				if(GetBit(temp, 0) &&GetBit(temp, 1) && i == 0 && button_1){
					succ_1=1;
					ones= True;
				}
				else if(GetBit(temp, 2) &&GetBit(temp, 3) && i == 0&& button_2){
					succ_2=1;
				}	
				else if(GetBit(temp, 4) &&GetBit(temp, 5) && i == 0&& button_3){
					succ_3=1;
				}	
				else if(GetBit(temp, 6) &&GetBit(temp, 7) && i == 0 && button_4){
					succ_4=1;
				}		
				else{
					succ_1=0;
					succ_2=0;
					succ_3=0;
					succ_4=0;	
				}
							
				PORTA = ~ALPHA[i+x];  //power the PORTA column
				i++;
				matrix = notes;
			}
			else{
				ones = False;
				i=0;
				a++;
				matrix = speed;
			}
		break;
		
		default:
		matrix = start;
		break;	
	}
	
	switch(matrix){
		case start:
		break;
		
		case wait:
		break;
		
		case rows:
		break;
		
		case speed:
		break;
		
		case notes:
		break;
		
	}	
	
	
	};


//double two;//					G4		E4	  G4		E4		G4	 break	G4		G4		G4		B4	   B4    [rest	5   ]		G4	    E4	      G4	   E4    G4	   break  G4		A4		F4    F4    [rest 7]			F4       E4       F4     E4      F4     break  F4   break   B4		B4   	D4   ||   D4		 ||    D4		D4
double array[] = {0,0,0,0,0,0,392.00, 329.63, 392.00, 329.63 ,392.00, 0,  392.00, 392.00, 392.00, 493.88, 493.88 , 0, 0, 0, 0,0, 392.00, 329.63, 392.00, 329.63 ,392.00,   0,    392.00, 440.00, 349.22, 349.22,/* 0, 0, 0, 0,0,0,0,   349.22, 329.63, 349.22, 329.63, 349.22,   0, 349.221,  0, 493.88, 493.88, 293.66, 0, 293.66, 0, 293.66, 0, 293.66,293.66, 293.66, 0,329.63*/ 0};
//int arraye [] = {0,0,0,0,0,0, two, four, two, four, two, 0,				 two,   two,    one,    one,  one          0,0,0,0,0,    one,     three, one,     three,  one,      0,   one, two, three,three,};
//two, four, two, four, two, 0,  two,two,one,one, 0,0,0,0,0, one,three, one, three,  one, 0,   one, two, three,three,	

unsigned char beat;//counter for the array
unsigned char measure;//manipulate the timer
unsigned char count;
enum song {init_2, wait_2, play} twice;

const int measures = 33;

void songs(){
	switch(twice){
		case init_2:
		twice = wait_2;
		break;
		///////////////////////////
		
		case wait_2:
		
		if(starter){//button is pressed then play song
			//PORTB =0x00;
			twice = play;
		}
		else{//turn it off
			//PORTB = 0x01;
			set_PWM(0);
			twice = wait_2;
		}
		break;
		///////////////////////////
		
		
		case play://9 notes right now //beat is the count// array[]
		
		if(count < measures){//timer is 60
			set_PWM(array[beat]);
			
			if(array[beat] != 0){////////////////////////////////////////////////////////////////////////////////////
				confirmed = 1;
			}
			else{
				confirmed = 0;
			}
			
			++beat;
			++count;
			
			twice = play;
		}
		else if(starter){
			set_PWM(0);
			twice = play;
		}
		else{
			twice = wait_2;
		}
		
		
		break;
		
		
		default:
		twice =init_2;
		break;
		
		
		
	}
	switch(twice){
		case init_2:
		break;
		
		case wait_2:
		beat = 0;// reset the song
		count = 0;
		break;
		
		case play:
		break;
		
		
	}
	
	
}




enum input_ticc {input_start, wait_input, input_1, input_2, input_3, input_4 , input_12, input_13, input_14, input_21, input_23, input_24, input_31, input_32, input_34, input_41, input_42, input_43} inputs;


void input_tick(){
	switch(inputs){
		case input_start:
		inputs = wait_input;
		break;
		
		case wait_input:
		if(button_1){
			inputs = input_1;
		}
		else if(button_2){
			inputs = input_2;
		}
		else if(button_3){
			inputs = input_3;
		}
		else if(button_4){
			inputs = input_4;
		}
		else{
			inputs = wait_input;
		}
		
		break;
		
		case input_1:
		if(button_1 && button_2){
			inputs = input_12;
		}
		else if(button_1 && button_3){
			inputs = input_13;
		}
		else if(button_1 && button_4){
			inputs = input_14;
		}
		else if(button_1){
			inputs = input_1;
		}
		else{
			inputs = wait_input;
		}
		break;
		case input_2:
		if(button_2 && button_1){
			inputs = input_21;
		}
		else if(button_2 && button_3){
			inputs = input_23;
		}
		else if(button_2 && button_4){
			inputs = input_24;
		}
		else if(button_2){
			inputs = input_2;
		}
		else{
			inputs = wait_input;
		}
		break;
		case input_3:
		if(button_3 && button_1){
			inputs = input_31;
		}
		else if(button_2 && button_3){
			inputs = input_32;
		}
		else if(button_3 && button_4){
			inputs = input_34;
		}
		else if(button_3){
			inputs = input_3;
		}
		else{
			inputs = wait_input;
		}
		break;
		case input_4:
		if(button_4 && button_1){
			inputs = input_41;
		}
		else if(button_4 && button_2){
			inputs = input_42;
		}
		else if(button_3 && button_4){
			inputs = input_43;
		}
		else if(button_4){
			inputs = input_4;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_12:
		if(button_1 && button_2){
			inputs = input_12;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_13:
		if(button_1 && button_3){
			inputs = input_13;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_14:
		if(button_1 && button_4){
			inputs = input_14;
		}
		else{
			inputs = wait_input;
		}
		break;

		case input_21:
		if(button_2 && button_1){
			inputs = input_21;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_23:
		if(button_2 && button_3){
			inputs = input_23;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_24:
		if(button_2 && button_4){
			inputs = input_24;
		}
		else{
			inputs = wait_input;
		}
		break;

		case input_31:
		if(button_3 && button_1){
			inputs = input_31;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_32:
		if(button_2 && button_3){
			inputs = input_32;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_34:
		if(button_3 && button_4){
			inputs = input_34;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_41:
		if(button_4 && button_1){
			inputs = input_41;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_42:
		if(button_4 && button_2){
			inputs = input_42;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		case input_43:
		if(button_4 && button_3){
			inputs = input_43;
		}
		else{
			inputs = wait_input;
		}
		break;
		
		
		default:
		inputs = input_start;
		break;
		
	}
	
	switch(inputs){
		case input_start:
		break;
		case wait_input:
		once = 0;
		tuwice = 0;
		triple = 0;
		quadra = 0;
		onceTwice = 0;
		onceTriple = 0;
		onceQuadra = 0;
		twiceTriple = 0;
		twiceQuadra = 0;
//		PORTB = 0x00;
		break;
		case input_1:
		once = 1;
//		PORTB = 0x01;
		break;
		case input_2:
		tuwice = 1;
//		PORTB = 0x02;
		break;
		case input_3:
		triple = 1;
//		PORTB = 0x04;
		break;
		case input_4:
		quadra = 1;
//		PORTB = 0x08;
		break;
		
		case input_12:
		onceTwice = 1;
//		PORTB = 0x03;
		break;
		
		case input_13:
		onceTriple = 1;
//		PORTB = 0x05;
		break;
		
		case input_14:
		onceQuadra = 1;
//		PORTB = 0x09;
		break;

		case input_21:
		onceTwice = 1;
//		PORTB = 0x03;
		break;
		
		case input_23:
		twiceTriple = 1;
//		PORTB = 0x06;
		break;
		
		case input_24:
		twiceQuadra = 1;
//		PORTB = 0x0A;
		break;
		
		case input_31:
		onceTriple = 1;
//		PORTB = 0x05;
		break;
		
		case input_32:
		twiceTriple = 1;
//		PORTB = 0x06;
		break;
		
		case input_34:
		tripleQuadra = 1;
//		PORTB = 0x0C;
		break;
		
		case input_41:
		onceQuadra = 1;
//		PORTB = 0x09;
		break;
		
		case input_42:
		twiceTriple = 1;
//		PORTB = 0x0A;
		break;
		
		case input_43:
		tripleQuadra = 1;
//		PORTB = 0x0C;
		break;
	}
	
	
	
	
	
}

//0,0,0,0,0,0,392.00,
//int arraye [] = {0,0,0,0,0,0, two, four, two, four, two, 0,  two,two,one,one,one, 0,0,0,0,0, one,three, one, three,  one, 0,   one, two, three,three};
enum accept_ticc{start_accept, wait_accept, proccess_accept} accept;
int counters = 0; 


void accept_tick(){
	switch(accept){
		case start_accept:
			accept = wait_accept;
		break;
		
		case wait_accept:
			if(GetBit(PORTC, 0)){
				//PORTB = 0x10;
			}
			else{
				//PORTB = 0x00;
			}
			
			accept = wait_accept;
		
		break;
		
		case proccess_accept:
			

			
		break;
		
		
		
		
		
	}
	switch(accept){
		case start_accept:
		
		break;
		
		case wait_accept:
			counters = 0;
		break;
		
		case proccess_accept:
		
		break;
		
		
		
	}
	
	
}

enum LCD_Display{start_lcd, wait_lcd, play_lcd, end_lcd, wait_end_lcd ,high_lcd, wait_high_lcd, wait_again_lcd, one_lcd,two_lcd,three_lcd,four_lcd, h1ghr_lcd, everyday_lcd} lsd;
/*#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))
void LCD_WriteCommand (unsigned char Command) {
	CLR_BIT(LCD_PortD,RS);
	LCD_PortD = Command;
	SET_BIT(LCD_PortD,EN);
	asm("nop");
	CLR_BIT(LCD_PortD,EN);
	_delay_ms(2); // ClearScreen requires 1.52ms to execute
}

void LCD_Cursor(unsigned char column) {
	if ( column < 17 ) { // 16x1 LCD: column < 9
		// 16x2 LCD: column < 17
		LCD_WriteCommand(0x80 + column - 1);
		} else {
		LCD_WriteCommand(0xB8 + column - 9);	// 16x1 LCD: column - 1
		// 16x2 LCD: column - 9
	}
}
*/
int oness = 10;//10
int twos= 9;
int threes=8;
int fours=7;//7 stop
int fives =6;

unsigned char First=0;
unsigned char Second = 0;
unsigned char Third = 0;
unsigned char Fourth = 0;
unsigned char Fifth = 0;

unsigned int high_one = 48;
unsigned int high_two = 48;
unsigned int high_three = 48;
unsigned int high_four = 48;
unsigned int high_five = 48;



int checkers = 0;
void LCD_tick(){
	switch(lsd){
		case start_lcd:

		lsd = wait_lcd;
		break;
		
		case wait_lcd:
		if(button_start && ~button_next  ){
			starter = 1;
			LCD_Commands(0x01);
			LCD_String("Song: TT");
			LCD_Commands(0xC0);
			LCD_String("Score:");
			LCD_String("00000");
			lsd = play_lcd;
		}
		else if(button_next){
			//PORTB = 0x80;
			LCD_Commands(0x01);
			LCD_String("High Score:");
			
			if(high_one >48 || high_two > 48 || high_three>48 || high_four>48 || high_five>48){//only problem is to make it work if the highscore isnt working
				LCD_Char(high_five);
				LCD_Char(high_four);
				LCD_Char(high_three);
				LCD_Char(high_two);
				LCD_Char(high_one);
			}
			
			
			////////////this shit is okay
			if(Fifth != 48 &&Fifth != 49&&Fifth != 50&&Fifth != 51&&Fifth != 52&&Fifth != 53&&Fifth != 54&&Fifth != 55&&Fifth != 56&&Fifth != 57 ){
				eeprom_update_byte((uint8_t*) 13, (uint8_t) 48);
				LCD_Char(48);
			}
			else{
				LCD_Char(Fifth);
			}
			if(Fourth != 48 &&Fourth != 49&&Fourth != 50&&Fourth != 51&&Fourth != 52&&Fourth != 53&&Fourth != 54&&Fourth != 55&&Fourth != 56&&Fourth != 57 ){
				eeprom_update_byte((uint8_t*) 10, (uint8_t) 48);
				LCD_Char(48);
			}
			else{
				LCD_Char(Fourth);
			}
			if(Third != 48 &&Third != 49&&Third != 50&&Third != 51&&Third != 52&&Third != 53&&Third != 54&&Third != 55&&Third != 56&&Third != 57 ){
				eeprom_update_byte((uint8_t*) 7, (uint8_t) 48);
				LCD_Char(48);
			}
			else{
				LCD_Char(Third);
			}
			if(Second != 48 &&Second != 49&&Second != 50&&Second != 51&&Second != 52&&Second != 53&&Second != 54&&Second != 55&&Second != 56&&Second != 57 ){
				eeprom_update_byte((uint8_t*) 4, (uint8_t) 48);
				LCD_Char(48);
			}
			else{
				LCD_Char(Second);
			}

			if(First != 48 && First != 49&&First != 50&&First != 51&&First != 52&&First != 53&&First != 54&&First != 55&&First != 56&&First != 57 ){
				eeprom_update_byte((uint8_t*) 1, (uint8_t) 48);
				LCD_Char(48);
			}
			else{
				LCD_Char(First);
			}
					
			
			
			LCD_Commands(0xC0);
			LCD_String("-Play-Menu");
			lsd = high_lcd;
		}
		else{
			
			lsd = wait_lcd;
		}
		break;
		
		case play_lcd:
		if(button_1 && succ_1){
			lsd = one_lcd;
		}
		else if(button_2 && succ_2){
			lsd = two_lcd;
		}
		else if(button_3 && succ_3){
			lsd = three_lcd;
		}
		else if(button_4 && succ_4){
			lsd = four_lcd;
		}
		else if(starter){
			lsd = play_lcd;
		}
		else{
			
			LCD_Commands(0x80);
			
			LCD_String("Total Score:");///////////////////////////////////////////////
			LCD_String_xyz(1,11,"");
			LCD_String("-Menu");
			
			if(scores_4 != 48){//eeprom_write_byte(First,scores); eeprom_read_byte(First);
				if(scores_4 == high_five){
					scores=48;
					scores_1=48;
					scores_2=48;
					scores_3=48;
					scores_4=48;					
				}
				else if(scores_4 > high_five){
					high_five = scores_4;
					high_four = scores_3;
					high_three = scores_2;
					high_two = scores_1;
					high_one = scores;
					eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);
					eeprom_update_byte((uint8_t*) 4, (uint8_t) high_two);
					eeprom_update_byte((uint8_t*) 7, (uint8_t) high_three);
					eeprom_update_byte((uint8_t*) 10, (uint8_t) high_four);
					eeprom_update_byte((uint8_t*) 13, (uint8_t) high_five);
					scores=48;
					scores_1=48;
					scores_2=48;
					scores_3=48;
					scores_4=48;
				}				
			}
			if(scores_3 != 48){
				if(scores_3 == high_four){
					scores=48;
					scores_1=48;
					scores_2=48;
					scores_3=48;
				}
				else if(scores_3> high_four){
					high_four = scores_3;
					high_three = scores_2;
					high_two = scores_1;
					high_one = scores;
					eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);
					eeprom_update_byte((uint8_t*) 4, (uint8_t) high_two);
					eeprom_update_byte((uint8_t*) 7, (uint8_t) high_three);
					eeprom_update_byte((uint8_t*) 10, (uint8_t) high_four);
					scores=48;
					scores_1=48;
					scores_2=48;
					scores_3=48;
				}				
			}
			if(scores_2 != 48){
				if(scores_2 == high_three){
					scores=48;
					scores_1=48;
					scores_2=48;
				}
				else if(scores_2> high_three){
					high_three = scores_2;
					high_two = scores_1;
					high_one = scores;
					eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);
					eeprom_update_byte((uint8_t*) 4, (uint8_t) high_two);
					eeprom_update_byte((uint8_t*) 7, (uint8_t) high_three);
					scores=48;
					scores_1=48;
					scores_2=48;
					
				}
			}
			if(scores_1 != 48){
				if(scores_1 == high_two){
					scores=48;
					scores_1=48;					
					
				}
				else if(scores_1> high_two){
					high_two = scores_1;
					high_one = scores;
					eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);
					eeprom_update_byte((uint8_t*) 4, (uint8_t) high_two);
					scores=48;
					scores_1=48;
				}
			}
			
			 if(scores != 48){
				if(scores == high_one){
					scores=48;
					
				}
				else if(scores > high_one){
					high_one = scores;
					eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);
					scores=48;
				}
			}
			else{

			}

			lsd = end_lcd;
		}
		break;
		
		case one_lcd:
		
		if(button_1&&succ_1){
			++checkers;
			lsd = one_lcd;	
		}
		else{
			if(scores==57){
				LCD_String_xy(1,oness,48);
				scores=48;
				
				++scores_1;//49
				LCD_String_xy(1,twos,scores_1);
				
				lsd = play_lcd;
			}
			else{
				++scores;
				LCD_String_xy(1,oness,scores);
				lsd = play_lcd;
			}			
			if(scores_1 == 58){
				LCD_String_xy(1,twos, 48);
				scores_1=48;
				++scores_2;
				LCD_String_xy(1,threes,scores_2);
				
				lsd = play_lcd;
			}
			if(scores_2 == 58){
				LCD_String_xy(1,threes, 48);
				scores_2=48;
				++scores_3;
				LCD_String_xy(1,fours,scores_3);				
				
				lsd = play_lcd;
			}
			if(scores_3 == 58){
				LCD_String_xy(1,fours, 48);
				scores_3=48;
				++scores_4;
				LCD_String_xy(1,fives,scores_4);
				
				lsd = play_lcd;				
			}

		}
		break;
		case two_lcd:
		
		if(button_2&&succ_2){
			++checkers;
			lsd = two_lcd;
		}
		else{
			if(scores==57){
				LCD_String_xy(1,oness,48);
				scores=48;
				
				++scores_1;//49
				LCD_String_xy(1,twos,scores_1);
				
				lsd = play_lcd;
			}
			else{
				++scores;
				LCD_String_xy(1,oness,scores);
				lsd = play_lcd;
			}
			if(scores_1 == 58){
				LCD_String_xy(1,twos, 48);
				scores_1=48;
				++scores_2;
				LCD_String_xy(1,threes,scores_2);
				
				lsd = play_lcd;
			}
			if(scores_2 == 58){
				LCD_String_xy(1,threes, 48);
				scores_2=48;
				++scores_3;
				LCD_String_xy(1,fours,scores_3);
				
				lsd = play_lcd;
			}
			if(scores_3 == 58){
				LCD_String_xy(1,fours, 48);
				scores_3=48;
				++scores_4;
				LCD_String_xy(1,fives,scores_4);
				
				lsd = play_lcd;
			}
		}
		break;
		case three_lcd:
		
		if(button_3&&succ_3){
			++checkers;
			lsd = three_lcd;
		}
		else{
			if(scores==57){
				LCD_String_xy(1,oness,48);
				scores=48;
				
				++scores_1;//49
				LCD_String_xy(1,twos,scores_1);
				
				lsd = play_lcd;
			}
			else{
				++scores;
				LCD_String_xy(1,oness,scores);
				lsd = play_lcd;
			}
			if(scores_1 == 58){
				LCD_String_xy(1,twos, 48);
				scores_1=48;
				++scores_2;
				LCD_String_xy(1,threes,scores_2);
				
				lsd = play_lcd;
			}
			if(scores_2 == 58){
				LCD_String_xy(1,threes, 48);
				scores_2=48;
				++scores_3;
				LCD_String_xy(1,fours,scores_3);
				
				lsd = play_lcd;
			}
			if(scores_3 == 58){
				LCD_String_xy(1,fours, 48);
				scores_3=48;
				++scores_4;
				LCD_String_xy(1,fives,scores_4);
				
				lsd = play_lcd;
			}
		}
		break;
		case four_lcd:
		
		if(button_4&&succ_4){
			++checkers;
			lsd = four_lcd;
		}
		else{
			if(scores==57){
				LCD_String_xy(1,oness,48);
				scores=48;
				
				++scores_1;//49
				LCD_String_xy(1,twos,scores_1);
				
				lsd = play_lcd;
			}
			else{
				++scores;
				LCD_String_xy(1,oness,scores);
				lsd = play_lcd;
			}
			if(scores_1 == 58){
				LCD_String_xy(1,twos, 48);
				scores_1=48;
				++scores_2;
				LCD_String_xy(1,threes,scores_2);
				
				lsd = play_lcd;
			}
			if(scores_2 == 58){
				LCD_String_xy(1,threes, 48);
				scores_2=48;
				++scores_3;
				LCD_String_xy(1,fours,scores_3);
				
				lsd = play_lcd;
			}
			if(scores_3 == 58){
				LCD_String_xy(1,fours, 48);
				scores_3=48;
				++scores_4;
				LCD_String_xy(1,fives,scores_4);
				
				lsd = play_lcd;
			}
		}
		break;
		
		case end_lcd:
	
		if(button_next){
			LCD_Commands(0x01);
			lsd = wait_end_lcd;
		}
		else{
			
			
		/*if(scores> high_one){
						
		eeprom_update_byte((uint8_t*) 1, (uint8_t) high_one);					
		}*/
			
			lsd = end_lcd;
		}
		break;
		
		case wait_end_lcd:
		if(button_next){
			lsd =wait_end_lcd;
		}
		else{
			lsd =wait_lcd;
		}
		break;
		
		case high_lcd:

		if(button_next){
			lsd = high_lcd;
		}
		else{
			lsd = wait_high_lcd;
		}
		break;
		
		case wait_high_lcd:
		if(button_start){
			LCD_Commands(0x01);
			LCD_String("Song: TT");
			LCD_Commands(0xC0);
			LCD_String("Score:00000");

			starter = 1;
			lsd = play_lcd;
		}
		else if(button_next){
			lsd = wait_again_lcd;
		}
		else{
			lsd = wait_high_lcd;
		}
		break;
		
		case wait_again_lcd:
		if(button_next){
			lsd = wait_again_lcd;
		}
		else{
			
			lsd= wait_lcd;
		}
		break;
		
		case h1ghr_lcd:///////////////////////////////////////////////

		break;
		
		
		default:
		lsd = start_lcd;
		break;
	}

	switch(lsd){
		case start_lcd:
		break;
		case wait_lcd:
			LCD_String("Rhythm Heaven");
			LCD_Commands(0xC0);
			LCD_String("-Play-High Score");		
		break;
		case play_lcd:
		
		break;
		case one_lcd:
		break;
		case two_lcd:
		break;
		case three_lcd:
		break;
		case four_lcd:
		break;
		case end_lcd:
		
		break;
		case wait_end_lcd:
		break;
		case high_lcd:

		break;
		
		case wait_high_lcd:
		break;
		
		case wait_again_lcd:
		LCD_Commands(0x01);
		break;
		case h1ghr_lcd:
		
		break;		
		
	}
	
	
	
	
}






















int main(void)
{
	
	DDRD=0xFC; PORTD = 0x03;
	DDRB=0xC0; PORTB = 0x3F;
	DDRA = 0xFF; //PORTA as output
	DDRC = 0xFF; //PORTC as output
			LCD_Init();
			
			PWM_on();
			
			TimerSet(tasksPeriodGCD);
			TimerOn();
			
	//initialize EEPROM addresses to 0
	//used for storing high score values
	if(eeprom_read_byte((uint8_t*)1) == 255) {
		eeprom_update_byte((uint8_t*)1, (uint8_t) 0);
	}
	if(eeprom_read_byte((uint8_t*)4) == 255) {
		eeprom_update_byte((uint8_t*)4, (uint8_t) 0);
	}
	if(eeprom_read_byte((uint8_t*)7) == 255) {
		eeprom_update_byte((uint8_t*)7, (uint8_t) 0);
	}
	if(eeprom_read_byte((uint8_t*)10) == 255) {
		eeprom_update_byte((uint8_t*)10, (uint8_t) 0);
	}
	if(eeprom_read_byte((uint8_t*)13) == 255) {
		eeprom_update_byte((uint8_t*)13, (uint8_t) 0);
	}
	First = eeprom_read_byte((uint8_t*)1);
	
	Second =eeprom_read_byte((uint8_t*)4);
	Third = eeprom_read_byte((uint8_t*)7);
	Fourth = eeprom_read_byte((uint8_t*)10);
	Fifth = eeprom_read_byte((uint8_t*)13);
		unsigned char i=0;
		tasks[i].state = start;
		tasks[i].period = matrixPeriod;
		tasks[i].elapsedTime = tasks[i].period;
		tasks[i].TickFct = &LED_Tick;		
		
		++i;
		tasks[i].state = init_2;
		tasks[i].period = sound;
		tasks[i].elapsedTime = tasks[i].period;
		tasks[i].TickFct = &songs;
		
		++i;
		tasks[i].state = input_start;
		tasks[i].period = buttsPeriod;
		tasks[i].elapsedTime = tasks[i].period;
		tasks[i].TickFct = &input_tick;	
		
		++i;
		tasks[i].state = start_accept;
		tasks[i].period = acceptPeriod;
		tasks[i].elapsedTime = tasks[i].period;
		tasks[i].TickFct = &accept_tick;		

		++i;
		tasks[i].state = start_lcd;
		tasks[i].period = lcdPeriod;
		tasks[i].elapsedTime = tasks[i].period;
		tasks[i].TickFct = &LCD_tick;


	while(1)
	{
		
	}
	
}