/*

   Written by Drew André using Daniel Garcia and Mark Kriegsman's FastLED library.
   Eagle files..

*/


#include <Arduino.h>
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#define SERIAL_TIMEOUT 5
//#include <MSGEQ7.h> // to be built

int btBOOL = 1;              //-ACTIVATE BLUETOOTH OR NOT
byte inbyte;                 //-SERIAL INPUT BYTE
int thisarg;                 //-SERIAL INPUT ARG
#define SOFT_RX_PIN 2         // BTCHIP-TX -> NANO-SOFT-RX (2)
#define SOFT_TX_PIN 3         // BTCHIP-RX -> NANO-SOFT-TX (3)
bool DEBUG = true;
SoftwareSerial btSerial(SOFT_RX_PIN, SOFT_TX_PIN);
// WS2812B SETUP
const uint8_t DATA_PIN = 7;
const uint8_t NUM_LEDS = 144;
int HALF_POS = (NUM_LEDS * 0.5) - 1;
//const uint8_t HALF_POS = 14;
CRGB leds[NUM_LEDS];
const uint8_t FRAMES_PER_SECOND = 300;

int ledMode = 1;

// MSGEQ7 SETUP and SMOOTHING (eventually replace by MSGEQ7 class)
const uint8_t AUDIO_LEFT_PIN = 0, AUDIO_RIGHT_PIN = 1;
const uint8_t MSGEQ7_STROBE_PIN = 4, MSGEQ7_RESET_PIN = 5;
float lowPass_audio = 0.15;
uint8_t filter_min = 120;
const uint8_t left_filter_max, right_filter_max, filter_max = 1016;
const float EQ[7] = {0, 0, 0.1, 0.1, 0.1, 0.1, 0}; /* EQ[7] = {0, 0, 0, 0, 0, 0, 0}; */
const uint8_t sensitive_min = filter_min - (filter_min * .15);
const float FILTER_MIN[7] = {filter_min, filter_min, filter_min, filter_min, filter_min, filter_min, filter_min};
uint8_t new_left, new_right, prev_left, prev_right, prev_value, left_value, right_value;
uint8_t left_volume, right_volume, mono_volume;
uint8_t left[7], right[7], mono[7];
uint8_t mapped_left[7], mapped_right[7], full_mapped[14], mapped[7], full_flex[7];
float left_factor, right_factor, mono_factor, full_factor;
float half_MAPPED_AMPLITUDE, half_MAPPED_LEFT_AMP, half_MAPPED_RIGHT_AMP;
uint8_t prev_left_amp, prev_right_amp;

// PALETTE SETUP
uint8_t maxChanges = 50; // speed for switching between palettes
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;
float colorIndex = (256 / float(NUM_LEDS));
// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;
CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

//NOISE SETUP
static uint16_t x, y, z, dist;
float scale = 20.00; //1 - ~4000 (shimmery, zoomed out)
//float SPEED = 1.00f;  //1-100 (fast)
uint8_t ioffset, joffset;

// HSV SETUP
int BRIGHTNESS = 255;
int saturation = 255;
uint8_t hue, ihue, encoderHue;
float fhue;
uint8_t spectrumWidth = 36;
float HUEcorrection = /*floor(255 / NUM_LEDS);*/ 1.7; // *ALMOST* hue range fits on strip length
float half_HUEcorrection = HUEcorrection * 2;
// unnecessary math, to be fixed
const uint8_t HUEZ[14] = {240, 200, 160, 120, 80, 40, 0, 0, 40, 80, 120, 160, 200, 240};

// OTHER
uint8_t ledindex, segment, INDEX, i, k, band;
float divFactor;

// EFFECT: radiate (stereo)
uint8_t zero_l, three_l, six_l, zero_r, three_r, six_r;

// EFFECT: flex (mono)
uint8_t current_hue, current_brightness, next_hue, next_brightness, pos, point;
// EFFECT: flex (stereo)
uint8_t left_current_brightness, left_next_brightness, right_current_brightness, right_next_brightness;
uint8_t left_next_hue, right_next_hue, left_current_sat, right_current_sat, left_next_sat, right_next_sat;
uint8_t left_pos, right_pos, left_point, right_point;

// EFFECT: audio data (mono and stereo)
const uint8_t segmentSize = 32;
const uint8_t half_segmentSize = floor(segmentSize * 0.5);
float non_mirror_divFactor = 1.00 / segmentSize;
const uint8_t segmentEnd[7] = {31, 22, 22, 22, 22, 13, 10};

// EFFECT: fire2012 (Daniel Garcia and Mark Kriegsman)
bool gReverseDirection = false;
const uint8_t COOLING  = 170, SPARKING = 10; // Higher chance = more roaring fire, Default = 120

int MILLISECONDS = 5;






int mode = 8;
//CRGB leds2[NUM_LEDS];
//CRGB leds3[NUM_LEDS];

void change_mode(int newmode) {

  //  EVERY_N_SECONDS(MILLISECONDS) {
  //    mode++;
  //    if (mode > 4) {
  //      mode = 0;
  //    }
  //  }
  switch (newmode) {
    case 0:
      flex_mono();
      break;
    case 1:
      radiate();
      break;
    case 2:
      audio_data();
      break;
    case 3:
      spectrum142();
      break;
    case 4:
      rainbow();
      break;
    case 5:
      flex();
      break;
    case 6:
      ambient();
      break;
    case 7:
      Fire2012();
      break;
    case 8:
      audio_data_stereo();
      break;
    case 9:
      spectrumcolor();
      break;
    case 10:
      sat_spectrum();
      break;
    case 11:
      strip_flex();
      break;
    case 12:
      print_audio();
      break;
    case 13:
      changePalette();
      map_palette(gCurrentPalette);
      break;
    case 14:
      pride();
      break;
    case 15:
      flex_stereo();
      break;
    case 16:
      strip_flex();
      break;
  }


  // set the blend ratio for the video cross fade
  // (set ratio to 127 for a constant 50% / 50% blend)
  //uint8_t ratio = beatsin8(5);

  // mix the 2 arrays together
  //for (int i = 0; i < NUM_LEDS; i++) {
  //  leds[i] = blend( leds2[i], leds3[i], ratio );
  //}
  ledMode = newmode;
  //FastLED.delay(1000 / FRAMES_PER_SECOND);

  /*
    FLEXFLEX();
    ambient();
    audio_spectrum();
    stereo_vu();
    flexflexstereo();
    flex_stereo();
    Fire2012(); // run simulation frame
  */

}


