#include "mrf24j.h"
#include <SPI.h>
#include <SoftPWM.h>
#include <SoftPWM_timer.h>
/*
 * uiGC0
 * Goofy Controller 0
 * common cathode LEDs +
 * Mrf24j40
 *
 * Benjamin Jeffery
 * University of Idaho
 * 10/09/2015
 *
 * Edited By Hayden Carroll
 * University of Idaho
 * 5/2/2022
 *
 */

int NUM_PACKETS = 3;
int RED_PIN_LOC = 3;   // red pin location
int GREEN_PIN_LOC = 4; // green pin location
int BLUE_PIN_LOC = 5;  // blue pin location
int pin_reset = 6;
int pin_cs = 8;
int DIP8_PIN_LOC = 10;
int DIP7_PIN_LOC = 9;
int pin_interrupt = 2;

unsigned int dipSwitch = 0; // value of the dipswitch for the board
unsigned int packetNum;
unsigned int startId; // value of where in the packet to read from (dipswitch + offset)
Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

void setup() {
    Serial.begin(38400);
    DDRC = 0x00;

    pinMode(RED_PIN_LOC, OUTPUT);   // sets the pin as output
    pinMode(GREEN_PIN_LOC, OUTPUT); // sets the pin as output
    pinMode(BLUE_PIN_LOC, OUTPUT);  // sets the pin as output

    setDipSwitch();
    startId = dipSwitch + 1;

    packetNum = dipSwitch / (256 / NUM_PACKETS);
    Serial.println("\n The dipswitch value is: ");
    Serial.println(dipSwitch);

    Serial.println("\n The packet num is: ");
    Serial.println(packetNum);
    
    //  SoftPWMBegin();
    //  SoftPWMSet(RED_PIN_LOC, 0);
    //  SoftPWMSet(GREEN_PIN_LOC, 0);
    //  SoftPWMSet(BLUE_PIN_LOC, 0);
    //  SoftPWMSetFadeTime(RED_PIN_LOC, 100, 1000);
    //  SoftPWMSetFadeTime(GREEN_PIN_LOC, 10, 10);
    //  SoftPWMSetFadeTime(BLUE_PIN_LOC, 10, 10);
    setColor(0, 0, 0);
    mrf.reset();
    mrf.init();
    mrf.set_pan(2015);
    mrf.set_channel(0x0C);
    mrf.address16_write(0x4202);
    mrf.set_promiscuous(true);
    mrf.set_bufferPHY(true);

    attachInterrupt(0, interrupt_routine, CHANGE);
    interrupts();
}

void setDipSwitch() {
    // The following pins are connected to the rotary or DIP switch that sets
    // the channel. They are all inputs, but we write to them to enable the
    // internal pullup resistor.
    
    pinMode(A0, INPUT_PULLUP);
    pinMode(A1, INPUT_PULLUP);
    pinMode(A2, INPUT_PULLUP);
    pinMode(A3, INPUT_PULLUP);
    pinMode(A4, INPUT_PULLUP);
    pinMode(A5, INPUT_PULLUP);
    pinMode(DIP7_PIN_LOC, INPUT_PULLUP);
    pinMode(DIP8_PIN_LOC, INPUT_PULLUP);

    dipSwitch = (digitalRead(DIP8_PIN_LOC))
                << 7; // get 8th dip switch bit, shift it to 8th pos
    dipSwitch =
        dipSwitch | (digitalRead(DIP7_PIN_LOC)
                     << 6); // get 7th dip switch bit, shift it to 7th pos
    dipSwitch = (dipSwitch | (PINC & 0x3F)); // get 1-6th dip switch bit
    dipSwitch = ~dipSwitch & 0xFF;
}

void interrupt_routine() { mrf.interrupt_handler(); }

void loop() { mrf.check_flags(&handle_rx, &handle_tx); }

void handle_rx() {
    rx_info_t *info = mrf.get_rxinfo();
    uint8_t currPackNum = info->rx_data[0];
    uint8_t encodedColor = info->rx_data[startId];
    uint8_t red, blue, green = 0;
    
    Serial.print("curr pack number ");
    Serial.print(currPackNum);
    Serial.print("\n");
    if (currPackNum == packetNum) {
      decodeColor(encodedColor, &red, &green, &blue);
    }

    if (currPackNum == NUM_PACKETS - 1) {
       setColor(red, green, blue);

    }

    mrf.rx_flush();
}

void handle_tx() {}

void setColor(int red, int green, int blue) {
    //  SoftPWMSet(RED_PIN_LOC, red);
    //  SoftPWMSet(GREEN_PIN_LOC, green);
    //  SoftPWMSet(BLUE_PIN_LOC, blue);
    analogWrite(RED_PIN_LOC, red); // reds intensity is way too high
    analogWrite(GREEN_PIN_LOC, green);
    analogWrite(BLUE_PIN_LOC, blue);
}

// takes in the encoded color, decodes it,
// and changes the red, green, and blue values accordingly
void decodeColor(uint8_t encodedColor, uint8_t *red, uint8_t *green,
                 uint8_t *blue) {
    *blue = encodedColor & 0b00000011;
    encodedColor = encodedColor >> 2;
    *green = encodedColor & 0b00000111;
    encodedColor = encodedColor >> 3;
    *red = encodedColor & 0b00000111;

    *red = (*red / 7.f) * 255;
    *green = (*green / 7.f) * 255;
    *blue = (*blue / 3.f) * 255;
}
