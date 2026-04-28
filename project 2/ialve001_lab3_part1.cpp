#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"
#include "serialATmega.h"


//look at datasheet for these values
void ADC_init() {
// TODO: figure out register values

/*insert your value for ADMUX*/ //23.9.1 ->  ADC uses AVCC as its reference voltage.
ADMUX =(1 << REFS0); // 

////23.9.2
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

enum states {Start, Display} state;
void Tick(void) {
    unsigned int raw = ADC_read(0);   // read A0
    serial_println((long)raw);
}

int main(void){
// TODO: Initialize your I/O pins



DDRB = 0x3C ;  // Binary: 00111100 - sets bits 2,3,4,5 as outputs
PORTB = 0x00; 
    
    // A0 and A1 are the inputs (10 or 01 )
DDRC = 0x00;  // system inputs
PORTC = 0x00; // Enable pull-ups on PC0 and PC1
    
//DDRD = ;
//PORTD = ;
ADC_init();
serial_init(9600);


TimerSet(500);
TimerOn();

//TimerSet(500); // Set timer to 100 ms
//TimerOn();


//state = Start;
while (1){
Tick(); // Execute one synchSM tick
while (!TimerFlag){} // Wait for SM period
TimerFlag = 0;
}
return 0;
}