// Gradient Color Palette definitions for 33 different cpt-city color palettes.
//    956 bytes of PROGMEM for all of the palettes together,
//   +618 bytes of PROGMEM for gradient palette code (AVR).
//  1,494 bytes total for all 34 color palettes and associated code.

void changePalette() {
  EVERY_N_SECONDS(2) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_MILLISECONDS(3) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, maxChanges);
  }
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void map_palette(const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  //startindex--;
  fill_palette( leds, NUM_LEDS, startindex, 256 / NUM_LEDS, gCurrentPalette, 255, LINEARBLEND);
}

void FillLEDsFromPaletteColors()
{
  uint8_t brightness = 255;
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( gCurrentPalette, colorIndex, brightness, LINEARBLEND);
    colorIndex += 6;
  }
}

/*
   DEFINE_GRADIENT_PALETTE( heatmap_gp ) {
   0,     0,  0,  0,   //black
   128,   255,  0,  0,   //red
   224,   255,255,  0,   //bright yellow
   255,   255,255,255 }; //full white
   The first column specifies where along the palette's indicies (0-255),
   the gradient should be anchored, as shown in the diagram above.
   In this case, the first gradient runs from black to red, in index positions 0-128.
   The next gradient from red to bright yellow runs in index positions 128-224,
   and the final gradient from bright yellow to full white runs in index positions 224-255.
   Note that you can specify unequal 'widths' of the gradient segments to stretch or squeeze parts of the palette as desired.
*/

// Gradient palette "ib_jul01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/xmas/tn/ib_jul01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( ib_jul01_gp ) {
  0, 194,  1,  1,
  94,   1, 29, 18,
  132,  57, 131, 28,
  255, 113,  1,  1
};

// Gradient palette "es_vintage_57_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_57.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_vintage_57_gp ) {
  0,   2,  1,  1,
  53,  18,  1,  0,
  104,  69, 29,  1,
  153, 167, 135, 10,
  255,  46, 56,  4
};

// Gradient palette "es_vintage_01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_vintage_01_gp ) {
  0,   4,  1,  1,
  51,  16,  0,  1,
  76,  97, 104,  3,
  101, 255, 131, 19,
  127,  67,  9,  4,
  153,  16,  0,  1,
  229,   4,  1,  1,
  255,   4,  1,  1
};

// Gradient palette "es_rivendell_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/rivendell/tn/es_rivendell_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_rivendell_15_gp ) {
  0,   1, 14,  5,
  101,  16, 36, 14,
  165,  56, 68, 30,
  242, 150, 156, 99,
  255, 150, 156, 99
};

// Gradient palette "rgi_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/rgi/tn/rgi_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( rgi_15_gp ) {
  0,   4,  1, 31,
  31,  55,  1, 16,
  63, 197,  3,  7,
  95,  59,  2, 17,
  127,   6,  2, 34,
  159,  39,  6, 33,
  191, 112, 13, 32,
  223,  56,  9, 35,
  255,  22,  6, 38
};

// Gradient palette "retro2_16_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/retro2/tn/retro2_16.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.

DEFINE_GRADIENT_PALETTE( retro2_16_gp ) {
  0, 188, 135,  1,
  255,  46,  7,  1
};

// Gradient palette "Analogous_1_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/red/tn/Analogous_1.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Analogous_1_gp ) {
  0,   3,  0, 255,
  63,  23,  0, 255,
  127,  67,  0, 255,
  191, 142,  0, 45,
  255, 255,  0,  0
};

// Gradient palette "es_pinksplash_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_pinksplash_08_gp ) {
  0, 126, 11, 255,
  127, 197,  1, 22,
  175, 210, 157, 172,
  221, 157,  3, 112,
  255, 157,  3, 112
};

// Gradient palette "es_pinksplash_07_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_07.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_pinksplash_07_gp ) {
  0, 229,  1,  1,
  61, 242,  4, 63,
  101, 255, 12, 255,
  127, 249, 81, 252,
  153, 255, 11, 235,
  193, 244,  5, 68,
  255, 232,  1,  5
};

// Gradient palette "Coral_reef_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/other/tn/Coral_reef.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( Coral_reef_gp ) {
  0,  40, 199, 197,
  50,  10, 152, 155,
  96,   1, 111, 120,
  96,  43, 127, 162,
  139,  10, 73, 111,
  255,   1, 34, 71
};

// Gradient palette "es_ocean_breeze_068_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_068.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_068_gp ) {
  0, 100, 156, 153,
  51,   1, 99, 137,
  101,   1, 68, 84,
  104,  35, 142, 168,
  178,   0, 63, 117,
  255,   1, 10, 10
};

// Gradient palette "es_ocean_breeze_036_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_036.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_036_gp ) {
  0,   1,  6,  7,
  89,   1, 99, 111,
  153, 144, 209, 255,
  255,   0, 73, 82
};

// Gradient palette "departure_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/mjf/tn/departure.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 88 bytes of program space.

