#include <avr/interrupt.h>
#include <avr/io.h>
#include "periph.h"
#include "helper.h"
#include "timerISR.h"


int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; //8 phases of the stepper motor step

#define NUM_TASKS 6 
// Task struct for concurrent synchSMs implmentations
typedef struct _task{
    unsigned int state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

//keep all the variables at the top so it doesnt look too messy
const unsigned long TASK1_PERIOD = 100; //100ms for joystick
const unsigned long TASK2_PERIOD = 5; //1ms for stepper
const unsigned long TASK3_PERIOD = 100; //100ms for servo
const unsigned long TASK4_PERIOD = 100; //100ms for buzzer
const unsigned long TASK5_PERIOD = 1;//1ms for idle monitor
const unsigned long TASK6_PERIOD = 50; //50ms for idle buzzer
const unsigned long GCD_PERIOD   = 1;
volatile int servo_angle = 0;      
volatile char vdir = '-';      
volatile unsigned short x = 0;
volatile unsigned short y = 0;
const unsigned long stepper_limits[6] = {1, 2, 3, 4, 5, 6};
volatile int stepper_index = 1;
task tasks[NUM_TASKS];    
volatile char direction = '-';
volatile int step_pos = 0;  
static unsigned int cnt = 0;  


unsigned char stepper_speedup   = 0;    
unsigned char stepper_slower  = 0;  
unsigned char stepper_idleflag = 0;        
unsigned char stepper_idle = 1;

unsigned char buzzer_on = 0;

unsigned long idle_cnt = 0;

//helper functions for the buzzer, without this it was just on nonstop and I couldnt get it to work

void buzzTone(unsigned char buzzScale) {
    OCR0A  = 128;
    TCCR0A |= (1 << COM0A1);
    TCCR0B = (TCCR0B & 0xF8) | buzzScale;
}

void buzz_off() {
    TCCR0A &= ~(1 << COM0A1);
    TCCR0B &= ~((1 << CS02) | (1 << CS01) | (1 << CS00));
    PORTD  &= ~(1 << PD6);
}



enum joystick_states { joystick_start , joystick_display };    
int tickfct_joystick(int state){
    //state transitions
    switch (state){
        case joystick_start:
            state = joystick_display;
            break;
        case joystick_display:
            state = joystick_display;
            break;
    }
    //state actions
    switch (state){
        case joystick_start:
            break;
        case joystick_display:    
            
            x = ADC_read(3);    
            y = ADC_read(2);    
          
            servo_angle = ((1023 - x) * 180L) / 1023 - 90; 
            
            // y axis is for stepper only now, and x axis only for servo.
            if (y < 300){    
                direction = 'U';    
            }    
            else if (y > 700){    
                direction = 'D';  //setting up flags to use in other sm's
            }    
            else {    
                direction = '-'; 
            }

            break;
    }
    return state;
}


enum stepper_states { stepper_start , stepper_left, stepper_right, stepper_stop };
int tickfct_stepper(int state){
    

    switch (state) {
        case stepper_start:
            if (direction == 'U' ){        //using the joystick flag to know which way to go   
                state = stepper_right; 
            }
            else if (direction == 'D'){      
                state = stepper_left; 
            }
            else{    
                state = stepper_stop;      
            }
            break;

        case stepper_left:
            if (direction == 'D'){
                state = stepper_left;    
            }
            else if (direction == 'U' ){
                state = stepper_right;    
            }
            else {    
                state = stepper_stop;    
            }    
            break;

        case stepper_right:
            if (direction == 'U'){
                state = stepper_right;        
            }
            else if (direction == 'D'){
                state = stepper_left;    
            }
            else{    
                state = stepper_stop;      
            }
            break;

        case stepper_stop:
            if (direction == 'U' ) {
                state = stepper_right;    
            }
            else if (direction == 'D' ) {    
                state = stepper_left;      
            }
            else{    
                state = stepper_stop;    
            }
            break;

        default:
            state = stepper_start;
            break; //this is all the same from the previous lab.
    }

    // State actions
    static int speed = 0;    
    unsigned long stepper_limit = stepper_limits[stepper_index];
    
    switch (state) {
        case stepper_start:
            cnt = 0;
            break;

        case stepper_left: 
            stepper_idle = 0;
            cnt++;
            if (cnt >= stepper_limit) { //still the same logic kinda, but now we have to make the speed faster or slower
                speed--;
                if (speed < 0){
                       speed = 7;        
                }        
                PORTB = (PORTB & 0x03) | (phases[speed] << 2);
                step_pos--;
                cnt = 0;
            }
            
            idle_cnt = 0;    //if its moving, we reset the idle timer
            break;

        case stepper_right: 
            stepper_idle = 0;
            cnt++;
            if (cnt >= stepper_limit) {
                speed++;
                if (speed > 7){
                       speed = 0;    
                }
                PORTB = (PORTB & 0x03) | (phases[speed] << 2);
                step_pos++;
                cnt = 0;
            }
            
            idle_cnt = 0;    
            break;

        case stepper_stop:
            stepper_idle = 1;
            cnt = 0;

            if (!buzzer_on) {
                idle_cnt += TASK2_PERIOD;
                if (idle_cnt >= 5000) {
                    stepper_idleflag = 1;
                    idle_cnt = 0;
                }
            }
            break;

        default:
            break;
    }

    return state;          
}


enum servo_state {servo_start, servo_move};
int tickfct_servo(int state){
    switch (state){
    case servo_start:
        state = servo_move;
        break;
    case servo_move:
        state = servo_move;    
        break;
    default:
        state = servo_start;
        break;
    }
    switch(state){
    case servo_start:
        break;
    case servo_move:
        OCR1A = 999 + ((unsigned long) x * 4000UL) / 1024UL; 
        if (servo_angle > 45)          vdir = 'U';    
        else if (servo_angle < -45) vdir = 'D';    
        else                       vdir = '-';    
        break;
    }
    return state;
}


enum ledstates { led_init, led_start };    
int tickfct_led(int state) {
    switch (state) {
        case led_init:
            state = led_start;
            PORTD &= ~((1 << PD2) | (1 << PD4));    
            PORTD |= (1 << PD3);                      
            PORTD &= ~((1 << PD5) | (1 << PD7));    
            PORTD |= (1 << PD6);                      
            PORTB |= (1 << PB0);    
            break;
        case led_start:
            state = led_start;    
            break;
        default:
            state = led_init;
            break;
    }
    switch (state) {
        case led_init:
            break;
        case led_start:
            
            if (direction == 'U') {
                PORTD |= (1 << PD2);                      
                PORTD &= ~((1 << PD3)|(1 << PD4));        
            } else if (direction == 'D') {
                PORTD |= (1 << PD4);                      
                PORTD &= ~((1 << PD2)|(1 << PD3));        
            } else {    
                PORTD |= (1 << PD3);                      
                PORTD &= ~((1 << PD4)|(1 << PD2));        
            }

            
            if (vdir == 'U') { //different var bc this is absed on angle
                PORTD |= (1 << PD5);                      
                PORTD &= ~((1 << PD7) | (1 << PD6));    
                PORTB |= (1 << PB0);                      
            } else if (vdir == 'D') {
                PORTD |= (1 << PD7);                      
                PORTD &= ~((1 << PD5) | (1 << PD6));    
                PORTB |= (1 << PB0);                      
            } else {    
                PORTD |= (1 << PD6);                      
                PORTD &= ~((1 << PD5) | (1 << PD7));    
                PORTB |= (1 << PB0);                      
            }
            break;
        default:
            break;
    }
    return state;
}


enum speedstates { speed_start };

int tickfct_speedcontrol(int state) {
    switch (state) {
        case speed_start:
            state = speed_start;
            break;
        default:
            state = speed_start;
            break;
    }

    static unsigned char prev_p = 0x00;    
    static unsigned char started = 0;
    unsigned char curr_p = PINC;

    if (!started) {
        prev_p = curr_p;
        started = 1;
        return state;
    }
    

    unsigned char prev_right = (prev_p & (1 << PC1));
    unsigned char curr_right = (curr_p & (1 << PC1));

    if (!prev_right && curr_right) {
        if (stepper_index > 0) {
            stepper_index--;
            stepper_speedup = 1;

            idle_cnt = 0;
            stepper_idleflag = 0;
        }
    }
    

    unsigned char prev_left = (prev_p & (1 << PC0));
    unsigned char curr_left = (curr_p & (1 << PC0));

    if (!prev_left && curr_left) {
        if (stepper_index < 5) {
            stepper_index++;
            stepper_slower = 1;

            idle_cnt = 0;
            stepper_idleflag = 0;
        }
    }
    
    prev_p = curr_p;    

    return state;
}


enum buzzer_states { 
    buzzer_off, 
    buzzer_left_1, buzzer_left_off1, buzzer_left_2, buzzer_left_off2,
    buzzer_right_1, buzzer_right_off1, buzzer_right_2, buzzer_right_off2,
    buzzer_idle_on1, buzzer_idle_off1, buzzer_idle_on2, buzzer_idle_off2, buzzer_idle_on3, buzzer_idle_off3
};

int tickfct_buzzer(int state) {
    static unsigned int note_timer = 0;
    const unsigned int note_duration = 100;

    if (state == buzzer_off) buzzer_on = 0;
    else                     buzzer_on = 1;

    switch (state) {
        case buzzer_off:
            buzz_off();
            if (stepper_speedup) {
                state = buzzer_left_1;
                stepper_speedup = 0;
            } else if (stepper_slower) {
                state = buzzer_right_1;
                stepper_slower = 0;
            } else if (stepper_idleflag) {
                state = buzzer_idle_on1;
                stepper_idleflag = 0;
            } else {
                state = buzzer_off;
            }
            break;
            
        case buzzer_left_1:
            buzzTone(0x03);
            note_timer = 0;
            state = buzzer_left_off1;
            break;

        case buzzer_left_off1:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_left_off1;
            } else {
                state = buzzer_left_2;
            }
            break;

        case buzzer_left_2:
            buzzTone(0x02);
            note_timer = 0;
            state = buzzer_left_off2;
            break;

        case buzzer_left_off2:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_left_off2;
            } else {
                state = buzzer_off;
            }
            break;
            
        case buzzer_right_1:
            buzzTone(0x05);
            note_timer = 0;
            state = buzzer_right_off1;
            break;

        case buzzer_right_off1:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_right_off1;
            } else {
                state = buzzer_right_2;
            }
            break;

        case buzzer_right_2:
            buzzTone(0x06);
            note_timer = 0;
            state = buzzer_right_off2;
            break;

        case buzzer_right_off2:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_right_off2;
            } else {
                state = buzzer_off;
            }
            break;

        case buzzer_idle_on1:
            buzzTone(0x02);
            note_timer = 0;
            state = buzzer_idle_off1;
            break;

        case buzzer_idle_off1:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_idle_off1;
            } else {
                state = buzzer_idle_on2;
            }
            break;

        case buzzer_idle_on2:
            buzzTone(0x04);
            note_timer = 0;
            state = buzzer_idle_off2;
            break;

        case buzzer_idle_off2:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_idle_off2;
            } else {
                state = buzzer_idle_on3;
            }
            break;

        case buzzer_idle_on3:
            buzzTone(0x02);
            note_timer = 0;
            state = buzzer_idle_off3;
            break;

        case buzzer_idle_off3:
            if (note_timer < note_duration) {
                note_timer += TASK6_PERIOD;
                state = buzzer_idle_off3;
            } else {
                state = buzzer_off;
            }
            break;
            
        default:
            state = buzzer_off;
            break;
    }
    
    return state;
}


