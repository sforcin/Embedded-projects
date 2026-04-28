#include "timerISR.h"
#include "helper_sonar.h"
#include "periph_sonar.h"

/*

*/

//TODO: declare variables for cross-task communication

// TODO: Change this depending on which exercise you are doing.
// Exercise 1: 3 tasks
// Exercise 2: 5 tasks
// Exercise 3: 6 tasks

//ex1 tasks:
// sonar sensor will be measured every 1000ms, so we need transitions and triggers (read input from sonar) every 1000ms
// the display will be used in a similar way as the potentiometer, use outNum to display and distance every 1ms
//need to adjust the array
// then, the RGB has task period 1ms, PWM period = 10ms
//pwm is basically turning on and off the led quickly to simulate dimming/brightness
#define NUM_TASKS 3


//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	int state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD =  1;

task tasks[NUM_TASKS]; // declared task array with NUM_TASKS amount of tasks

//all task period's gcd = 1ms

//task 1: sonar reading task

volatile unsigned int distance_cm = 0; //remembeer from class - volatile variable to use in other sm's
enum SonarStates { SONAR_INIT, SONAR_READ };
//int sonar_update = 0; // global var to indicate when to update the sonar reading
int TickFct_Sonar(int state) {
    switch (state) {
        case SONAR_INIT:
            state = SONAR_READ;
            break;

        case SONAR_READ:
            state = SONAR_READ; //just keep reading
            break;

        default:
            state = SONAR_INIT;
            break;
    }

    // State actions. q: do we need to clear the distance after each reading, or does it not matter ?
    switch (state) {
        case SONAR_INIT:
            break;

        case SONAR_READ: //we need to read it from pin A0
            distance_cm = sonar_read(); // here we can use the global var 
            

            break;

        default:
            break;
    }

    return state;
}
//task 2: display task 
enum DisplayStates { DISPLAY_INIT, D1, D2, D3, D4 };
int TickFct_Display(int state) {
 

    switch (state) {
        case DISPLAY_INIT:
            state = D1;
            break;

        case D1:
            state = D2;
            break;

        case D2:
            state = D3;
            break;

        case D3:
            state = D4;
            break;

        case D4:
            state = D1;
            break;

        default:
            state = DISPLAY_INIT;
            break;
    }

    // State actions
    switch (state) {

        case DISPLAY_INIT:
            break;

        case D1:
            PORTB= SetBit(PORTB, PB2, 1); // Enable digit 0
            PORTB= SetBit(PORTB, PB3, 1);
            PORTB= SetBit(PORTB, PB4, 1);

            PORTB= SetBit(PORTB, PB5, 0);
            if(distance_cm >=25){
                outNum(15);
            }
            else {
                int digit_value = (distance_cm / 1000) % 10; // thousands
                outNum(digit_value);
            }
            break; // 'F'
        case D2:
            PORTB= SetBit(PORTB, PB2, 1); 
            PORTB= SetBit(PORTB, PB3, 1);
            PORTB= SetBit(PORTB, PB5, 1);

            PORTB= SetBit(PORTB, PB4, 0);
            if(distance_cm >=25){
                outNum(16);
            } 
            else {
                int digit_value = (distance_cm / 100) % 10; // thousands
                outNum(digit_value);
            }

            break;
        case D3:
            PORTB= SetBit(PORTB, PB2, 1); 
            PORTB= SetBit(PORTB, PB4, 1);
            PORTB= SetBit(PORTB, PB5, 1);

            PORTB= SetBit(PORTB, PB3, 0);
            if(distance_cm >=25){
                outNum(17);
            } 
            else {
                int digit_value = (distance_cm / 10) % 10; // thousands
                outNum(digit_value);
            }
            break;
        case D4:
            PORTB= SetBit(PORTB, PB3, 1); 
            PORTB= SetBit(PORTB, PB4, 1);
            PORTB= SetBit(PORTB, PB5, 1);

            PORTB= SetBit(PORTB, PB2, 0);
            if(distance_cm >=25){
                outNum(17);
            } 
            else {
                int digit_value = (distance_cm / 1) % 10; // thousands
                outNum(digit_value);
            }
            break;
    }

    return state;
}

//task 3: RGB LED PWM task. 
enum RGBStates { RGB_INIT, RGB_PWM };
int TickFct_RGB(int state) {
    static unsigned long pwm_counter = 0;
    const unsigned long pwm_period = 10; 
    const unsigned long pwm_duty = 5;  //condition to turn on the lights, 50% of the period.

    switch (state) {
        case RGB_INIT:
            state = RGB_PWM;
            break;

        case RGB_PWM:
            state = RGB_PWM; 
            break;

        default:
            state = RGB_INIT;
            break;
    }

    // State actions
    switch (state) {
        case RGB_INIT:
            break;

        case RGB_PWM:
            pwm_counter += GCD_PERIOD;
            if (pwm_counter < pwm_duty) {
               // if half of period, make the light appear purple (red and blue)
                PORTC = SetBit(PORTC, PC3, 1); 
                PORTC = SetBit(PORTC, PC5, 1); // Blue
            } else {
                // Turn off 
                PORTC = SetBit(PORTC, PC3, 0); // Red
                PORTC = SetBit(PORTC, PC5, 0); // Blue
            }
            if (pwm_counter >= pwm_period) {
                pwm_counter = 0; //after full period, restart it.
            }
            break;

        default:
            break;
    }

    return state;
}





void TimerISR() {
    
    //TODO: sample inputs here

	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {         
			tasks[i].state = tasks[i].TickFct(tasks[i].state); 
			tasks[i].elapsedTime = 0;                          
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        
	}
}


int main(void) {
    //TODO: initialize all your inputs and ouputs
DDRB |= (1<<PB5)|(1<<PB4)|(1<<PB3)|(1<<PB2)|(1<<PB1);  
DDRB &= ~(1<<PB0); // PB0 input
PORTB |= (1<<PB0); 
DDRD |= (1<<PD7)|(1<<PD6)|(1<<PD5)|(1<<PD4)|(1<<PD3)|(1<<PD2);
DDRC |= (1<<PC3)|(1<<PC4)|(1<<PC5)|(1<<PC1)|(1<<PC0)|(1<<PC2);


    ADC_init();   
    sonar_init();

    tasks[0].state = SONAR_INIT;
    tasks[0].period =1000 ;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &TickFct_Sonar;

    tasks[1].state = DISPLAY_INIT;
    tasks[1].period = 1;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &TickFct_Display;

    tasks[2].state = RGB_INIT;
    tasks[2].period =1 ;
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct = &TickFct_RGB;


    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        
    }

    return 0;
}