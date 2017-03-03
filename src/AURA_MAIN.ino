/*

   Written by Drew André using Daniel Garcia and Mark Kriegsman's FastLED library.
   Eagle files..

 */


#include <Arduino.h>
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
//#include <MSGEQ7.h> // to be built

bool DEBUG = true;

// WS2812B SETUP
const uint8_t DATA_PIN = 7;
const uint8_t NUM_LEDS = 28;
//int HALF_POS = (NUM_LEDS * 0.5) - 1;
const uint8_t HALF_POS = 14;
CRGB leds[NUM_LEDS];
const uint8_t FRAMES_PER_SECOND = 300;

// RGB ENCODER SETUP
const uint8_t REDPIN = 3, GREENPIN = 5, BLUEPIN = 6;
ClickEncoder *encoder;
int16_t last, value;
int16_t encoderStatus = 1;
void timerIsr() {
        encoder->service();
}

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

int MILLISECONDS = 0;

void setup() {
        delay(500);
        INITIALIZE();
        welcome();
}

void loop() {

        // EVERY_N_MILLISECONDS(MILLISECONDS) {
        //         //READ_AUDIO();
        // }
        MODE();


}
