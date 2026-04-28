#include <avr/interrupt.h>
#include <avr/io.h>
#include "periph.h"
#include "helper.h"
 #include "timerISR.h" 
#define RST_BIT  0
#define A0_BIT   6  
#define CS_BIT   2  
#undef NUM_TASKS
#define NUM_TASKS 9
#define RGB_R (1 << PD4)
#define RGB_G (1 << PD3)
#define RGB_B (1 << PD2)

#define MAX_ALIENS 4 //5 was bugging sadly
//final version.
//ascii chars:
const uint8_t font[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // Space
    {0x7F,0x09,0x09,0x09,0x7F}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // : 
    {0x08,0x14,0x22,0x41,0x00}, // < 
    {0x00,0x41,0x22,0x14,0x08}, // > 
    {0x3E,0x41,0x41,0x41,0x00}, // ( 
    {0x00,0x41,0x41,0x41,0x3E}  // ) 
};
const uint8_t alienSprite[] = {0b01110, 0b01010, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
const uint8_t shipSprite[] = { 0b00100,0b01110,0b11111,0b10101,0b00100, 0b00000,0b00000,0b00000};
const uint8_t bulletSprite[] = { 0b00100,0b00100,0b00100,0b00000,0b00000,0b00000,0b00000,0b00000};

volatile unsigned char game_over_flag = 0; 
const unsigned long TASK_JOYSTICK_PERIOD = 100; // 100ms for joystick
const unsigned long TASK_BUTTON_PERIOD = 1;      // 1ms for button input check
const unsigned long TASK_BUZZER_PERIOD = 50;     // 50ms for buzzer sequence tick
const unsigned long TASK_TIMER_PERIOD = 1000;    // 1000ms for timer countdown (1 second)
const unsigned long TASK_RGB_PERIOD   = 100;
const unsigned long GCD_PERIOD   = 1;

typedef struct _task{
    unsigned int state; //Task's current state
    unsigned long period; //Task period
    unsigned long elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

volatile char vdir = '-';      
volatile unsigned short x = 0;
volatile unsigned short y = 0;
task tasks[NUM_TASKS];    
volatile char direction = '-';
volatile int step_pos = 0;  

// game flags
unsigned char time_45 =0;
unsigned char time_half =0;
unsigned char time_quarter =0;
volatile unsigned char time_up =0; // made volatile to be used in the game sm, so when the time is up the screen turns black.
unsigned char reset_flag =0;
volatile unsigned char shoot_flag =0;
unsigned char buzzer_flag =0; 
volatile int game_timer_sec = 30; 
volatile int score =0;
volatile int pX = 60; 
volatile int pY = 110; 

unsigned char buzzer_on = 0;
unsigned long idle_cnt = 0;

/* alien stuff 
I had to add some functions for the alien because when they fall, they basically dont go back to the beginning of the screen 
so i created a steruct, created the aliens, 
*/
typedef struct {
    int x;
    int y;
    int prev_y; 
    unsigned char active; 
} Alien;
Alien aliens[MAX_ALIENS];

//in order to make the aliens move, im basically going to set up their initial positions. so they dont all show up at once,.
//im making some of them appear off the screen. then theyre going to move down
void resetAliens() {
    aliens[0].x = 10;  aliens[0].y = 10;   aliens[0].active = 1; //i just chose random positions for the aliens to start
    aliens[1].x = 40;  aliens[1].y = -20;  aliens[1].active = 1;
    aliens[2].x = 70;  aliens[2].y = -50;  aliens[2].active = 1;
    aliens[3].x = 100; aliens[3].y = -80;  aliens[3].active = 1;
    for(int i=0; i<MAX_ALIENS; i++){
        aliens[i].prev_y = aliens[i].y;
    }
}
//buzzer functions (to turn it off or on when we need a buzzer)
//fixed w the correct implementation of the timer according to lab 7 manual 
//helper functions
void buzzTone(uint16_t freq) {

    DDRB |= (1 << PB1);  //fixed w the right formulas. i did it all here so i can just call the frequencies at each tone
    uint32_t icr = (16000000 / (8 * (uint32_t)freq)) - 1;
    ICR1 = icr;
    OCR1A = icr / 2; 
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
}

void buzz_off() {
    TCCR1A = 0;
    TCCR1B = 0;
    PORTB &= ~(1<<PB1);
}

void SPI_INIT(){
    DDRB |= (1<<3) | (1<<5) | (1<<CS_BIT) | (1<<RST_BIT); //MOSI, SCK, SS, RST as output on PORTB
    SPCR |= (1<<SPE) |(1<<MSTR);
    PORTB = SetBit(PORTB, CS_BIT, 1);
}

void SPI_SEND(uint8_t data){
    SPDR = data;
    while(!(SPSR & (1<<SPIF)));// wait until the transferni is completed.
}

void Send_Data(uint8_t data){
    PORTD = SetBit(PORTD, A0_BIT, 1);  // data mode (PORTD Pin 6)
    PORTB = SetBit(PORTB, CS_BIT, 0); //send the data to the screen
    SPI_SEND(data);
    PORTB = SetBit(PORTB, CS_BIT, 1); 
}

void Send_Command(uint8_t command){
    PORTD = SetBit(PORTD, A0_BIT, 0);  // command mode (PORTD Pin 6)
    PORTB = SetBit(PORTB, CS_BIT, 0); //send the command 
    SPI_SEND(command); //going to send the command and stay there until it's finished.
    PORTB = SetBit(PORTB, CS_BIT, 1); 
}
//need to implement Send_command and Send_data to complement profs code.
void HardwareReset() {
    PORTB = SetBit(PORTB, RST_BIT, 0); // Reset Low
    _delay_ms(200);
    PORTB = SetBit(PORTB, RST_BIT, 1); // Reset High
    _delay_ms(200);
}

void ST7735_init() {
    HardwareReset();
    Send_Command(0x01); // SWRESET
    _delay_ms(150);
    Send_Command(0x11); // SLPOUT
    _delay_ms(200);
    Send_Command(0x3A); // COLMOD
    Send_Data(0x05);    // 16-bit color
    _delay_ms(10);
    Send_Command(0x29); // DISPON
    _delay_ms(200);
}

// Defines the "window" we are coding in
void setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    Send_Command(0x2A); // column data coming up
    Send_Data(0x00);
    Send_Data(x0);      //x axis start (all the way to the left)
    Send_Data(0x00);
    Send_Data(x1);      // x ends at the end of screen to the right
//repreat logic for y axis
    Send_Command(0x2B); 
    Send_Data(0x00);
    Send_Data(y0);      //start
    Send_Data(0x00);
    Send_Data(y1);      // end
    Send_Command(0x2C); 
}

// Fills the whole screen with one color
void fillScreen(uint16_t color) {
    uint8_t highByte = (color >> 8); //split the color into 2 bytes becasue the screen takes 16 bit color
    uint8_t lowByte = (color & 0xFF); 
//set window to full (since we're filling it)
    setAddressWindow(0, 0, 127, 127);
    PORTB = SetBit(PORTB, CS_BIT, 0); 
    PORTD = SetBit(PORTD, A0_BIT, 1); // Data Mode (updated w pin swap to use timer1)
//128 x 128 = 16384 pixels
    for (uint16_t i = 0; i < 16384; i++) {
        SPI_SEND(highByte);
        SPI_SEND(lowByte);
    }
    
    PORTB = SetBit(PORTB, CS_BIT, 1); // deselect.
}

void drawPixel(uint8_t x, uint8_t y, uint16_t color) { //split it into 2 bytes because the screen uses 16 bit color
    setAddressWindow(x, y, x, y);
    PORTB = SetBit(PORTB, CS_BIT, 0); 
    PORTD = SetBit(PORTD, A0_BIT, 1); 
    SPI_SEND(color >> 8);
    SPI_SEND(color & 0xFF); // send low byte

    PORTB = SetBit(PORTB, CS_BIT, 1); 
}

void drawSprite(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint16_t color, uint8_t scale) {
    // scan th rows, depending on the height of the object we are creating.
    for (uint8_t i = 0; i < h; i++) {
        uint8_t rowByte = bitmap[i];
        for (uint8_t j = 0; j < w; j++) { //then, scan the columns within the rows, depending on the width of the object
            uint8_t bitPosition = (w - 1) - j;
            // move the byte we're checking to the rightmost position, then check if its a 1
            if ((rowByte >> bitPosition) & 0x01) { // if it is a 1,  when it gets to the 1 we need to trigger the drawPixel function to make the pixel show up in the screen,
                if (scale == 1){ // if the scale is 1, just draw the pixel )
                    drawPixel(x + j, y + i, color);
                } 
                else { //scale the x and the y axis of the drawing up to the desired scale (to make sure its uniform)
                    for (uint8_t scale_x = 0; scale_x < scale; scale_x++) {
                        for (uint8_t scale_y = 0; scale_y < scale; scale_y++) {
                            drawPixel(x + (j * scale) + scale_x, y + (i * scale) + scale_y, color);
                        }
                    }
                }
            }
        }
    }
}

//next i will implement the player task:
// the player will control the ship. for that
//px and py will be the current position of the ship
//the issue with this is the tracks, so ill try to use kinda the same logic ad the button debouncing
// so i will create a prev andx curr, and the prev will draw a black ship to make it look like it disappeared. this works.

volatile int prev_pX = 60; //this is to store the prev, the one ill clear

/* to draw the letters and numbers, im doing the same logic as i did for the sprites, but i hardcoded the mapping of the chrars using the mapping i have
*/
void drawChar(uint8_t x, uint8_t y, char c, uint16_t color, uint8_t scale) {
    int index = -1;

    switch (c) {
        case ' ': index = 0; break;
        case 'A': index = 1; break;
        case 'B': index = 2; break;
        case 'C': index = 3; break;
        case 'D': index = 4; break;
        case 'E': index = 5; break;
        case 'F': index = 6; break;
        case 'G': index = 7; break;
        case 'H': index = 8; break;
        case 'I': index = 9; break;
        case 'J': index = 10; break;
        case 'K': index = 11; break;
        case 'L': index = 12; break;
        case 'M': index = 13; break;
        case 'N': index = 14; break;
        case 'O': index = 15; break;
        case 'P': index = 16; break;
        case 'Q': index = 17; break;
        case 'R': index = 18; break;
        case 'S': index = 19; break;
        case 'T': index = 20; break;
        case 'U': index = 21; break;
        case 'V': index = 22; break;
        case 'W': index = 23; break;
        case 'X': index = 24; break;
        case 'Y': index = 25; break;
        case 'Z': index = 26; break;
        case '0': index = 27; break;
        case '1': index = 28; break;
        case '2': index = 29; break;
        case '3': index = 30; break;
        case '4': index = 31; break;
        case '5': index = 32; break;
        case '6': index = 33; break;
        case '7': index = 34; break;
        case '8': index = 35; break;
        case '9': index = 36; break;
        case ':': index = 37; break;
        default: return; 
    }


    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = font[index][i]; 
        for (uint8_t j = 0; j < 8; j++) {
            if ((line >> j) & 0x01) {
                if (scale == 1) {
                    drawPixel(x + i, y + j, color); //just draw the pixels to "fill in " the words.
                }
                else { //same as the sprite, if I need it bigger just scale it up or down
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            drawPixel(x + (i * scale) + sx, y + (j * scale) + sy, color);
                        }
                    }
                }
            }
        }
    }
}
//instead of drawing each char individually, this will just loop thru the string and draw the chars.
void drawString(uint8_t x, uint8_t y,const char* str, uint16_t color, uint8_t scale) {
    while (*str) {
        drawChar(x, y, *str, color, scale); 
        x += 6 * scale; 
        str++; 
    }
}

