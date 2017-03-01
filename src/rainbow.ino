#include <Arduino.h>
float Hue = 0;
int state = 0;

void rainbow() {

  hue = 0;
  EVERY_N_MILLISECONDS(50) {
    ihue--;
    //if (ihue > 255) ihue = 0;
  }
  fill_rainbow(leds, NUM_LEDS, hue, 1);

  for(int i = 0; i < NUM_LEDS; i++){
    hue = hue + 3;
    if (hue > 255) hue = 0;
    leds[i] = CHSV(hue + ihue, saturation,255);
  }

 if(state == 0){
   saturation -= 7;
   if(saturation < 10) state = 1;
 }

 if(state == 1){
    saturation += 7;
    if(saturation > 245) state = 0;
 }

  //addGlitter(80);

}