DEFINE_GRADIENT_PALETTE( departure_gp ) {
  0,   8,  3,  0,
  42,  23,  7,  0,
  63,  75, 38,  6,
  84, 169, 99, 38,
  106, 213, 169, 119,
  116, 255, 255, 255,
  138, 135, 255, 138,
  148,  22, 255, 24,
  170,   0, 255,  0,
  191,   0, 136,  0,
  212,   0, 55,  0,
  255,   0, 55,  0
};

// Gradient palette "es_landscape_64_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_64.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_landscape_64_gp ) {
  0,   0,  0,  0,
  37,   2, 25,  1,
  76,  15, 115,  5,
  127,  79, 213,  1,
  128, 126, 211, 47,
  130, 188, 209, 247,
  153, 144, 182, 205,
  204,  59, 117, 250,
  255,   1, 37, 192
};

// Gradient palette "es_landscape_33_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_33.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_landscape_33_gp ) {
  0,   1,  5,  0,
  19,  32, 23,  1,
  38, 161, 55,  1,
  63, 229, 144,  1,
  66,  39, 142, 74,
  255,   1,  4,  1
};

// Gradient palette "rainbowsherbet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/icecream/tn/rainbowsherbet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( rainbowsherbet_gp ) {
  0, 255, 33,  4,
  43, 255, 68, 25,
  86, 255,  7, 25,
  127, 255, 82, 103,
  170, 255, 255, 242,
  209,  42, 255, 22,
  255,  87, 255, 65
};

// Gradient palette "gr65_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr65_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( gr65_hult_gp ) {
  0, 247, 176, 247,
  48, 255, 136, 255,
  89, 220, 29, 226,
  160,   7, 82, 178,
  216,   1, 124, 109,
  255,   1, 124, 109
};

// Gradient palette "gr64_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr64_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( gr64_hult_gp ) {
  0,   1, 124, 109,
  66,   1, 93, 79,
  104,  52, 65,  1,
  130, 115, 127,  1,
  150,  52, 65,  1,
  201,   1, 86, 72,
  239,   0, 55, 45,
  255,   0, 55, 45
};

// Gradient palette "GMT_drywet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_drywet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( GMT_drywet_gp ) {
  0,  47, 30,  2,
  42, 213, 147, 24,
  84, 103, 219, 52,
  127,   3, 219, 207,
  170,   1, 48, 214,
  212,   1,  1, 111,
  255,   1,  7, 33
};

// Gradient palette "ib15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/general/tn/ib15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( ib15_gp ) {
  0, 113, 91, 147,
  72, 157, 88, 78,
  89, 208, 85, 33,
  107, 255, 29, 11,
  141, 137, 31, 39,
  255,  59, 33, 89
};

// Gradient palette "Fuschia_7_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/fuschia/tn/Fuschia-7.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Fuschia_7_gp ) {
  0,  43,  3, 153,
  63, 100,  4, 103,
  127, 188,  5, 66,
  191, 161, 11, 115,
  255, 135, 20, 182
};

// Gradient palette "es_emerald_dragon_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/emerald_dragon/tn/es_emerald_dragon_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_emerald_dragon_08_gp ) {
  0,  97, 255,  1,
  101,  47, 133,  1,
  178,  13, 43,  1,
  255,   2, 10,  1
};

// Gradient palette "lava_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/lava.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE( lava_gp ) {
  0,   0,  0,  0,
  46,  18,  0,  0,
  96, 113,  0,  0,
  108, 142,  3,  1,
  119, 175, 17,  1,
  146, 213, 44,  2,
  174, 255, 82,  4,
  188, 255, 115,  4,
  202, 255, 156,  4,
  218, 255, 203,  4,
  234, 255, 255,  4,
  244, 255, 255, 71,
  255, 255, 255, 255
};

// Gradient palette "fire_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/fire.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( fire_gp ) {
  0,   1,  1,  0,
  76,  32,  5,  0,
  146, 192, 24,  0,
  197, 220, 105,  5,
  240, 252, 255, 31,
  250, 252, 255, 111,
  255, 255, 255, 255
};

// Gradient palette "Colorfull_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Colorfull.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE( Colorfull_gp ) {
  0,  10, 85,  5,
  25,  29, 109, 18,
  60,  59, 138, 42,
  93,  83, 99, 52,
  106, 110, 66, 64,
  109, 123, 49, 65,
  113, 139, 35, 66,
  116, 192, 117, 98,
  124, 255, 255, 137,
  168, 100, 180, 155,
  255,  22, 121, 174
};

// Gradient palette "Magenta_Evening_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Magenta_Evening.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( Magenta_Evening_gp ) {
  0,  71, 27, 39,
  31, 130, 11, 51,
  63, 213,  2, 64,
  70, 232,  1, 66,
  76, 252,  1, 69,
  108, 123,  2, 51,
  255,  46,  9, 35
};

// Gradient palette "Pink_Purple_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Pink_Purple.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE( Pink_Purple_gp ) {
  0,  19,  2, 39,
  25,  26,  4, 45,
  51,  33,  6, 52,
  76,  68, 62, 125,
  102, 118, 187, 240,
  109, 163, 215, 247,
  114, 217, 244, 255,
  122, 159, 149, 221,
  149, 113, 78, 188,
  183, 128, 57, 155,
  255, 146, 40, 123
};

// Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
  0, 120,  0,  0,
  22, 179, 22,  0,
  51, 255, 104,  0,
  85, 167, 22, 18,
  135, 100,  0, 103,
  198,  16,  0, 130,
  255,   0,  0, 160
};

// Gradient palette "es_autumn_19_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/autumn/tn/es_autumn_19.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_autumn_19_gp ) {
  0,  26,  1,  1,
  51,  67,  4,  1,
  84, 118, 14,  1,
  104, 137, 152, 52,
  112, 113, 65,  1,
  122, 133, 149, 59,
  124, 137, 152, 52,
  135, 113, 65,  1,
  142, 139, 154, 46,
  163, 113, 13,  1,
  204,  55,  3,  1,
  249,  17,  1,  1,
  255,  17,  1,  1
};