void drawTwoDigits(uint8_t x, uint8_t y, int val, uint16_t color, uint8_t scale) { 
    char tens = (val / 10) + '0';  //tens (i only did 10s bc its only a 30 second game, and no one would score over 99)
    char ones = (val % 10) + '0'; 
    drawChar(x, y, tens, color, scale);
    drawChar(x + (6 * scale), y, ones, color, scale);
}


enum timer_states { timer_start, timer_count };

int tickfct_timer(int state) {
    switch (state) {
        case timer_start:
            game_timer_sec = 30;  //changed to 30 from 60 so the demos are not too long
            time_up = 0; 
            state = timer_count;
            break;

        case timer_count:
            if (reset_flag) { 
                game_timer_sec = 30; 
                time_up = 0;
                game_over_flag = 0; 
                score = 0; 
                pX = 60; 
                pY = 110; 
                prev_pX = 60;
                resetAliens(); 
                fillScreen(0x0000); 
                drawString(0, 0, "SC:", 0xFFFF, 1); //if I reset, redraw the score to 0 and the score thingy (in case it comes from game over)
                drawTwoDigits(20, 0, score, 0xFFFF, 1);
                drawSprite(pX, pY, shipSprite, 5, 8, 0x07E0, 3);
                reset_flag = 0; 
            } 
            else if (game_timer_sec > 0) {
                game_timer_sec--;
                
                if (game_timer_sec == 20) {
                    buzzer_flag = 1; 
                }
                else if (game_timer_sec == 10) { //beep at 3 different instances
                    buzzer_flag = 2; 
                }
                else if (game_timer_sec == 5) {
                    buzzer_flag = 3; 
                }
                else if (game_timer_sec == 0) {
                    buzzer_flag = 4; 
                    time_up = 1; 
                }
            }
            state = timer_count;
            break;
    }
    return state;
}

