#include <avr/io.h>
#include <util/delay.h>


unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
   return (b ? (x | (0x01 << k)) : (x & ~(0x01 << k)) );
   // Set bit to 1 Set bit to 0
}


unsigned char GetBit(unsigned char x, unsigned char k) {
   return ((x & (0x01 << k)) != 0);
}


// TODO: complete nums
// the index is also the digit it represents
// eg: nums[0]="0", nums[9]="9", nums[15]="f"
// bits layout: gfedcba (REVERSE ORDER!)
int nums[16] = {0b0111111, // 0
               0b0000110, // 1
               0b1011011, // 2
               0b1001111, // 3
               0b1100110, // 4
               0b1101101, // 5
               0b1111101, // 6
               0b0000111, // 7
               0b1111111, // 8
               0b1101111, // 9
               0b1110111, // a
               0b1111100, // b
               0b0111001, // c
               0b1011110, // d
               0b1111001, // e
               0b1110001  // f
};


void outNum(int num); //TODO: complete outNum() below


//TODO: complete outNum()
// void outNum(int num){
//     PORTB = ; //top left
//     // depends on your wiring. Assign bits (e-a), which are bits (4-0)
//     // from nums[] to register PORTB
//     PORTD = ; //0-7 top right
//     // Assign bits (g & f) which are bits 6 & 5 of nums[] to register PORTD
// }


//this is where I am going to create the state machine in the code.
//declare the 4 lights on and off, and start
enum states {Start, sm_one, sm_two, sm_three, sm_four} state;


void Tick() {
   // Declare any static variables here
   static unsigned char prev = 0x03; // keeps track of previous button, system stayed on loop until I added this.
   unsigned char curr = PINC & 0x03; // current button pressing
  
   // State Transitions
   switch(state) {
       case Start:
           state = sm_one; //start ->first state
           break;
          
       case sm_one:
           // from 1, if right ->2, if left (otherwise) just stay there
           if ((prev & 0x02) && !(curr & 0x02)) {
               state = sm_two;
           }
           else {
               state = sm_one;
           }
           break;
          
       case sm_two:
           // from 2, could go to right or left
           if ((prev & 0x02) && !(curr & 0x02)) { //checks for right click (10)
               state = sm_three;
           }
          
           else if ((prev & 0x01) && !(curr & 0x01)) { //checks for left click (01)
               state = sm_one;
           }
           else {
               state = sm_two;
           }
           break;
          
       case sm_three:
           // same logic again
           if ((prev & 0x02) && !(curr & 0x02)) {
               state = sm_four;
           }
     
           else if ((prev & 0x01) && !(curr & 0x01)) {
               state = sm_two;
           }
           else {
               state = sm_three;
           }
           break;
          
       case sm_four:
         
           if ((prev & 0x01) && !(curr & 0x01)) {
               state = sm_three;
           }
           else {
               state = sm_four;
           }
           break;
   }
  
  
   switch(state) {
       case Start:
           PORTB = SetBit(PORTB, 2, 1);
           PORTB = SetBit(PORTB, 3, 1);
           PORTB = SetBit(PORTB, 4, 1);
           PORTB = SetBit(PORTB, 5, 1);
           break;
          
       case sm_one:
           PORTB = SetBit(PORTB, 2, 0); 
           PORTB = SetBit(PORTB, 3, 0); 
           PORTB = SetBit(PORTB, 4, 0); 
           PORTB = SetBit(PORTB, 5, 1); 
           break;
          
       case sm_two:
           PORTB = SetBit(PORTB, 2, 0); 
           PORTB = SetBit(PORTB, 3, 0);
           PORTB = SetBit(PORTB, 4, 1);
           PORTB = SetBit(PORTB, 5, 0); 
           break;
          
       case sm_three:
           PORTB = SetBit(PORTB, 2, 0);
           PORTB = SetBit(PORTB, 3, 1);
           PORTB = SetBit(PORTB, 4, 0);
           PORTB = SetBit(PORTB, 5, 0);
           break;
          
       case sm_four: 
           PORTB = SetBit(PORTB, 2, 1);
           PORTB = SetBit(PORTB, 3, 0);
           PORTB = SetBit(PORTB, 4, 0);
           PORTB = SetBit(PORTB, 5, 0); 
           break;     
   }
  
   prev = curr; // update curr for next iteration
}


int main(void){
   //inputs: A0=Left button, A1=Right button
   //outputs: pins 10-13 (PB2-PB5)
  
   // Set pins 10-13 (PB2-PB5) as outputs
   DDRB = 0x3C;  // Binary: 00111100 - sets bits 2,3,4,5 as outputs
   PORTB = 0x00;
  
   // A0 and A1 are the inputs (10 or 01 )
   DDRC = 0x00;  // system inputs
   PORTC = 0x03; // Enable pull-ups on PC0 and PC1
  
   // Initialize state
   state = Start;
  
   while (1) {
       Tick();
       _delay_ms(100); // debounce buttons
   }
}

