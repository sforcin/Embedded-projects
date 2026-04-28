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

//I will use the same setup for exercise 2
/*
exercise2:
coount from 1-15 using the 4 lights in binary. what needs to change:
state transitions: we will still have 4 states, and the left and right will still be used but for different purposes.
right: increment number
left: decrement number

so if 1, LED 1 lights up 
if right button is pressed, it should go to increment. maybe we would only need 3 states then? 
if left button is pressed, it should go to decrement.
then the states would have the conditions and the actions. 
maybe we need an idle state where it waits for input. 
*/

//maybe ill just hardcode the 16 states :') who cares about optimization 
enum states {Start, zero, one, two, three, four, five, six, seven, eight, nine, ten, eleven, twelve, thirteen, fourteen, fifteen} state;

void Tick() {
    // Declare any static variables here
    static unsigned char prev = 0x03; // keeps track of previous button, system stayed on loop until I added this.
    unsigned char curr = PINC & 0x03; // current button pressing
    
    //UPDATE: left button increments, right button decrements
    // State Transitions
    switch(state) {
        case Start:
            state = one; //start ->first state
            break;
//comments are flipped
        case zero:
            // from 0, if right ->1, if left (otherwise) just stay there
            if ((prev & 0x02) && !(curr & 0x02)) {
                state = fifteen;
            }
            else if ((prev & 0x01) && !(curr & 0x01)){  //if theres a left click, goes back to 15
                state = one;
            }
            else{
                state = zero;
            }
            break;
        case one:
            // from 1, if right ->2, if left (otherwise) just stay there
            if ((prev & 0x02) && !(curr & 0x02)) {
                state = zero;
            }
            else if ((prev & 0x01) && !(curr & 0x01)) { //checks for left click (01)
                state = two;
            }
            else {
                state = one;
            }
            break;
            
        case two:
            // from 2, could go to right or left
            if ((prev & 0x02) && !(curr & 0x02)) { //checks for right click (10)
                state = one;
            }
            
            else if ((prev & 0x01) && !(curr & 0x01)) { //checks for left click (01)
                state = three;
            }
            else {
                state = two; // if no button is pressed, do we wait for input or do we just stay here?
            }
            break;
            
        case three:
            // same logic again
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = two;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = four;
            }
            else {
                state = three;
            }
            break;

        case four:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = three;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = five;
            }
            else {
                state = four;
            }
            break;
        case five:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = four;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = six;
            }
            else {
                state = five;
            }
            break;

        case six:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = five;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = seven;
            }
            else {
                state = six;
            }
            break;
        case seven:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = six;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = eight;
            }
            else {
                state = seven;
            }
            break;
        case eight:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = seven;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = nine;
            }
            else {
                state = eight;
            }
            break;
        case nine: 
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = eight;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = ten;
            }
            else {
                state = nine;
            }
            break;
        case ten:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = nine;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = eleven;
            }
            else {
                state = ten;
            }
            break;
        case eleven:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = ten;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = twelve;
            }
            else {
                state = eleven;
            }
            break;
        case twelve:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = eleven;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = thirteen;
            }
            else {
                state = twelve;
            }
            break;
        case thirteen:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = twelve;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = fourteen;
            }
            else {
                state = thirteen;
            }
            break;
        case fourteen:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = thirteen;
            }
       
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = fifteen;
            }
            else {
                state = fourteen;
            }
            break;
        case fifteen:
            if ((prev & 0x02) && !(curr & 0x02)) { //right click (10)
                state = fourteen; //stay at 15
            }
            else if ((prev & 0x01) && !(curr & 0x01)) { //left click (01)
                state = zero;
            }
            else {
                state = fifteen;
            }
            break;
            
    }
    
    
    switch(state) {

        case Start:
            PORTB = SetBit(PORTB, 2, 0); 
            PORTB = SetBit(PORTB, 3, 0); 
            PORTB = SetBit(PORTB, 4, 0); 
            PORTB = SetBit(PORTB, 5, 0); 
            break;
        case zero:
            PORTB = SetBit(PORTB, 2, 0); 
            PORTB = SetBit(PORTB, 3, 0); 
            PORTB = SetBit(PORTB, 4, 0); 
            PORTB = SetBit(PORTB, 5, 0); 
            break;

        case one:
            PORTB = SetBit(PORTB, 2, 1);
            PORTB = SetBit(PORTB, 3, 0);
            PORTB = SetBit(PORTB, 4, 0);
            PORTB = SetBit(PORTB, 5, 0);
            break;
            
        case two:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 0);  
            PORTB = SetBit(PORTB, 5, 0);  
            break;
            
        case three:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 1); 
            PORTB = SetBit(PORTB, 4, 0); 
            PORTB = SetBit(PORTB, 5, 0);  
            break;
        case four:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 0);  
            break;
        case five:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 0);  
            break;
        case six:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 0);  
            break;
        case seven:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 0);  
            break;
        case eight:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 0);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case nine:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 0);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case ten:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 0);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case eleven:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 0);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case twelve:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case thirteen:  
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 0);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case fourteen:
            PORTB = SetBit(PORTB, 2, 0);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 1);  
            break;
        case fifteen:
            PORTB = SetBit(PORTB, 2, 1);  
            PORTB = SetBit(PORTB, 3, 1);  
            PORTB = SetBit(PORTB, 4, 1);  
            PORTB = SetBit(PORTB, 5, 1);  
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