//same as lab 7

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
          
            
            //set up the x axis thingy
            if (x < 300){    
                direction = 'L';    
            }    
            else if (x > 700){    
                direction = 'R';  //setting up flags to use in other sm's
            }    
            else {    
                direction = '-'; 
            }
            
            //to do: make sure it doesnt go out of bounds
            break;
    }
    return state;
}


// button task -> currently working perfectly, checkpoint !
enum button_state { game_start }; //this is working properly

int tickfct_button(int state) {
    switch (state) {
        case game_start:
            state = game_start;
            break;
        default:
            state = game_start;
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
    

    unsigned char prev_right = (prev_p & (1 << PC0));
    unsigned char curr_right = (curr_p & (1 << PC0));

//if the right button is pressed (ill just use this as the shooting button)
    if (prev_right && !curr_right && game_over_flag == 0) { //i added the game over flag here but this was not the issue. left it just to be safe
       shoot_flag = 1;  
       buzzer_flag = 6; 
    }
    

    unsigned char prev_left = (prev_p & (1 << PC1));
    unsigned char curr_left = (curr_p & (1 << PC1)); 

    if (prev_left && !curr_left) {  //if the reset button is pressed (fixed)
         reset_flag = 1;
         buzzer_flag = 5; 
    }
    
    prev_p = curr_p;    

    return state;
}


enum buzzer_states { 
    buzzer_off, 
    buzzer_time_n1, buzzer_time_w1, buzzer_time_n2, buzzer_time_w2, 
    buzzer_shoot_n1, buzzer_shoot_w1, buzzer_shoot_n2, buzzer_shoot_w2, 
    buzzer_reset_n1, buzzer_reset_w1, buzzer_reset_n2, buzzer_reset_w2, buzzer_reset_n3, buzzer_reset_w3,
    buzzer_song_n1, buzzer_song_w1,
    buzzer_song_n2, buzzer_song_w2,
    buzzer_song_n3, buzzer_song_w3,
    buzzer_song_n4, buzzer_song_w4,
    buzzer_song_n5, buzzer_song_w5,
    buzzer_song_n6, buzzer_song_w6 //for the end of the game little song
    
};
//fixed the buzzer so the sounds are a little nicer and not like screeching. the low frequencies dont really soound like anything.

int tickfct_buzzer(int state) {
    static unsigned int note_timer = 0;
    const unsigned int note_duration = 100; 
    const unsigned int song_duration = 150;

    switch (state) {
        case buzzer_off:
            buzz_off();
            if (buzzer_flag == 4) {         
                state = buzzer_song_n1;
                buzzer_flag = 0;
            }
            else if (buzzer_flag == 6) {         
                state = buzzer_shoot_n1;
                buzzer_flag = 0;            
            } 
            else if (buzzer_flag == 5) {    
                state = buzzer_reset_n1;
                buzzer_flag = 0;
            } 
            else if (buzzer_flag >= 1 && buzzer_flag <= 3 && !time_up) {
                state = buzzer_time_n1;
                buzzer_flag = 0;
            }
            else {
                state = buzzer_off;
            }
            break;

        case buzzer_time_n1:
            buzzTone(261); 
            note_timer = 0;
            state = buzzer_time_w1;
            break;

        case buzzer_time_w1:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_time_w1;
            } 
            else {
                state = buzzer_time_n2;
            }
            break;

        case buzzer_time_n2:
            buzzTone(329); 
            note_timer = 0;
            state = buzzer_time_w2;
            break;

        case buzzer_time_w2:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_time_w2;
            } 
            else {
                state = buzzer_off;
            }
            break;

        case buzzer_shoot_n1:
            buzzTone(900); //i liked the higher frequencies more because they sound cleaner, i couldnt get a nice pew pew sound with low freqs
            note_timer = 0;
            state = buzzer_shoot_w1;
            break;

        case buzzer_shoot_w1:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_shoot_w1;
            } 
            else {
                state = buzzer_shoot_n2;
            }
            break;

        case buzzer_shoot_n2:
            buzzTone(1200); 
            note_timer = 0;
            state = buzzer_shoot_w2;
            break;

        case buzzer_shoot_w2:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_shoot_w2;
            } 
            else {
                state = buzzer_off;
            }
            break;

        case buzzer_reset_n1:
            buzzTone(500); 
            note_timer = 0;
            state = buzzer_reset_w1;
            break;

        case buzzer_reset_w1:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_reset_w1;
            } 
            else {
                state = buzzer_reset_n2;
            }
            break;
        case buzzer_reset_n2:
            buzzTone(800); 
            note_timer = 0;
            state = buzzer_reset_w2;
            break;
        case buzzer_reset_w2:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_reset_w2;
            } 
            else {
                state =buzzer_reset_n3;
            }
            break;
        case buzzer_reset_n3:
            buzzTone(300); 
            note_timer = 0;
            state = buzzer_reset_w3;
            break;
        case buzzer_reset_w3:
            if (note_timer < note_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_reset_w3;
            } 
            else {
                state = buzzer_off;
            }
            break;
       
        case buzzer_song_n1:
            buzzTone(100); // 
            note_timer = 0;
            state = buzzer_song_w1;
            break;

        case buzzer_song_w1:
            if (note_timer < song_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w1;
            } 
            else {
                state = buzzer_song_n2;
            }
            break;

        case buzzer_song_n2:
            buzzTone(349); 
            note_timer = 0;
            state = buzzer_song_w2;
            break;

        case buzzer_song_w2:
            if (note_timer < song_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w2;
            } 
            else {
                state = buzzer_song_n3;
            }
            break;

        case buzzer_song_n3:
            buzzTone(329); 
            note_timer = 0;
            state = buzzer_song_w3;
            break;

        case buzzer_song_w3:
            if (note_timer < song_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w3;
            } 
            else {
                state = buzzer_song_n4;
            }
            break;

        case buzzer_song_n4:
            buzzTone(293); 
            note_timer = 0;
            state = buzzer_song_w4;
            break;

        case buzzer_song_w4:
            if (note_timer < song_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w4;
            } 
            else {
                state = buzzer_song_n5;
            }
            break;

        case buzzer_song_n5:
            buzzTone(261); 
            note_timer = 0;
            state = buzzer_song_w5;
            break;

        case buzzer_song_w5:
            if (note_timer < song_duration) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w5;
            } 
            else {
                state = buzzer_song_n6;
            }
            break;

        case buzzer_song_n6:
            buzzTone(196);
            note_timer = 0;
            state = buzzer_song_w6;
            break;

        case buzzer_song_w6:
            if (note_timer < song_duration * 2) {
                note_timer += TASK_BUZZER_PERIOD;
                state = buzzer_song_w6;
            } 
            else {
                state = buzzer_off;
            }
            break;
            
        default:
            state = buzzer_off;
            break;
    }
    return state;
}
/*
next thing I will do is the RGB light 
and the timer function [it will count down from 60 to 0]
starts at green, at 30 seconds blinks yellow, at 15 seconds blinks red, at 0 seconds solid red
*/
enum rgb_states {rgb_start, rgb_halfway, rgb_quarter, rgb_timeup};
//at every stage, check if the timer hasnt changed or antyhing. done. checkpoint.
int tickfct_rgb(int state) {
    static unsigned int blink_cnt = 0;
    static unsigned char blink_on = 0;

    const unsigned char RGB_MASK = RGB_R | RGB_G | RGB_B; 

    switch(state) {
 
        case rgb_start:
            if (game_timer_sec <= 0){
                 state = rgb_timeup;
            }
            else if (game_timer_sec <= 5){
                state = rgb_quarter;
            }
            else if (game_timer_sec <= 15){ 
                state = rgb_halfway;
            }
            else if(game_over_flag==1){
                state = rgb_start;
            }
            else{
                state = rgb_start;
            }
            break;

        case rgb_halfway:
            if (game_timer_sec <= 0){
                state = rgb_timeup;
            }
            else if (game_timer_sec <= 5){
                 state = rgb_quarter;
            }
            else if (game_timer_sec > 15) {
                state = rgb_start;
            }
            else if(game_over_flag==1){
                state = rgb_start;
            }

            else {
                state = rgb_halfway;
            }
            break;

        case rgb_quarter:
            if (game_timer_sec <= 0){ 
                state = rgb_timeup;
            }
            else if (game_timer_sec > 15) {
                state = rgb_halfway;
            }
            else if(game_over_flag==1){
                state = rgb_start;
            }
            else {
                state = rgb_quarter;
            }
            break;

        case rgb_timeup:
            if (reset_flag || game_over_flag==1){
                state = rgb_start;
            }
            else{ 
                state = rgb_timeup;
            }
            break;

        default:
            state = rgb_start;
            break;
    }

    switch(state) {
        case rgb_start:
            PORTD = (PORTD & ~RGB_MASK) | RGB_G;
            blink_cnt = 0;
            blink_on = 0;
            break;

        case rgb_halfway:
            blink_cnt += TASK_RGB_PERIOD;
            if (blink_cnt >= 500) {
                blink_cnt = 0;
                blink_on = !blink_on;
            }
            if (blink_on) {
                
                PORTD = (PORTD & ~RGB_MASK) | (RGB_R | RGB_G);
            } else {
                
                PORTD = (PORTD & ~RGB_MASK);
            }
            break;

        case rgb_quarter:
            blink_cnt += TASK_RGB_PERIOD;
            if (blink_cnt >= 500) {
                blink_cnt = 0;
                blink_on = !blink_on;
            }
            if (blink_on) {
               
                PORTD = (PORTD & ~RGB_MASK) | RGB_R;
            } else {
                PORTD = (PORTD & ~RGB_MASK);
            }
            break;

        case rgb_timeup:
            PORTD = (PORTD & ~RGB_MASK) | RGB_R;
            blink_cnt = 0;
            blink_on = 0;
            break;
    }

    return state;
}