// Gradient palette "BlacK_Blue_Magenta_White_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Blue_Magenta_White.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( BlacK_Blue_Magenta_White_gp ) {
  0,   0,  0,  0,
  42,   0,  0, 45,
  84,   0,  0, 255,
  127,  42,  0, 255,
  170, 255,  0, 255,
  212, 255, 55, 255,
  255, 255, 255, 255
};

// Gradient palette "BlacK_Magenta_Red_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Magenta_Red.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( BlacK_Magenta_Red_gp ) {
  0,   0,  0,  0,
  63,  42,  0, 45,
  127, 255,  0, 255,
  191, 255,  0, 45,
  255, 255,  0,  0
};

// Gradient palette "BlacK_Red_Magenta_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Red_Magenta_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( BlacK_Red_Magenta_Yellow_gp ) {
  0,   0,  0,  0,
  42,  42,  0,  0,
  84, 255,  0,  0,
  127, 255,  0, 45,
  170, 255,  0, 255,
  212, 255, 55, 45,
  255, 255, 255,  0
};

// Gradient palette "Blue_Cyan_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Blue_Cyan_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Blue_Cyan_Yellow_gp ) {
  0,   0,  0, 255,
  63,   0, 55, 255,
  127,   0, 255, 255,
  191,  42, 255, 45,
  255, 255, 255,  0
};


// Single array of defined cpt-city color palettes.
// This will let us programmatically choose one based on
// a number, rather than having to activate each explicitly
// by name every time.
// Since it is const, this array could also be moved
// into PROGMEM to save SRAM, but for simplicity of illustration
// we'll keep it in a regular SRAM array.
//
// This list of color palettes acts as a "playlist"; you can
// add or delete, or re-arrange as you wish.
const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  rgi_15_gp,
  Sunset_Real_gp,
  es_rivendell_15_gp,
  es_ocean_breeze_036_gp,
  retro2_16_gp,
  Analogous_1_gp,
  es_pinksplash_08_gp,
  Coral_reef_gp,
  es_ocean_breeze_068_gp,
  es_pinksplash_07_gp,
  es_vintage_01_gp,
  departure_gp,
  es_landscape_64_gp,
  es_landscape_33_gp,
  rainbowsherbet_gp,
  gr65_hult_gp,
  gr64_hult_gp,
  GMT_drywet_gp,
  ib_jul01_gp,
  es_vintage_57_gp,
  ib15_gp,
  Fuschia_7_gp,
  es_emerald_dragon_08_gp,
  lava_gp,
  fire_gp,
  Colorfull_gp,
  Magenta_Evening_gp,
  Pink_Purple_gp,
  es_autumn_19_gp,
  BlacK_Blue_Magenta_White_gp,
  BlacK_Magenta_Red_gp,
  BlacK_Red_Magenta_Yellow_gp,
  Blue_Cyan_Yellow_gp
};


// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount =
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );





// wake up the MSGEQ7
void InitMSGEQ7() {
  analogReference(DEFAULT);              // change
  pinMode(MSGEQ7_RESET_PIN,  OUTPUT);
  pinMode(MSGEQ7_STROBE_PIN, OUTPUT);
  pinMode(AUDIO_LEFT_PIN,   INPUT);
  pinMode(AUDIO_RIGHT_PIN,  INPUT);
  digitalWrite(MSGEQ7_RESET_PIN, LOW);
  digitalWrite(MSGEQ7_STROBE_PIN, LOW);
  //RESET MSGEQ7
  digitalWrite(MSGEQ7_RESET_PIN,  HIGH);
  delay(1);
  digitalWrite(MSGEQ7_RESET_PIN,  LOW);
  digitalWrite(MSGEQ7_STROBE_PIN, HIGH);
  delay(1);
  READ_AUDIO();
}

void InitWS2812B() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  //FastLED.setTemperature(Candle);

  for (int i = 0; i < NUM_LEDS; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}




void welcome() {
  //intro animation for box button and leds
  delay(500);
}



void flex_mono() {
  READ_AUDIO();

  left_pos    = HALF_POS;
  left_point  = HALF_POS;
  right_pos   = HALF_POS;
  right_point = HALF_POS;

  MILLISECONDS = 10;
  lowPass_audio = 0.10;
  spectrumWidth = 32;

  //fhue += mono[0] * .05;
  //  EVERY_N_MILLISECONDS(200){
  //    fhue++;
  //  }

  for (int band = 0; band < 7; band++) {

    left_current_brightness = map(left[band], 0, 255, 0, 255);
    right_current_brightness = left_current_brightness;

    if (band < 6) {
      left_next_brightness  = map(left[band + 1], 0, 255, 0, 255);
      right_next_brightness = left_next_brightness;
    } else {
      left_next_brightness  = 0;
      right_next_brightness = 0;
    }

    current_hue = fhue + (band * spectrumWidth);
    next_hue = fhue + ((band + 1) * spectrumWidth);

    left_point -=  mapped_left[band];
    right_point += mapped_left[band];


    //if (band == 6) (left_point = 0) && (right_point = NUM_LEDS - 1) && (next_hue = band * 35) /*&& (left_next_brightness = 0) && (right_next_brightness = 0)*/;

    fill_gradient(leds, left_pos, CHSV(current_hue, 255, left_current_brightness), left_point, CHSV(next_hue, 255, right_next_brightness), SHORTEST_HUES);
    fill_gradient(leds, right_pos, CHSV(current_hue, 255, right_current_brightness), right_point, CHSV(next_hue, 255, right_next_brightness), SHORTEST_HUES);

    //fill_gradient(leds, left_pos, ColorFromPalette(gCurrentPalette, current_hue, left_current_brightness, LINEARBLEND), left_point, ColorFromPalette(gCurrentPalette, next_hue, right_next_brightness, LINEARBLEND), SHORTEST_HUES);
    //fill_gradient(leds, right_pos, ColorFromPalette(gCurrentPalette, current_hue, right_current_brightness, LINEARBLEND), right_point, ColorFromPalette(gCurrentPalette, next_hue, right_next_brightness, LINEARBLEND), SHORTEST_HUES);

    //current_hue += 12;
    //next_hue += 12;

    left_pos  = left_point;
    right_pos = right_point;
  }

  //fadeToBlackBy(leds,NUM_LEDS,1);
  blur1d(leds, NUM_LEDS, 130);
  //nblend(leds, NUM_LEDS, 130);
  FastLED.show();
}



