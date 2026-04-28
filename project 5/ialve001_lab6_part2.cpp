#include <avr/interrupt.h>
#include <avr/io.h>
#include "periph.h"
#include "helper.h"
#include "timerISR.h"

//TODO: Complete dir array to be able to display L,r,U,d and - with outDir function
int dir[5] = {0b0111110, 0b001110, 0b0000101, 0b0111101, 0b0000001};
// L = FED = 0b000111000 = dir[0]
// R = EFAB = 0b11001100 = dir[1]
// U = 0b01111100 = dir[2]
// D = BCDEG = 0b01111010 = dir[3]
// FEDCB = 0b01111100 = dir[4]
//

// a b c d e f g

//outDir will be like outnum, but it will output the direction of the joystick 
void outDir(int num){
  PORTD = dir[num] << 1;
  PORTB = SetBit(PORTB, 0 ,dir[num]&0x01);
}

int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; //8 phases of the stepper motor step
/*
/Stepper motors consist of a permanent magnet around the shaft with multiple stages of 
electromagnets surrounding the permanent magnet. Actuating the electromagnets in a specific order causes 
the shaft to rotate, and this process can be implemented as a concurrent SynchSM task. Stepper motors 
offer precise control over shaft rotation and produce more torque in comparison to other types of motors 
(such as the DC motor in your lab kit).


In order to control the stepper motor, you will need to cycle through a series of 8 phases to complete a step; 
the process then repeats to complete the next step. A full rotation of the stepper motor shaft requires 512 
completed step cycles (4096 individual phases). The amount of time spent in each phase controls the speed of
 the motor: reducing the amount of time per phase runs the motor at a faster speed. Stepper motors are bidirectional
(i.e., clockwise vs. counterclockwise): the motor will reverse if you cycle through the phases in reverse order. 

//TODO: declare variables for cross-task communication here
//TODO: declare the total number of tasks here
*/

#define NUM_TASKS 3 //number of task here - 2 for ex1*
// Task struct for concurrent synchSMs implmentations
typedef struct _task{
  unsigned int state; //Task's current state
  unsigned long period; //Task period
  unsigned long elapsedTime; //Time elapsed since last task tick
  int (*TickFct)(int); //Task tick function
} task;


const unsigned long TASK1_PERIOD = 100;
const unsigned long TASK2_PERIOD = 1;
const unsigned long TASK3_PERIOD = 100;


const unsigned long GCD_PERIOD = 1;
task tasks[NUM_TASKS]; // declared task array with 5 tasks (2 in exercise 1)
//TODO: Define, for each task:
// (1) enums and
// (2) tick functions
// Example below
//enum ABC_States {ABC_INIT, ABC_DO_STH};
//int TickFct_ABC(int state);

// task 1 : read the joystick and display which direction it's pointing torwards. 

// >>> cross-task signal for direction (volatile as requested)
volatile char direction = '-';

enum joystick_states { joystick_start , joystick_display }; 
int TickFct_joystick(int state){
  //state transitions
//the display needs to output one of 4 values, which are inplemented in the num function
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
    case joystick_display:  //based on the adc read, we will 'assume' the direction of the joystick
      {
        unsigned short x = ADC_read(3); 
        unsigned short y = ADC_read(2); 
        if (x < 300){ 
          outDir(1); //display L
          direction = 'L'; 
        } else if (x > 700){ 
          outDir(2); //outDir2 = R ->right.
          direction = 'R';
        } else if (y < 300){ 
          outDir(3);
          direction = 'U';
        } else if (y > 700){ 
          outDir(0);
          direction = 'D';
        } else { 
          outDir(4); //if there's nothing, just display '-'
          direction = '-';
        }
      }
      break;
  }
  return state;
}