enum player_states { player_move };
int tickfct_player(int state) {

    if (game_over_flag || time_up) return state;
    prev_pX = pX; //store the previous position
    // use the flags i set earlier for the joystick (working)
    if (direction == 'L') {
        pX -= 3; // Move Left
    } 
    else if (direction == 'R') {
        pX += 3; // Move Right
    }
    // check for the boundaries
    if (pX < 0)   pX = 0;   // left wall boundary
    if (pX > 118) pX = 118; // right wal boudnadry 

    if (pX != prev_pX) { //if the position changed
        drawSprite(prev_pX, pY, shipSprite, 5, 8, 0x0000, 3); //draw the previous position in black to erase it
        drawSprite(pX, pY, shipSprite, 5, 8, 0x07E0, 3); //draw the new one in green (as notmal). it works yay
    }
    if(shoot_flag==1){
        drawSprite(pX, pY, shipSprite, 5, 8, 0x07E0, 3); //redraw thr ship bc the bullets where erasing part of it.
    }

    //drawSprite(90, 100, shipSprite, 5, 8, 0x07E0, 3);

    return player_move;
}

//now i'll do the bullet / shooting. im going to make a state machine that will read the button flag (shooting)
//if the shooting is active, it will track the location of the ship and shoot upwards from there. it should have
// the option to reset once it reaches the top of the screen.

