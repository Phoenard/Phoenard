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
Adafruit_VS1053_FilePlayer musicPlayer(VS1053_RESET_PIN, VS1053_CS_PIN, VS1053_DCS_PIN, VS1053_DREQ_PIN, VS1053_CARDCS_PIN);

void setup() {
  // Initialize Serial for debugging
  Serial.begin(9600);

  // Setup the play button widget
  playButton.setBounds(50, 50, 150, 140);
  playButton.setText("Play!");
  display.addWidget(playButton);

  // Setup the volume slider widget
  // Use volume 0 as max, because volume 0 is loudest
  volumeSlider.setBounds(246, 40, 30, 240-60);
  volumeSlider.setRange(64, 0);
  volumeSlider.setValue(50);
  display.addWidget(volumeSlider);
  
  // Print a label for volume
  display.debugPrint(230, 10, 2, "Volume");

  // Initialize the VS1053 digital pins
  pinMode(VS1053_GPIO_PIN, OUTPUT);
  pinMode(VS1053_PWR_PIN, OUTPUT);
  pinMode(VS1053_DCS_PIN, OUTPUT);
  digitalWrite(VS1053_PWR_PIN, HIGH);
  digitalWrite(VS1053_GPIO_PIN, LOW);
  digitalWrite(VS1053_DCS_PIN, LOW);

  // Initialize the VS1053 library
  musicPlayer.begin();

  // Make sure to use the pin interrupt
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  // Initialize the SD library for file playback
  // [!] This has to be done AFTER the initialization of the VS1053 [!]
  if (!SD.begin(VS1053_CARDCS_PIN)) {
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