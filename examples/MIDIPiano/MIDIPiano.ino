/*
 * Displays a 12-key piano keyboard and uses it to play MIDI sound effects
 * For a full list of instruments, see the VS1053 datasheet page #33:
 * https://www.sparkfun.com/datasheets/Components/SMD/vs1053.pdf
 *
 * The MIDI instruments can be switched between using a few button widgets
 */
#include "Phoenard.h"

// Automatically pre-calculated constants for displaying the 12-key keyboard
#define KEYS_COUNT         12
#define KEYS_WHITE_WIDTH   42
#define KEYS_WHITE_HEIGHT  190
#define KEYS_BLACK_WIDTH   (KEYS_WHITE_WIDTH*0.52)
#define KEYS_BLACK_HEIGHT  (KEYS_WHITE_HEIGHT*0.6)
#define KEYS_WHITE_STEP    (KEYS_WHITE_WIDTH + 2)
#define KEYS_WHITE_GAP     (KEYS_WHITE_HEIGHT - KEYS_BLACK_HEIGHT - 1)
#define KEYS_WHITE_OFFX    ((320-7*KEYS_WHITE_STEP)/2)
#define KEYS_WHITE_OFFY    (240-KEYS_WHITE_HEIGHT-KEYS_WHITE_OFFX)
#define KEYS_BLACK_OFFX    (KEYS_WHITE_OFFX+KEYS_WHITE_WIDTH-KEYS_BLACK_WIDTH/2+1)

// Define the Midi control object
PHN_Midi midi;

// Track the keys pressed down
boolean keys_pressed[KEYS_COUNT];

// Define midi constants
char midi_channel    = 0;
char midi_velocity   = 120;
int  midi_instrument_idx;

// Store a set of instruments here
// Others are only accessible by index
typedef struct {
  int id;
  char name[20];
} InstrumentInfo;

// Instrument presets for quick switching
InstrumentInfo instruments[] = {
  {1, "Acoustic Piano"},
  {5, "Electric Piano"},
  {7, "Harpsichord"},
  {8, "Clavi"},
  {10, "Glockenspiel"},
  {13, "Marimba"},
  {14, "Xylophone"},
  {18, "Organ"},
  {20, "Church Organ"},
  {23, "Harmonica"},
  {41, "Violin"},
  {47, "Harp"},
  {49, "String Ensembles"},
  {51, "Synth Strings"},
  {54, "Voice Oohs"},
  {63, "Synth Brass"},
  {74, "Flute"},
  {96, "Sweep"},
  {98, "Sound Track"},
  {107, "Shamisen"},
  {115, "Percussion"},
  {117, "Taiko Drum"},
  {125, "Telephone"},
  {128, "Gunshot"}
};

// Define some widgets for altering the instrument
PHN_Button instrument_next;
PHN_Button instrument_prev;
PHN_Label instrument_name;

void setup() {
  // Initialize the MIDI
  midi.begin();

  // Fill the background with a gray color so black keys show properly
  display.fill(GRAY_LIGHT);

  // Set up the instrument adjusting widgets
  instrument_prev.setBounds(5, 5, 32, 32);
  instrument_prev.setText("<");
  display.addWidget(instrument_prev);

  instrument_next.setBounds(320-37, 5, 32, 32);
  instrument_next.setText(">");
  display.addWidget(instrument_next);

  instrument_name.setBounds(45, 5, 320-90, 32);
  instrument_name.setColor(CONTENT, GREEN);
  instrument_name.setDrawFrame(true);
  display.addWidget(instrument_name);

  // Setup initial instrument
  setInstrument(0);

  // Update all keys at least once, forcing them to draw
  for (int i = 0; i < KEYS_COUNT; i++) {
    keys_pressed[i] = updateKey(i, false, true);
  }
}

void loop() {
  // Update the touch input and other display functions
  display.update();

  // Update instrument as requested
  if (instrument_prev.isClicked()) {
    setInstrument(midi_instrument_idx-1);
  }
  if (instrument_next.isClicked()) {
    setInstrument(midi_instrument_idx+1);
  }

  // Refresh the press-down states of the keys
  for (int i = 0; i < KEYS_COUNT; i++) {
    boolean pressed_new = updateKey(i, keys_pressed[i], false);
    if (keys_pressed[i] != pressed_new) {
      keys_pressed[i] = pressed_new;
      playKey(i, keys_pressed[i]);
    }
  }
}

/* Updates instrument information */
void setInstrument(int index) {
  // Update the index, looping around
  const int instruments_cnt = sizeof(instruments) / sizeof(InstrumentInfo);
  // There are 128 instruments.  The index of the first one is instruments_cnt + 0,
  //  so the index of the last one is instruments_cnt + 127.
  const int IDX_MAX = instruments_cnt + 127;
  if (index < 0) {
    index = IDX_MAX;
  } else if (index > IDX_MAX) {
    index = 0;
  }
  midi_instrument_idx = index;
  
  // Calculate the instrument ID and name
  // Using instrument numbers 1 to 128.
  //  With id of type char (8 bits), instrument 128 is displayed as -128.
  //  Need id to be type int so instrument 128 is displayed as 128.
  int id;
  String fullName;
  if (index >= instruments_cnt) {
    id = index-instruments_cnt+1;
    fullName += "Instrument #";
    fullName += id;
  } else {
    id = instruments[index].id;
    fullName += instruments[index].name;
    fullName += " (#";
    fullName += id;
    fullName += ")";
  }

  // Update information in MIDI/label widget
  instrument_name.setText(fullName);
  // Using instrument numbers 1 to 128 in this sketch,
  //  but MIDI instruments are numbered 0 to 127, so subtract 1 when setting the instrument
  midi.setInstrument(id-1);
}