volatile int bX = 0; //these will keep track of the bullet position
volatile int bY = 0;
volatile int prev_bY = 0; //ill use this to clear the previous position to avoid the tracks
unsigned char bullet_active = 0;

enum bullet_states { bullet_wait, bullet_fly };
int tickfct_bullet(int state) {
    switch(state) {
        case bullet_wait:
            if (shoot_flag && !game_over_flag && !time_up) {  
                bX = pX + 5;  //center the bullet to come off from the tallest part of the ship
                bY = pY - 6; 
                prev_bY = bY; 
                bullet_active = 1; 
                shoot_flag = 0; 
                state = bullet_fly;
            }
            break;
// if the bullet catches an alien, I will do the same lofic i did for the ship, it will erase the alien and move it t the top
//that way i dont need to keep creating new aliens, just send them back to the beginning.
//im also adding the score thing here since this is where the hit detection is.
        case bullet_fly:
            if (bullet_active) {
                prev_bY = bY; 
                bY -= 5; 
                for (int i = 0; i < MAX_ALIENS; i++) {
                    if ( (bX >= aliens[i].x) && (bX <= (aliens[i].x + 15)) &&
                         (bY >= aliens[i].y) && (bY <= (aliens[i].y + 24)) ) { // if the bullet overlaps w an aliens position
                        drawSprite(aliens[i].x, aliens[i].y, alienSprite, 5, 8, 0x0000, 3);
                        drawSprite(bX, prev_bY, bulletSprite, 5, 8, 0x0000, 3);
                        aliens[i].y = -15; 
                        aliens[i].x = (bX + 37) % 110; 
                        drawTwoDigits(20, 0, score, 0x0000, 1); //erase the previous score
                        score++; 
                        drawTwoDigits(20, 0, score, 0xFFFF, 1);  //draw the new score
                        bullet_active = 0;
                        state = bullet_wait;
                        buzzer_flag = 7; //i was going to add a buzzer, but tbh theres too much noise going on already
                        return state;
                    }
                }
                if (bY < 0) { 
                    drawSprite(bX, prev_bY, bulletSprite, 5, 8, 0x0000, 3);
                    bullet_active = 0; 
                    state = bullet_wait;
                } 
                else { 
                    drawSprite(bX, prev_bY, bulletSprite, 5, 8, 0x0000, 3);
                    drawSprite(bX, bY, bulletSprite, 5, 8, 0xF800, 3); 
                }
            }
            break;
    }
    return state;
}