//task 2 ; depending on what the display is outputting, we will rotate the stepper motor accordingly.
//copy from the RGB light lab, where it changed color depending on what the value displayed was. 
//if it's displaying up or down, motor should not move. we don't need an edge case, just the 'other' will take care of that.
volatile int step_limit = 1024; // 1/4 = 90 degrees
volatile int step_pos = 0; // 
enum stepper_states { stepper_start , stepper_left, stepper_right, stepper_stop };
int TickFct_stepper(int state){
  switch (state) {
    case stepper_start:
      if (direction == 'L' && step_pos > -step_limit){      // ok to go left
        state = stepper_left; 
      }
      else if (direction == 'R' && step_pos < step_limit){   // ok to go right
        state = stepper_right; 
      }
      else{ 
        state = stepper_stop;  
      }
      break;

    case stepper_left:
      // stay left only if STILL left AND NOT past left limit
      if (direction == 'L' && step_pos > -step_limit){
        state = stepper_left; 
      }
      else if (direction == 'R' && step_pos < step_limit){
        state = stepper_right; 
      }
      else { 
        state = stepper_stop; 
      } 
      break;

    case stepper_right:
      // stay right only if STILL right AND NOT past right limit
      if (direction == 'R' && step_pos < step_limit){
        state = stepper_right;  
      }
      else if (direction == 'L' && step_pos > -step_limit){
        state = stepper_left; 
      }
      else{ 
        state = stepper_stop;  
      }
      break;

    case stepper_stop:
      // only leave stop if direction is valid AND not at the limit
      if (direction == 'L' && step_pos > -step_limit) {
        state = stepper_left; 
      }
      else if (direction == 'R' && step_pos < step_limit) { 
        state = stepper_right;  
      }
      else{ 
        state = stepper_stop; 
      }
      break;

    default:
      state = stepper_start;
      break;
  }

  // State actions (unchanged except for tiny limit guards)
  static int phase_index = 0; //keeps track of which phase we're in (we need to go thru all 8)
  switch (state) {
    case stepper_start:
      break;

    case stepper_left:
      if (step_pos <= -step_limit) break;        // guard: don't step past -limit
      phase_index--;
      if (phase_index < 0) phase_index = 7;      // wrap underflow
      PORTB = (PORTB & 0x03) | (phases[phase_index] << 2);
      step_pos--;
      break;

    case stepper_right:
      if (step_pos >= step_limit) break;         // guard: don't step past +limit
      phase_index++;
      if (phase_index > 7) phase_index = 0;      // wrap overflow
      PORTB = (PORTB & 0x03) | (phases[phase_index] << 2);
      step_pos++;
      break;

    case stepper_stop:
      // Do nothing, motor is stopped
      break;

    default:
      break;
  }

  return state;   // <-- you were missing this
}

enum LEDStates { LED_INIT, LED_START, LED_ON };
int TickFct_LED(int state) {
  switch (state) {
    case LED_INIT:
      state = LED_START;
      PORTC |= (1 << PC0) | (1 << PC1);
      break;

    case LED_START:
      if (step_pos <= -step_limit || step_pos >= step_limit) {
        state = LED_ON;
      } else {
        PORTC |= (1 << PC0) | (1 << PC1);
        state = LED_START;
      }
      break;

    case LED_ON:
      if (step_pos > -step_limit && step_pos < step_limit) {
        state = LED_START;
      } else {
        state = LED_ON;
      }
      break;

    default:
      state = LED_INIT;
      break;
  }

  // State actions
  switch (state) {
    case LED_ON:
      // Turn on the appropriate LED based on which limit was reached
      if (step_pos <= -step_limit) {
        PORTC &= ~(1 << PC0);   // Left LED ON (clear bit = low = on)
        PORTC |= (1 << PC1);    // Right LED OFF (set bit = high = off)
      } else if (step_pos >= step_limit) {
        PORTC |= (1 << PC0);    // Left LED OFF
        PORTC &= ~(1 << PC1);   // Right LED ON
      }
      else{
        // Both LEDs OFF
        PORTC |= (1 << PC0) | (1 << PC1);
      }
      break;

    case LED_START:
    case LED_INIT:
      // Both LEDs OFF (high = off for active-low LEDs)
      PORTC |= (1 << PC0) | (1 << PC1);
      break;

    default:
      break;
  }
  
  return state;
}



void TimerISR() {
  for ( unsigned int i = 0; i < NUM_TASKS; i++ ) { // Iterate through each task in the task array
    if ( tasks[i].elapsedTime == tasks[i].period ) { // Check if the task is ready to tick
      tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
      tasks[i].elapsedTime = 0; // Reset the elapsed time for the next tick
    }

    tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
  }
}
int main(void) {
//TODO: initialize all your inputs and ouputs
//DDRC = 0x00; 
//PORTC = 0x00;   // analog inputs for joystick (no pull-ups)
DDRB = 0xFF; 
PORTB = 0x00;
DDRC  = (1 << PC0) | (1 << PC1);
PORTC = (1 << PC0) | (1 << PC1); 
//DDRB = ;
//PORTB = ;
DDRD = 0xFF;
PORTD = 0x00;
ADC_init(); // initializes ADC
//TODO: Initialize tasks here
// Example given below
 tasks[0].period = TASK1_PERIOD;
 tasks[0].state = joystick_start;
 tasks[0].elapsedTime = TASK1_PERIOD;
 tasks[0].TickFct = &TickFct_joystick;

 tasks[1].period = TASK2_PERIOD;
 tasks[1].state = stepper_start;
 tasks[1].elapsedTime = TASK2_PERIOD;
 tasks[1].TickFct = &TickFct_stepper;

    tasks[2].period = TASK2_PERIOD;
    tasks[2].state = LED_INIT;
    tasks[2].elapsedTime = TASK2_PERIOD;
    tasks[2].TickFct = &TickFct_LED;


TimerSet(GCD_PERIOD);
TimerOn();
while (1) {}
return 0;
}
//do we implement the joystick functionality here or in Tick ?
//TODO: Implement your task functions here
// example given
//int TickFct_ABC(int state) {
//
// switch(state) {"""state transition implementation"""}
//
// switch(state) {"""state action implementation"""}
//
// return state
// }