/* Tells the MIDI component to press or release a key */
void playKey(int key, boolean down) {
  char note = 60 + key; // Starting at note C4
  midi.note(midi_channel, note, midi_velocity, down);
}

/*
 * Checks if a key is pressed down, handling the re-drawing automatically
 * Specify the previous press-down state for accurate redrawing
 */
boolean updateKey(int key, boolean was_pressed, boolean forceRedraw) {
  // Key algorithm - separate white from black keys
  // This is needed to calculate the x-offset
  // Total of 7 white keys and 5 black keys
  // Black IDX 2 is skipped - dummy key to allow proper x calculation
  //
  // index | 00  01  02  03  04  05  06  07  08  09  10  11
  // color | W   B   W   B   W   W   B   W   B   W   B   W
  // widx  | 0       1       2   3       4       5       6
  // bidx  |     0       1           3       4       5

  int k = (key % 12);
  const char w_indices[] = { 0, -1,  1, -1,  2,  3, -1,  4, -1,  5, -1,  6};
  const char b_indices[] = {-1,  0, -1,  1, -1, -1,  3, -1,  4, -1,  5, -1};
  boolean pressed = false;
  
  // WHITE KEYS
  if (w_indices[k] != -1) {
    int w_idx = (int) w_indices[k] + (key / 12) * 7;
    const char gap_left[]  = {0, 1, 1, 0, 1, 1, 1};
    const char gap_right[] = {1, 1, 0, 1, 1, 1, 0};

    // Programatically calculate key shape parameters
    int x = KEYS_WHITE_OFFX + (w_idx * KEYS_WHITE_STEP);
    int y = KEYS_WHITE_OFFY;
    int y_mid = y+KEYS_WHITE_HEIGHT-KEYS_WHITE_GAP;
    int l_a = x;
    int l_b = x+KEYS_WHITE_WIDTH;
    if (gap_left[w_idx])  l_a += KEYS_BLACK_WIDTH / 2;
    if (gap_right[w_idx]) l_b -= KEYS_BLACK_WIDTH / 2;

    // See if area is pressed down
    if (display.isTouched(x, y_mid, KEYS_WHITE_WIDTH, KEYS_WHITE_GAP)) {
      pressed = true;
    }
    if (display.isTouched(l_a, y, l_b-l_a, y_mid-y)) {
      pressed = true;
    }
  
    // Only draw if state changed
    if (forceRedraw || pressed != was_pressed) {
      color_t color = pressed ? YELLOW : WHITE;

      // Draw the rectangle at the bottom
      display.drawVerticalLine(x, y_mid, KEYS_WHITE_GAP, BLACK);
      display.drawVerticalLine(x+KEYS_WHITE_WIDTH-1, y_mid, KEYS_WHITE_GAP, BLACK);
      display.drawHorizontalLine(x, y_mid+KEYS_WHITE_GAP-2, KEYS_WHITE_WIDTH, BLACK);
      display.fillRect(x+1, y_mid+1, KEYS_WHITE_WIDTH-2, KEYS_WHITE_GAP-2, color);
  
      // Draw the middle part with connecting lines
      display.drawHorizontalLine(l_a, y, l_b-l_a, BLACK);
      display.drawVerticalLine(l_a, y, y_mid-y, BLACK);
      display.drawVerticalLine(l_b-1, y, y_mid-y, BLACK);
      display.drawHorizontalLine(x, y_mid, l_a-x+1, BLACK);
      display.drawHorizontalLine(l_b-1, y_mid, (x+KEYS_WHITE_WIDTH)-l_b+1, BLACK);
      //display.drawRect(l_a, y, l_b - l_a, y_mid-y, BLACK);
      display.fillRect(l_a+1, y+1, l_b - l_a - 2, y_mid-y, color);
    }
  }
  
  // BLACK KEYS
  if (b_indices[k] != -1) {
    int b_idx = (int) b_indices[k] + (key / 12) * 6;

    // Programatically calculate the top-left location of the key
    int x = KEYS_BLACK_OFFX + (b_idx * KEYS_WHITE_STEP);
    int y = KEYS_WHITE_OFFY;
    
    // See if pressed down here
    pressed = display.isTouched(x, y, KEYS_BLACK_WIDTH, KEYS_BLACK_HEIGHT);
    
    // Only draw if state changed
    if (forceRedraw || pressed != was_pressed) {
      color_t color = pressed ? YELLOW : GRAY_DARK;
      display.drawRect(x, y, KEYS_BLACK_WIDTH, KEYS_BLACK_HEIGHT, BLACK);
      display.fillRect(x+1, y+1, KEYS_BLACK_WIDTH-2, KEYS_BLACK_HEIGHT-2, color);
    }
  }
  return pressed;
}