void timerisr() {
    for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {    
        if ( tasks[i].elapsedTime >= tasks[i].period ) {    
            tasks[i].state = tasks[i].TickFct(tasks[i].state);    
            tasks[i].elapsedTime = 0;    
        }

        tasks[i].elapsedTime += GCD_PERIOD;    
    }
}

int main(void) {
    DDRB = 0xFF; PORTB = 0x00;
    DDRC  = 0x00; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    DDRD |= (1 << PD6);
    TCCR0A |= (1 << WGM01) | (1 << WGM00);
    TCCR0A |= (1 << COM0A1); 
    buzz_off();
    ADC_init();
    DDRB |= (1 << PB1);          
    TCCR1A |= (1 << WGM11) | (1 << COM1A1);
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11);
    ICR1 = 39999;    
    OCR1A = 2999;    

//learned about this type of initialization at office hours.
    unsigned char i = 0;

    tasks[i].period = TASK1_PERIOD;
    tasks[i].state = joystick_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_joystick;
    i++;
    
    tasks[i].period = TASK2_PERIOD;
    tasks[i].state = stepper_start;
    tasks[i].elapsedTime = tasks[i].period;    
    tasks[i].TickFct = &tickfct_stepper;
    i++;

    tasks[i].period = TASK3_PERIOD;
    tasks[i].state = led_init;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_led;
    i++;

    tasks[i].period = TASK4_PERIOD;
    tasks[i].state = servo_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_servo;
    i++;
    
    tasks[i].period = TASK5_PERIOD;
    tasks[i].state = speed_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_speedcontrol;
    i++;

    tasks[i].period = TASK6_PERIOD;
    tasks[i].state = buzzer_off;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_buzzer;

    // Start Scheduler
    TimerSet(GCD_PERIOD);
    TimerOn();
    while (1) {}
    return 0;
}
/*
Exercise 3
Add a passive buzzer such that:

Increasing the speed of the stepper motor gives a two-note alert.
Decreasing the speed of the stepper motor gives a two-note alert. This should sound distinct from the previous sound.
Leaving the stepper motor idle for more than 10 seconds gives a three-note alert every 5 seconds for as long as the motors are idle.

shouldnt be too hard, if the speed up is triggered, turn on sonar, same for slow down
period of the button thing is 1 ms, so if it has 10,000 periods unattended it should make a 3 note alert every 5 seconds
going to copy from the potentiometer lab where it had the buzzer so i know how to use it again
the ony tricky part will be the period, so knowing the turn on/off things.

game plan:
i tried to do fancy variables for the tones, but that didn't work, so since each tone is a layer of 2 tones, I will just hardcode each tone
using a state for each one of them. it's gonna be a lot but it should work :')

problems: buzzer stays on, doesn't recognize buttons. I had to make helper functions that addressed that.
*/