enum alien_states { alien_move };

int tickfct_aliens(int state) {
    if (game_over_flag || time_up){ return state;
    }

    for (int i = 0; i < MAX_ALIENS; i++) {
        aliens[i].prev_y = aliens[i].y;
        aliens[i].y += 1;  //the speed the aliens will fall (i did 2 at first but it was lowkey too hard)

        if (aliens[i].y > 115) { //if the alien is at the bofttom 
            game_over_flag = 1; 
            buzzer_flag = 4;   
        }


        if (aliens[i].y > 0 && aliens[i].y < 128) { //updatr aliens position (draw it black first to erase previous)
            if (aliens[i].prev_y > 0) {
                drawSprite(aliens[i].x, aliens[i].prev_y, alienSprite, 5, 8, 0x0000, 3);
            }
            drawSprite(aliens[i].x, aliens[i].y, alienSprite, 5, 8, 0xFFFF, 3);
        }
    }
    return state;
}

void TimerISR() {
    for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {    
        if ( tasks[i].elapsedTime >= tasks[i].period ) {    
            tasks[i].state = tasks[i].TickFct(tasks[i].state);    
            tasks[i].elapsedTime = 0;    
        }

        tasks[i].elapsedTime += GCD_PERIOD;    
    }
}

//im making this sm just so i can display game over at the end, so i can reset everything properly
//bc i was getting an issue of when the game was over the game would still go on.
enum gameover_states { go_wait, go_countdown };