void flex() {
  READ_AUDIO();
  MILLISECONDS  = 0;
  lowPass_audio = 0.07;
  //filter_min    = 150;

  //  left_current_brightness = 0;
  //  right_current_brightness = 0;
  //  left_next_brightness = 0;
  //  right_next_brightness = 0;
  //  current_hue = 0;
  //  next_hue = 0;

  spectrumWidth = 31;
  fhue = 0;

  left_pos    = HALF_POS;
  left_point  = HALF_POS;
  right_pos   = HALF_POS;
  right_point = HALF_POS;

  for (int band = 0; band < 7; band++) {

    left_point   -= mapped_left[band];
    right_point  +=  mapped_right[band];

    if (band == 0) {
      current_hue = fhue;
      next_hue = fhue + spectrumWidth;
      left_current_brightness  = map(left[0], 0, 255, 0, 200);
      right_current_brightness = map(right[0], 0, 255, 0, 200);
      left_next_brightness     = map(left[band + 1], 0, 255, 0, 200);
      right_next_brightness    = map(right[band + 1], 0, 255, 0, 200);
    }

    else if (band < 6) {
      current_hue = next_hue;
      next_hue = fhue + ((band + 1) * spectrumWidth);
      left_current_brightness  = left_next_brightness;
      right_current_brightness = right_next_brightness;
      left_next_brightness     = map(left[band + 1], 0, 255, 0, 200);
      right_next_brightness    = map(right[band + 1], 0, 255, 0, 200);
    }

    else if (band == 6) {
      current_hue = next_hue;
      next_hue = fhue + (7 * spectrumWidth);
      left_current_brightness  = left_next_brightness;
      right_current_brightness = right_next_brightness;
      left_next_brightness     = left_current_brightness;
      right_next_brightness    = right_current_brightness;

      left_point = 0;
      right_point = NUM_LEDS - 1;
    }

    fill_gradient(leds, left_pos,  CHSV(current_hue, 255, left_current_brightness),  left_point,  CHSV(next_hue, 255, left_next_brightness),  SHORTEST_HUES);
    fill_gradient(leds, right_pos, CHSV(current_hue, 255, right_current_brightness), right_point, CHSV(next_hue, 255, right_next_brightness), SHORTEST_HUES);

    left_pos  = left_point;
    right_pos = right_point;

  }



  EVERY_N_MILLISECONDS(6) {
    //blur1d(leds, NUM_LEDS, 1);
    fadeToBlackBy(leds, NUM_LEDS, 1);
  }

  //fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}



void flex_stereo() {
  READ_AUDIO();
  lowPass_audio = 0.1;
  MILLISECONDS = 10;
  filter_min  = 150;
  spectrumWidth = 28;

  pos = 0;
  point = pos;

  spectrumWidth = 36;

  for (int band = 0; band < 14; band++) {

    current_hue = HUEZ[band];
    next_hue    = HUEZ[band + 1];

    point += full_mapped[band];

    if (band < 6) {
      current_brightness = right[6 - band] * 0.25;
      next_brightness = right[5 - band]  * 0.25;
    } else if (band == 6) {
      current_brightness = right[0] * 0.25;
      next_brightness = left[0] * 0.25;
    } else {
      current_brightness = left[band - 6] * 0.25;
      next_brightness = left[band - 5] * 0.25;
    }

    fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), point, CHSV(next_hue, 255, next_brightness),  SHORTEST_HUES);


    pos = point;

  }

  blur1d(leds, NUM_LEDS, 100);

  FastLED.show();
}



void radiate() {
  READ_AUDIO();
  //int SPEED = mono[0] * 0.004;
  //HALF_POS = beatsin8(40, 30 + SPEED, 80 + SPEED);
  MILLISECONDS  = 0;
  //lowPass_audio = 0.10;
  //filter_min    = 100;

  //  EVERY_N_MILLISECONDS(50) {
  //    hue++;
  //    if ( hue > 255) hue = 0;
  //  }


  zero_l  = left[0];
  three_l = left[3];
  six_l   = left[6];

  zero_r  = right[0];
  three_r = right[3];
  six_r   = right[6];

  leds[HALF_POS] = CRGB(zero_l, three_l, six_l);
  leds[HALF_POS + 1] = CRGB(zero_r, three_r, six_r);
  //leds[HALF_POS].fadeToBlackBy(30);


  EVERY_N_MILLISECONDS(11) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].blue = leds[i - 1].blue;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].blue = leds[i + 1].blue;
    }
  }
  EVERY_N_MILLISECONDS(27) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].green = leds[i - 1].green;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].green = leds[i + 1].green;
    }
  }
  EVERY_N_MILLISECONDS(52) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].red = leds[i - 1].red;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].red = leds[i + 1].red;
    }
  }

  EVERY_N_MILLISECONDS(2) {
    //blur1d(leds, NUM_LEDS, 1);
  }
  FastLED.show();
}



