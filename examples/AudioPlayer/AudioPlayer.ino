#include "Phoenard.h"
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

// Define the file path on the Micro-SD to play
char filePath[] = "Sounds/beep.mp3";

// Define a button used to initiate the play
PHN_Button playButton;

// Define a scrollbar/slider to change the volume
PHN_Scrollbar volumeSlider;

// Define the Adafruit VS1053 music player
Adafruit_VS1053_FilePlayer musicPlayer(VS1053_PIN_RESET, VS1053_PIN_CS, VS1053_PIN_DCS, VS1053_PIN_DREQ, VS1053_PIN_CARDCS);

void setup() {
  // Setup the play button widget
  playButton.setBounds(50, 50, 150, 140);
  playButton.setText("Play!");
  display.addWidget(playButton);
  
  // Setup the volume slider widget
  volumeSlider.setBounds(246, 40, 30, 240-60);
  volumeSlider.setRange(0, 64);
  volumeSlider.setValue(50);
  display.addWidget(volumeSlider);
  
  // Print a label for volume
  display.debugPrint(230, 10, 2, "Volume");

  // Initialize the VS1053 digital pins
  pinMode(VS1053_PIN_GPIO, OUTPUT);
  pinMode(VS1053_PIN_PWR, OUTPUT);
  pinMode(VS1053_PIN_DCS, OUTPUT);
  digitalWrite(VS1053_PIN_PWR, HIGH);
  digitalWrite(VS1053_PIN_GPIO, LOW);
  digitalWrite(VS1053_PIN_DCS, LOW);

  // Initialize the VS1053 library
  musicPlayer.begin();

  // Make sure to use the pin interrupt
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  // Initialize the SD library for file playback
  // [!] This has to be done AFTER the initialization of the VS1053 [!]
  if (!SD.begin(VS1053_PIN_CARDCS)) {
    Serial.println("Failed to initialize SD card");
  }
}

void loop() {
  display.update();
  
  // Play the sound when clicked
  if (playButton.isClicked()) {
    // If already playing, stop it first with a delay to settle
    if (musicPlayer.playingMusic) {
      musicPlayer.stopPlaying();
      delay(20);
    }
    // Play the sound (async)
    musicPlayer.startPlayingFile(filePath);
    //musicPlayer.playFullFile(filePath);
  }
  
  // Update volume when changed
  if (volumeSlider.isValueChanged()) {
    uint8_t volume = (uint8_t) volumeSlider.value();
    musicPlayer.setVolume(volume, volume);
  }
}