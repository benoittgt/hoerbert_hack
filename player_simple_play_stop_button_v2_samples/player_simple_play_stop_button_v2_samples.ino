/*************************************************** 
  Written by Benoit Tigeot, thanks Adafruit
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

// Pin to turn off polulu
#define OFF_PIN 2

// Analog pins for buttons to select 1 track
static const uint8_t analog_button_pins[] = {A1,A2,A3,A4,A5};
int analog_button_pins_count = 5;

// Previous next button pin
#define NEXT_SONG_PIN 0
#define PREVIOUS_SONG_PIN 8

// Analog pin for Volume
#define VOLUME_POT_PIN A0

static const int delay_between_input_check = 200; //ms

const int inactivity_timeout = 4000; //240000 ms = 4minutes
unsigned long lastAction;

uint8_t outputValue = 50;
bool playing = false;

static const int number_of_tracks = 14;
int last_track_played = 1;

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

void play(int number, String type) {
  if (type == "track") { last_track_played = number; }
  type += (number / 100) % 10;
  type += (number / 10) % 10;
  type += number % 10;
  type += ".mp3";
  Serial.print("Starting to play ");
  Serial.println(type);
  musicPlayer.stopPlaying();
  musicPlayer.startPlayingFile(type.c_str());

  playing = true;
  lastAction = millis();
}


void setup() {
  Serial.begin(9600);
  Serial.println("Boot");

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
   if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // Setup analog input
  for (int i = 0; i < analog_button_pins_count; i++) {
    pinMode(analog_button_pins[i], INPUT_PULLUP);
  }

  // Setup previous-next buttons
   pinMode(NEXT_SONG_PIN, INPUT_PULLUP);
   pinMode(PREVIOUS_SONG_PIN, INPUT_PULLUP);

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(outputValue,outputValue);

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Track inactivity even if no sound play
  lastAction = millis();
  
  // Set randomSeed for random boot sound
  randomSeed(analogRead(0));
  // play random boot sound form file named sfx00?.mp3
  play(random(1,10), "sfx");
}

void set_volume() {
  int sensorValue = analogRead(VOLUME_POT_PIN);
  // map it to the range of the analog out:
  outputValue = map(sensorValue, 0, 1023, 0, 100);

  // change the analog out value:
  musicPlayer.setVolume(outputValue,outputValue);
}

int next_song_id(int id) {
  if (last_track_played == number_of_tracks) {
    last_track_played = 1;
  } else {
    last_track_played += 1;
  }
  return last_track_played;
}

int previous_song_id(int id) {
  if (last_track_played == 1) {
    last_track_played = number_of_tracks;
  } else {
    last_track_played -= 1;
  }
  return last_track_played;
}

void loop() {
  set_volume();

  // GPIO input 0 to 6 inclusive for 1 to 7 tracks
  for (int i = 0; i < 8; i++) {
    if(musicPlayer.GPIO_digitalRead(i) == 1){
      play(i, "track");
    }
  }

  // analog input 1 to 5 inclusive for 8 to 12 tracks
  for (int i = 0; i < analog_button_pins_count; i++) {
    if (digitalRead(analog_button_pins[i]) == LOW) {
      Serial.print("Analog input ");
      play(i + 8, "track");
    }
  }

  // Next track
  if (digitalRead(NEXT_SONG_PIN) == LOW) {
    Serial.println("Next track.");
    play(next_song_id(last_track_played), "track");
    delay(300); //ms. avoid skipping tracks too fast
  }

  // Previous track
  if (digitalRead(PREVIOUS_SONG_PIN) == LOW) {
    Serial.println("Previous track.");
    play(previous_song_id(last_track_played), "track");
    delay(300); //ms. avoid skipping tracks too fast
  }

  if (playing) {
    lastAction = millis();
    if (musicPlayer.stopped()) {
      Serial.println("Song ended.");
      playing = false;  
    }
  }

  unsigned long sinceLast = millis() - lastAction;
  if (sinceLast < 0) lastAction = millis();
  else if (sinceLast > inactivity_timeout) {
    Serial.println("Turing off..");
    digitalWrite(OFF_PIN, HIGH);
    lastAction = millis();
  }
  delay(delay_between_input_check);
}