void audio_data() {
  READ_AUDIO();
  lowPass_audio = 0.1;
  MILLISECONDS = 10;
  filter_min  = 100;
  spectrumWidth = 4;

  int pos = 0;
  int point = 0;

  for (int band = 0; band < 7; band++) {
    point += 20;
    if (band == 6) point = NUM_LEDS - 1;
    hue = band * spectrumWidth;

    BRIGHTNESS = mono[band];

    for (int i = pos; i < point; i++) {
      leds[i] = CHSV(hue, 255, BRIGHTNESS);
    }
    pos = point;
  }
  FastLED.show();
}

int left_hue;
int right_hue;
int left_brightness;
int right_brightness;

void audio_data_stereo() {
  READ_AUDIO();
  lowPass_audio = 0.12;
  MILLISECONDS = 10;
  //filter_min  = 100;
  spectrumWidth = 33;

  //  EVERY_N_MILLISECONDS(100) {
  //    hue++;
  //  }

  int pos = 0;
  int point = 0;

  for (int band = 0; band < 7; band++) {
    point += 10;

    left_brightness  = map(left[band], 0, 255, 0, 150);
    right_brightness = map(right[band], 0, 255, 0, 150);

    hue = band * spectrumWidth;
    //left_hue = hue + (left_brightness * 0.25);
    //right_hue = hue + (right_brightness * 0.25);

    if (band == 0) {
      left_brightness = map(mono[0], 0, 255, 0, 150);
      right_brightness = left_brightness;
      point = 12;
    }


    for (int i = HALF_POS + pos; i < HALF_POS + point; i++) {
      leds[i] = CHSV(hue, 255, left_brightness);
    }
    for (int i = HALF_POS - pos; i > (HALF_POS - point); i--) {
      leds[i] = CHSV(hue, 255, right_brightness);
    }
    pos = point;
  }
  blur1d(leds, NUM_LEDS, 10);
  FastLED.show();
}

//void audio_data() {
// READ_AUDIO();
///  hue = 0;
//  int INDEX = 0;

//  changePalette();

//  for (int i = 0; i < NUM_LEDS; i++) {                                    // Just ONE loop to fill up the LED array as all of the pixels change.
//    uint8_t index = inoise8(i * scale, dist + scale, z) % 255;            // Get a value from the noise function. I'm using both x and y axis.
//    hue[i] = ColorFromPalette(gCurrentPalette, index, 255, LINEARBLEND); // With that value, look up the 8 bit colour palette value and assign it to the current LED.
//  }
//  dist += beatsin8(10, 1, 4);                                           // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave


 // for (int band = 0; band < 7; band++) {
 //   hue = 252 - (band * 36);
  //  hue = 215;
  //  for (int i = INDEX; i < (INDEX + segmentSize); i++) {
  //    leds[i] = CHSV(hues[i], 255, mono[band] * 0.25);
  //  }
  //  INDEX += segmentSize;
 // }
 // FastLED.show();
//}



void spectrum142() {
  READ_AUDIO();
  pos = 0;
  point = 0;

  MILLISECONDS  = 0;
  lowPass_audio = 0.18;
  //filter_min    = 0;
  spectrumWidth = 34;
  hue = 0;

  //  EVERY_N_MILLISECONDS(20) {
  //    if (mono[0] > 50) hue--;;
  //  }
  //if(mono[0] > 80) hue++;

  // fill pre-segments
  //current_brightness = left[6] * 0.25;
  //current_hue = hue;
  //fill_gradient(leds, pos, CHSV(current_hue, 255, 0), point, CHSV(current_hue, 255, current_brightness), SHORTEST_HUES);
  //pos = point;

  for (int band = 0; band < 7; band++) {
    //k = (band + 1) % 7; // debug...
    point += segmentEnd[band];
    current_brightness = map(mono[band], 10, 255, 10, 255);
    current_hue = hue + (band * spectrumWidth); // COLOR FROM PALETTE

    if (band < 6) {
      next_brightness = map(mono[band + 1], 10, 255, 10, 255); // CAN JUST LOWER THIS TO CREATE EDGES INSTEAD OF /*next_brightness * 0.6*/
      next_hue = hue + ((band + 1) * spectrumWidth);
      fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), point, CHSV(next_hue, 255, next_brightness), SHORTEST_HUES);

    } else {
      next_brightness = current_brightness; // help this out to make it go further to the egde! more color!
      next_hue = current_hue;
      fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), NUM_LEDS - 1, CHSV(next_hue, 255, next_brightness), SHORTEST_HUES);
    }
    pos = point;
  }

  //EVERY_N_MILLISECONDS(5) {
  //fadeToBlackBy(leds, NUM_LEDS, 1);
  blur1d(leds, NUM_LEDS, 160);
  //nscale8(leds,NUM_LEDS,70);

  //}
  FastLED.show();
}

float Hue = 0;
int state = 0;

void rainbow() {

  hue = 0;
  EVERY_N_MILLISECONDS(50) {
    ihue--;
    //if (ihue > 255) ihue = 0;
  }

  fill_rainbow(leds, NUM_LEDS, hue, 1);

  for (int i = 0; i < NUM_LEDS; i++) {
    hue = hue + 3;
    if (hue > 255) hue = 0;
    leds[i] = CHSV(hue + ihue, saturation, 255);
  }

  // if(state == 0){
  //   saturation -= 7;
  //   if(saturation < 10) state = 1;
  // }
  //
  // if(state == 1){
  //    saturation += 7;
  //    if(saturation > 245) state = 0;
  // }

  //addGlitter(80);
  FastLED.show();
}