int tickfct_gameover(int state) {
    static int wait_timer = 0; 

    switch(state) {
        case go_wait:
            if (game_over_flag || time_up) {
                fillScreen(0x0000); //fill the scree w black
                drawString(25, 40, "GAME OVER", 0xF800, 2); //write the game over
                drawString(40, 70, "SCORE:", 0xFFFF, 1);    //write the score
                drawTwoDigits(80, 70, score, 0xFFFF, 1); //output the score
                wait_timer = 0; 
                state = go_countdown;
            }
            break;

        case go_countdown:
            if (game_over_flag == 0 && time_up == 0) {
                state = go_wait;
                break; 
            }
            if (wait_timer < 300) { 
                wait_timer++; 
            } 
            else {
                game_over_flag = 0; 
                time_up = 0; 
                reset_flag = 1; 
                state = go_wait;
            }
            break;
    }
    return state;
}

int main(void) {

    DDRB = 0xFF; PORTB = 0x00; 
    DDRC  = 0x00; 
    PORTC = (1 << PC0) | (1 << PC1); //buttons
    DDRD = 0xFF; // 
    PORTD = 0x00;
    
    buzz_off();
    resetAliens();
    
    ADC_init();
    SPI_INIT();
    ST7735_init();
    fillScreen(0x0000); // fill the screen with black [which will be the background of my game]
    // drawSprite(50, 50, alienSprite, 5, 8, 0xFFFF, 3); //im going to draw some aliens for the beginning just for the demo.
    // drawSprite(80, 40, alienSprite, 5, 8, 0xFFFF, 3);
    // drawSprite(40, 80, alienSprite, 5, 8, 0xFFFF, 3);
    // drawSprite(20, 20, alienSprite, 5, 8, 0xFFFF, 3);
    // drawSprite(90, 20, alienSprite, 5, 8, 0xFFFF, 3);
    // drawSprite(5, 88, alienSprite, 5, 8, 0xFFFF, 3); now the aliens are falling from the top so i dont need these anymore
    // drawSprite(99, 99, alienSprite, 5, 8, 0xFFFF, 3);
    //drawSprite(90, 100, shipSprite, 5, 8, 0x07E0, 3); //drawing the spaceship at the start. the ship drawing is "bigger", so i scaled it down a little bit
    drawString(0, 0, "SC:", 0xFFFF, 1); //output the score at the befinning
    drawTwoDigits(20, 0, 0, 0xFFFF, 1);
    drawSprite(60, 110, shipSprite, 5, 8, 0x07E0, 3);
    unsigned char i = 0;

   
    tasks[i].period = TASK_JOYSTICK_PERIOD;
    tasks[i].state = joystick_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_joystick;
    i++;

    tasks[i].period = TASK_BUTTON_PERIOD;
    tasks[i].state = game_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_button;
    i++;
 
    tasks[i].period = TASK_BUZZER_PERIOD;
    tasks[i].state = buzzer_off;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_buzzer;
    i++;

    tasks[i].period = TASK_TIMER_PERIOD;
    tasks[i].state = timer_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_timer;
    i++;

    tasks[i].period = TASK_RGB_PERIOD;
    tasks[i].state = rgb_start;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_rgb;
    i++;

    tasks[i].period= TASK_JOYSTICK_PERIOD;
    tasks[i].state= player_move;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_player;
    ++i;

    tasks[i].period= 30;
    tasks[i].state= bullet_wait;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickfct_bullet;
    ++i;

    tasks[i].period = 100;
    tasks[i].state = alien_move;
    tasks[i].TickFct = &tickfct_aliens;
    i++;
    tasks[i].period = 100;
    tasks[i].state = go_wait;
    tasks[i].TickFct = &tickfct_gameover;
    i++;

    TimerSet(GCD_PERIOD);
    TimerOn(); 
    while (1) {}
    return 0;
}


