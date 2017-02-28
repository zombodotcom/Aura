#include <Arduino.h>

void splatter() {

  changePalette();

  //MILLISECONDS  = 10;
  //lowPass_audio = 0.15;

  //READ_AUDIO();

  EVERY_N_MILLISECONDS(1000) {
    hue+=10;
  }

//EVERY_N_MILLISECONDS(15){
  //fadeToBlackBy(leds,NUM_LEDS,1);
//}


  //fadeToBlackBy( leds, NUM_LEDS, 1);

  blur1d(leds, NUM_LEDS, 100);

  //for (int band = 0; band < 7; band++) {

    //hue = band * 36;
    //INDEX = random(0, 8);

    //hue = ColorFromPalette(gCurrentPalette, INDEX, 255, LINEARBLEND);

    //MILLISECONDS = beatsin8(1,40,800);
    EVERY_N_MILLISECONDS(50) {
      //if (mono[band] > 10) {
      pos = random16(NUM_LEDS - 1);
      left_pos  = pos - random(2, 10);
      right_pos = pos + random(2, 10);
      if (left_pos < 0) {
        left_pos = 0;
      }
      if (right_pos > NUM_LEDS - 1) {
        right_pos = NUM_LEDS - 1;
      }
      //}
    }

    for (int i = left_pos; i < right_pos; i++) {
      leds[i] = CHSV( hue, 220, 220);
    }


  }
//}

void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