void ambient() {
  //READ_AUDIO();
  changePalette();
  scale = 2;

  for (int i = 0; i < NUM_LEDS; i++) {                                     // Just ONE loop to fill up the LED array as all of the pixels change.
    uint8_t index = inoise8(i * scale, dist + scale, z) % 255;             // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(gCurrentPalette, index, 255, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  //dist += beatsin8(10, 1, 4);                                             // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave
  //dist += mono[4] * .03;
  z += .5 + (mono[0] * .03);

  FastLED.show();
}

void audio_ambient() {
  changePalette();
  scale = 13;

  for (int i = 0; i < NUM_LEDS; i++) {                                     // Just ONE loop to fill up the LED array as all of the pixels change.
    uint16_t index = inoise8(i * scale, dist + scale, z) % 255;             // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(gCurrentPalette, index, 255, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }
  //dist += beatsin8(30, 1, 2);                                            // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave
  dist += mono[0] * .03;

  //EVERY_N_MILLISECONDS(8) {
  //z++;
  //}
  blur1d(leds, NUM_LEDS, 60);
  //FastLED.show();
}


void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < HALF_POS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }
  for ( int i = HALF_POS; i < NUM_LEDS - 1; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= (HALF_POS); k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  for ( int k = 0; k <= (HALF_POS); k++) {
    heat[k] = (heat[k + 1] + heat[k + 2] + heat[k + 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int l = random8(7);
    int r = random8(7);
    heat[HALF_POS + r] = qadd8( heat[HALF_POS + r], random8(160, 255) );
    heat[HALF_POS - l] = qadd8( heat[HALF_POS - l], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }


  //FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);

}


void spectrumcolor() {

  pos = 0;
  point = 0;

  MILLISECONDS  = 0;
  lowPass_audio = 0.12;
  //filter_min    = 0;
  spectrumWidth = 0;
  hue = 200;

  //  EVERY_N_MILLISECONDS(20) {
  //    if (mono[0] > 50) hue--;;
  //  }
  //if(mono[0] > 80) hue++;

  // fill pre-segments
  //current_brightness = left[6] * 0.25;
  //current_hue = hue;
  //fill_gradient(leds, pos, CHSV(current_hue, 255, 0), point, CHSV(current_hue, 255, current_brightness), SHORTEST_HUES);
  //pos = point;

  for (int band = 0; band < 7; band++) {
    //k = (band + 1) % 7; // debug...
    point += segmentEnd[band];
    current_brightness = map(mono[band], 50, 255, 50, 255);
    current_hue = hue + ((band * spectrumWidth) /*+ (mono[band] * .2)*/); // COLOR FROM PALETTE

    if (band < 6) {
      next_brightness = map(mono[band + 1], 50, 255, 50, 255); // CAN JUST LOWER THIS TO CREATE EDGES INSTEAD OF /*next_brightness * 0.6*/
      next_hue = hue + (((band + 1) * spectrumWidth) /*+ (mono[band] * .2)*/);
      fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), point, CHSV(next_hue, 255, next_brightness), SHORTEST_HUES);

    } else {
      next_brightness = current_brightness; // help this out to make it go further to the egde! more color!
      //next_hue = hue;
      fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), NUM_LEDS - 1, CHSV(next_hue, 255, next_brightness), SHORTEST_HUES);
    }
    pos = point;
  }

  //blur1d(leds, NUM_LEDS, 30);
  //nscale8(leds,NUM_LEDS,50);
  //FastLED.show();
}


void sat_spectrum() {

  pos = 0;
  point = 0;

  MILLISECONDS  = 10;
  lowPass_audio = 0.1;
  //filter_min    = 0;
  spectrumWidth = 34;
  hue = 0;

  //  EVERY_N_MILLISECONDS(20) {
  //    if (mono[0] > 50) hue--;;
  //  }
  //if(mono[0] > 80) hue++;

  for (int band = 0; band < 7; band++) {

    point += segmentEnd[band];
    current_brightness = map(mono[band], 0, 255, 255, 0);
    current_hue = hue + (band * spectrumWidth); // COLOR FROM PALETTE

    if (band < 6) {
      next_brightness = map(mono[band + 1], 0, 255, 255, 0); // CAN JUST LOWER THIS TO CREATE EDGES INSTEAD OF /*next_brightness * 0.6*/
      next_hue = hue + ((band + 1) * spectrumWidth);
      fill_gradient(leds, pos, CHSV(hue, current_brightness, 255), point, CHSV(hue, next_brightness, 255), SHORTEST_HUES);

    } else {
      next_brightness = current_brightness; // help this out to make it go further to the egde! more color!
      next_hue = current_hue;
      fill_gradient(leds, pos, CHSV(hue, current_brightness, 255), NUM_LEDS - 1, CHSV(hue, next_brightness, 255), SHORTEST_HUES);
    }
    pos = point;
  }

  //EVERY_N_MILLISECONDS(5) {
  //fadeToBlackBy(leds, NUM_LEDS, 1);
  //blur1d(leds, NUM_LEDS, 60);
  //nscale8(leds,NUM_LEDS,70);

  //}
  FastLED.show();
}


void strip_flex() {
  READ_AUDIO();
  pos = 0;
  point = 0;

  MILLISECONDS = 10;
  lowPass_audio = 0.08;
  spectrumWidth = 32;

  //fhue += mono[0] * .05;
  //  EVERY_N_MILLISECONDS(200){
  //    fhue++;
  //  }

  for (int band = 0; band < 7; band++) {

    current_brightness = map(mono[band], 50, 255, 50, 200);

    if (band < 6) {
      next_brightness  = map(mono[band + 1], 50, 255, 50, 200);
    } else {
      next_brightness  = current_brightness;
    }

    current_hue = fhue + (band * spectrumWidth);
    next_hue = fhue + ((band + 1) * spectrumWidth);

    point +=  full_flex[band];

    fill_gradient(leds, pos, CHSV(current_hue, 255, current_brightness), point, CHSV(next_hue, 255, next_brightness), SHORTEST_HUES);

    pos = point;
  }

  fadeToBlackBy(leds, NUM_LEDS, 1);
  //blur1d(leds, NUM_LEDS, 70);
  FastLED.show();
}