//CUSTOM LAB PROJECT TIME !
// things I will be recyclig from this lab:
/*
joystick function, we only need x axis since it only goes left and right. 
active buzzer will beep slower or faster depending on the time there is left 
I will have the stepper motor just to show that the joystick is functioning
SCENARIO / GAME FLOW:
- timer starts: the game is 60 seconds long. 
    - The game ends either after 60 seconds or if the alien reaches the bottom of the screen
    - Part one (baseline) it will just use the timer, the game loss won't be accounted for 
    - For this week I will just implement the timer and the buzzer, and get rid of servo motor

Timer task: 
    -count down from 60 seconds
    - creates 4 flags:
        - 45 seconds left (1) 
        - 30 seconds left (2) 
        - 15 seconds left (3)
        - time up         (4)
    -these flags will be variables and used in the buzzer to determine which tones to play

3 STAGES OF DEMOS:
was planning on showing the screen working, but my screen was on back order and will only arrive Saturday.
1. Timer and Buzzer + FLags:
    - timer counts down from 60 to 0
    - buzzer beeps at 45, 30, 15 seconds left
    - at 0 seconds, buzzer plays a song
    -at 0 seconds, light is red and solid
    - the shooting and reset buttons are implemented and functional, they set a flag that will be used for game functionality.

2. 
    - Screen will be working, characted will move left and right with joystick 
    - shooting button will make the character shoot upward (no hit detection yet)
    - score thing will be displayed
    - sprites will be (hopefully) finished
    updates : all of these are done except for the score and the game over. 

3. 
    - hit detection will be implemented -> done
    - game over conditions will be implemented  done
    - game start screen too. decided not to do this
    - high score saving will be implemented ->not doing this anymore 
    - final tweaks and improvements done yay



    FINAL CHECKPOINT deliverables:
    when the reset button is pressed, it should go back to the "home scree", reset the score and the timer
    organize the whole code so it's easier to read and understand 

*/
