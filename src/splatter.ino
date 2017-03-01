#include <Arduino.h>

void splatter() {

  maxChanges = 20;  // Value for blending speed between palettes

  changePalette();
  INDEX = random(10,240);

  //MILLISECONDS  = 10;
  //lowPass_audio = 0.15;
  //READ_AUDIO();

  EVERY_N_MILLISECONDS(52){
    fadeToBlackBy(leds,NUM_LEDS,1);
    blur1d(leds, NUM_LEDS, 140);
  }

    //MILLISECONDS = beatsin8(50,20,400); // oscillating sin wave controlling
                                          // raindrop drawing every 20-400ms, 50bmp
    EVERY_N_MILLISECONDS(40) {

      //if (mono[band] */ or mono_volume /*  > 10) { // if audio crosses a threshold
      pos = random16(NUM_LEDS - 1);
      left_pos  = pos - random(4, 14);
      right_pos = pos + random(4, 14);
      if (left_pos < 0) {
        left_pos = 0;
      }
      if (right_pos > NUM_LEDS - 1) {
        right_pos = NUM_LEDS - 1;
      }
      for (int i = left_pos; i < right_pos; i++) {
        leds[i] += ColorFromPalette(gCurrentPalette, INDEX, 150, LINEARBLEND);

      }
      //} // end audio threshold
    }


  }

// void addGlitter( fract8 chanceOfGlitter)
// {
//   if ( random8() < chanceOfGlitter) {
//     leds[ random16(NUM_LEDS) ] += CRGB::White;
//   }
// }