void READ_AUDIO() {

  prev_left  = left_volume;
  prev_right = right_volume;

  prev_left_amp  = half_MAPPED_LEFT_AMP;
  prev_right_amp = half_MAPPED_RIGHT_AMP;

  left_volume  = 0.0;
  right_volume = 0.0;
  mono_volume  = 0;
  left_factor  = 0.0;
  right_factor = 0.0;
  mono_factor  = 0;

  digitalWrite(MSGEQ7_RESET_PIN, HIGH);
  digitalWrite(MSGEQ7_RESET_PIN, LOW);
  delayMicroseconds(1);

  for (int band = 0; band < 7; band++)
  {

    filter_min = FILTER_MIN[band];
    digitalWrite(MSGEQ7_STROBE_PIN, LOW);
    delayMicroseconds(36);

    prev_value  = left[band];
    left_value  = analogRead(AUDIO_LEFT_PIN);
    //left_value  -= (left_value * EQ[band]);
    left_value  = constrain(left_value, filter_min, filter_max);
    left_value  = map(left_value, filter_min, filter_max, 0, 255);
    left[band]  = prev_value + (left_value - prev_value) * lowPass_audio;
    left_volume += left[band];


    prev_value   = right[band];
    right_value  = analogRead(AUDIO_RIGHT_PIN);
    //right_value  -= (right_value * EQ[band]);
    right_value  = constrain(right_value, filter_min, filter_max);
    right_value  = map(right_value, filter_min, filter_max, 0, 255);
    right[band]  = prev_value + (right_value - prev_value) * lowPass_audio;
    right_volume += right[band];

    digitalWrite(MSGEQ7_STROBE_PIN, HIGH);
    delayMicroseconds(1);

    mono[band]   = (left[band] + right[band]) * 0.5;
    mono_volume += mono[band];

    //IF DEF is (this effect) then do these extra/specfic tasks *stereo_vu*
  }

  //mono_volume  /= 7;
  //left_volume  /= 7;
  //right_volume /= 7;
  //  new_left = (left_volume  / 7);
  //  new_left = prev_left + (new_left - prev_left) * 0.005;
  //  new_right = (right_volume / 7);
  //  new_right = prev_right + (new_right - prev_right) * 0.005;
  //
  //  left_filter_max  = new_left;
  //  right_filter_max = new_right;


  left_factor   = float(HALF_POS) / left_volume;
  right_factor  = float(HALF_POS) / right_volume;
  mono_factor   = float(HALF_POS) / mono_volume;
  //if(mono_factor < 0.01) mono_factor = 0;
  full_factor   = float(NUM_LEDS) / mono_volume;

  for (int band = 0; band < 7; band++)
  {

    full_flex[band] = mono[band] * full_factor;

    mapped[band]    = mono[band] * mono_factor;

    //    if (half_MAPPED_AMP < 2) {
    //      half_MAPPED_AMP -= 2;
    //    }
    //    mapped[band] = half_MAPPED_AMP;

    half_MAPPED_LEFT_AMP = floor(left[band]  * left_factor);
    //    if (half_MAPPED_LEFT_AMP < 3) {
    //      half_MAPPED_LEFT_AMP -= 3;
    //    }
    mapped_left[band]    = half_MAPPED_LEFT_AMP;

    half_MAPPED_RIGHT_AMP = floor(right[band] * right_factor);
    //    if (half_MAPPED_RIGHT_AMP < 2) {
    //      half_MAPPED_RIGHT_AMP -= 2;
    //    }
    mapped_right[band]    = half_MAPPED_RIGHT_AMP;


    //full_mapped[band]     = left[6 - band]  * left_factor;
    //full_mapped[band + 7] = right[band] * right_factor;

    // }

  }
}
void print_audio() {
  READ_AUDIO();
  for (int band = 0; band < 7; band++) {
    Serial.print(mono[band]);
    Serial.print("\t");
  }
  Serial.println();
}

void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
  FastLED.show();
}


void setup() {
  delay(500);
  btSerial.begin(9600);    // SETUP SOFTWARE SERIAL (USB)
  btSerial.setTimeout(SERIAL_TIMEOUT);
  //---PROCESS HARDWARE SERIAL COMMANDS AND ARGS
  InitWS2812B();
  InitMSGEQ7();
  welcome();
  Serial.begin(9600);
}

void loop() {

  while (Serial.available() > 0) {
    inbyte = Serial.read();
    switch (inbyte) {
      case 59: break; //---BREAK IF INBYTE = ';'

      case 109:      //---"m" - SET MODE
        thisarg = Serial.parseInt();
        change_mode(thisarg);
        break;
    }
  }

  //---PROCESS SOFTWARE SERIAL COMMANDS AND ARGS
  while (btSerial.available() > 0) {
    inbyte = btSerial.read();
    if (btSerial.overflow()) {
      Serial.println("SoftwareSerial overflow!");
    }
    switch (inbyte) {
      case 109:      //---"m" - SET MODE
        thisarg = btSerial.parseInt();
        change_mode(thisarg);
        break;
    }

  }
  switch (ledMode) {
    case 0:
      flex_mono();
      break;
    case 1:
      radiate();
      break;
    case 2:
      audio_data();
      break;
    case 3:
      spectrum142();
      break;
    case 4:
      rainbow();
      break;
    case 5:
      flex();
      break;
    case 6:
      ambient();
      break;
    case 7:
      Fire2012();
      break;
    case 8:
      audio_data_stereo();
      break;
    case 9:
      spectrumcolor();
      break;
    case 10:
      sat_spectrum();
      break;
    case 11:
      strip_flex();
      break;
    case 12:
      print_audio();
      break;
    case 13:
      changePalette();
      map_palette(gCurrentPalette);
      break;
    case 14:
      pride();
      break;
    case 15:
      flex_stereo();
      break;
    case 16:
      strip_flex();
      break;
  }




}
