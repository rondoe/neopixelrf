#include "FastLED.h"
#include <MySensor.h>
#include <Timer.h>
#include <SPI.h>

// SPI Pins
#define CE_PIN 48
#define CS_PIN 49

#define RF69_IRQ_PIN 0

// burgoboard 9, 10
// arduino mega 48,49
MyTransportNRF24 transport(CE_PIN,CS_PIN);
MyHwATMega328 hw;
MySensor gw(transport, hw);


// How many leds in your strip?
int numberLeds = 1;
#define FRAMES_PER_SECOND  120
#define MAX_BRIGHTNESS 50

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 38

#define CLOCK_PIN 13

// DETERMINE MAX NUMBER OF LEDS IN RAM
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
#define MAX_LEDS          300
#elif  defined(__AVR_ATmega2560__)
#define MAX_LEDS          900
#else
#define MAX_LEDS          300
#endif

// Define the array of leds => 60 leds/m => 15m
CRGB leds[MAX_LEDS];

// Initialize temperature message
MyMessage msg(0, V_RGB);
MyMessage msgBrightness(0, V_DIMMER);
MyMessage msgLight(0, V_LIGHT);



byte brightness = 50;
void saveColor(byte r, byte g, byte b) {
  gw.saveState(0, r);
  gw.saveState(1, g);
  gw.saveState(2, b);
}



void sendLight(byte status) {
  msgLight.setType(V_LIGHT);
  msgLight.set(status);
  gw.send(msgLight);
}

void setColor(byte r, byte g, byte b) {

  for (int i = 0; i < numberLeds; i++) {
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
  }

}

void sendColor(byte r, byte g, byte b) {
  char rgbBuffer[6];
  sprintf(rgbBuffer, "%02X%02X%02X", r, g, b);

  msg.setType(V_RGB);
  msg.set(rgbBuffer);
  // gw.send(msg);
}

void sendBrightness(byte val) {
  msgBrightness.setType(V_DIMMER);
  msgBrightness.set(val);
  gw.send(msgBrightness);
}


void setBrightness(byte dim) {
  // Return if 0, user must turn off
  if(dim ==0 ) return;
  FastLED.setBrightness( dim) ;
    brightness = dim;
}

void saveBrightness(byte val) {
  gw.saveState(3, val);
}

void off() {
  setColor(0, 0, 0);
  FastLED.setBrightness( 0) ;
}


void on() {

  // warm white
  // setColor(255, 147, 41);
  byte r = gw.loadState(0);
  byte g = gw.loadState(1);
  byte b = gw.loadState(2);
  if (r == 0x0 && g == 0x0 && b == 0x0) {
    r = 255;
    g = 147;
    b = 41;
  }

  brightness = gw.loadState(3);
  if(brightness == 0) brightness = 50;

  setColor(r, g, b);
  setBrightness(brightness);
}

void incomingMessage(const MyMessage &message) {
  if (message.type == V_LIGHT) {
    byte val = atoi(message.data);
    if (val == 1) {
      // todo: load from settings here
      on();
      sendLight(1);

    }
    if (val == 0) {
      off();
      sendLight(0);
    }
  }

  if (message.type == V_DIMMER) {
    // Get rid of '#' and convert it to integer
    // Get rid of '#' and convert it to integer
    byte val = map(message.getLong(), 0, 100, 0, 255);
    brightness = val;
    setBrightness(brightness);
    sendBrightness(message.getLong());
    saveBrightness(brightness);
  }

  if (message.type == V_RGB) {
    // it's rgb crossFade to it
    if (strlen(message.data) < 6) return;

    // Get rid of '#' and convert it to integer
    char subbuff[6];
    memcpy( subbuff, &message.data[0], 6 );


    long number = (long) strtol(subbuff, NULL, 16);

    // Split them up into r, g, b values
    byte r = number >> 16;
    byte g = number >> 8 & 0xFF;
    byte b = number & 0xFF;

    setColor(r, g, b);
    sendColor(r, g, b);
        saveColor(r, g, b);

  }


  // set number of leds
  if (message.type == V_VAR1) {
    off();
    FastLED.show(); // display this frame

    numberLeds  = atoi(message.data);
    // reset the fastled
    FastLED.clear(true);
    // Get rid of '#' and convert it to integer
    // Get rid of '#' and convert it to integer
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, numberLeds);

    // turn on the new leds
    on();
  }


  FastLED.show(); // display this frame

}



void setup() {
  delay(50);

  gw.begin(incomingMessage, 51, true);
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("LED strip /w repeater", "1.2");
  // Register the LED Dimmable Light with the gateway
  gw.present( 0, S_RGB_LIGHT);

  // request V_VAR1 => number of leds from master
  // gw.request(255, V_VAR1, 0);


  // first reset all to black
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, MAX_LEDS);
  for (int i = 0; i < MAX_LEDS; i++) {
    leds[i].r = 0;
    leds[i].g = 0;
    leds[i].b = 0;
  }
  FastLED.show(); // display this frame
  // reset the fastled
  FastLED.clear(true);

  // Uncomment/edit one of the following lines for your leds arrangement.
  // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, numberLeds);
  // FastLED.addLeds<APA104, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<P9813, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<APA102, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<DOTSTAR, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);

  on();
  FastLED.show(); // display this frame





  // enable watchdog timer to enable resets on malfunction
  wdt_enable(WDTO_1S);
}


void loop() {
  // the program is alive...for now.
  wdt_reset();

  gw.process();

}
