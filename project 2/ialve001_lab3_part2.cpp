#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
return (b ? (x | (0x01 << k)) : (x & ~(0x01 << k)) );
// Set bit to 1 Set bit to 0
}

unsigned char GetBit(unsigned char x, unsigned char k) {
return ((x & (0x01 << k)) != 0);
}


//TODO: Copy and paste your nums[] and outNum[] from the previous lab. The wiringis still the same, so it should work instantly
int nums[16] = {
    //gfedcba
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
    0b1110111, // A
    0b1111100, // B
    0b0111001, // C
    0b1011110, // D
    0b1111001, // E
    0b1110001  // F
};

//questions: is this function supposed to output anything ?
//how do i set up the mapping correctly ? i believe my wiring is correct, but for some reason it is not outputting the correct values on the dislay

void outNum(int n){

    PORTB = nums[n] & 0x1F ; // 12 in hex -> 0x0C. we are using pins 8-12 for PORTB, and 7 and 6 for port D
    // depends on your wiring. Assign bits (e-a), which are bits (4-0)
    // from nums[] to register PORTB
    PORTD =(nums[n] & 0x60)<<1 ; //0-7 top right 
    // Assign bits (g & f) which are bits 6 & 5 of nums[] to register PORTD

    //is this not working right becayse this function doesn't output anything ? should it activate the pins? because after we map, we call this function to output the number
}

void ADC_init() {
// TODO: figure out register values
/*insert your value for ADMUX*/ //23.9.1 ->  ADC uses AVCC as its reference voltage.
ADMUX =(1 << REFS0); // 

//Set this register so that ADC is enabled, and auto-trigger is disabled. Also, set the appropriate prescaler (division factor) so that ADC can get a 10-bit output resolution (the max resolution).
ADCSRA = (1<< ADEN) | (0<<ADATE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); 
//bit 7: ADEN: ADC Enable
//bit 5: ADATE: ADC Auto Trigger Enable (should be set to 0 here to disable auto-triggering)
//scaling should be 111, division factor 128 ADPS2  ADPS1 ADPS0
//23.9.4
//ADTS2 ADTS1 ADTS0
ADCSRB = (0<<ACME)  |(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0); //free running mode, ACME =0 means negative input of the analog comparator is connected to AIN0 
}


unsigned int ADC_read(unsigned char chnl){
// ^^^^ unsigned char chnl selects which pin you're reading analog signal from
ADMUX = (ADMUX & 0xF0) | (chnl & 0x0F); // 
 //*set MUX3:0 bits without modifying any other bits*/;
ADCSRA |= (1 << ADSC);     /*set the bit that starts conversion in free running mode without modifying any other bit*/;
while((ADCSRA >> 6)&0x01) {} /*what does this line do?*/

uint8_t low, high;
low = ADCL ;
high = ADCH ;

// the results are stored in ADCL and ADCH
// ADLAR can be 0 or 1.
return ((high << 8) | low) ;
}
// Provided map()
long map(long x, long in_min, long in_max, long out_min, long out_max) {
return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



enum states {AM_mode, FM_mode} state; //TODO: finish the enum for the SM
void Tick() {
static unsigned char prev=1;
unsigned char curr = GetBit(PINC,1); // read input from PC0
unsigned int adc_val;
int display;
//TODO: declare your static variables here or declare it globally
// State Transistions
//TODO: complete transitions
switch (state) {
    case AM_mode:
        if (prev == 1 && curr == 0) { // detect high to low transition
            state = FM_mode;
        }
        break;
    case FM_mode:
        if (prev == 1 && curr == 0) { // detect high to low transition
            state = AM_mode;
        }
        break;
    default:
        state = AM_mode;
        break;
}
prev = curr;

// State Actions
//TODO: complete transitions
switch (state) {
    case AM_mode:
        adc_val = ADC_read(0);   // read A0
        display = map(adc_val, 0, 1023, 0x00,0x09); //9 in hex is 0x09
        outNum(display); // display  digit
        break;
    case FM_mode:
        adc_val = ADC_read(0);   // read A0
        display = map(adc_val, 0, 1023, 0x0A, 0x0F ); // A and F in hex is 0x0A and 0x0F
        outNum(display); // display  digit
        break;
}
}

int main(void){
//TODO: initialize all outputs and inputs
DDRB |= (1<<PB4)|(1<<PB3)|(1<<PB2)|(1<<PB1)|(1<<PB0); // Binary: 00111100 - sets bits 2,3,4,5 as outputs
DDRD |= (1<<PD7)|(1<<PD6);
    // A0 and A1 are the inputs (10 or 01 )
DDRC = 0x00;  // system inputs
PORTC = 0x00; // Enable pull-ups on PC0 and PC1



ADC_init();
TimerSet(500); //TODO: Set your timer
TimerOn();
while (1){
Tick(); // Execute one synchSM tick
while (!TimerFlag){} // Wait for SM period
TimerFlag = 0; // Lower flag
}
return 0;
}