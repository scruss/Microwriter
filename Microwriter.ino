// Microwriter reboot on Arduino (c)2016 vik@diamondage.co.nz
// Released under the GPLv3 licence http://www.gnu.org/licenses/gpl-3.0.en.html
// Uses key tables loosely borrowed from the Microwriter https://en.wikipedia.org/wiki/Microwriter
// Requires a Leonardo or Micro (ATmega32u4 CPU) to emulate keyboard and mouse.

#include <Keyboard.h>
#include <Mouse.h>

#define NUM_KEYS  6
#define MOUSE_DELAY_MAX  30;

#define KEYS_SHIFT_ON  1000
#define KEYS_SHIFT_OFF  1001
#define KEYS_NUMERIC_SHIFT 1002
#define KEYS_CONTROL_SHIFT  1003
#define KEYS_ALT_SHIFT  1004
#define KEYS_EXTRA_SHIFT  1005
#define KEYS_MOUSE_MODE_ON  1006
#define KEYS_FUNC_SHIFT  1007

#undef DEBUG

// Thumb, index, middle, ring, pinkie, control pins - rewired for Arduino Micro (scruss)
const int keyPorts[] = {8, 3, 4, 5, 6, 7};

const char alphaTable[] = " eiocadsktrny.fuhvlqz-'gj,wbxmp";
const char numericTable[] = " 120(*3$/+;\"?.46-&#)%!@7=,:8x95";
const char extraTable[] = {0, KEY_ESC, 0, 0, 0, '[', KEY_DELETE, 0,
                           KEY_HOME, 0, 0, 0, 0, 0, KEY_END, 0,
                           0, '\\', 0, ']', KEY_PAGE_DOWN, 0, 0, 0,
                           KEY_PAGE_UP, 0, 0, 0, 0, 0, 0, 0
                          };
const char funcTable[] = {KEY_F1, 0, KEY_F2, KEY_F10, 0, KEY_F11, KEY_F3, 0,
                          0, 0, 0, 0, 0, KEY_F12, KEY_F4, KEY_F6,
                          0, 0, 0, 0, 0, 0, 0, KEY_F7,
                          0, 0, 0, KEY_F8, 0, KEY_F9, KEY_F5
                         };
const int shiftTable[] = {KEYS_SHIFT_ON, KEYS_SHIFT_OFF, KEY_INSERT, KEYS_MOUSE_MODE_ON, KEY_RETURN, 0, KEY_BACKSPACE, 0,
                          KEY_LEFT_ARROW, 0, KEY_TAB, 0, KEYS_NUMERIC_SHIFT, 0, KEY_RIGHT_ARROW, 0,
                          KEYS_EXTRA_SHIFT, 0, KEYS_FUNC_SHIFT, 0, KEY_DOWN_ARROW, 0, 0, 0,
                          KEY_UP_ARROW, 0, 0, 0, KEYS_CONTROL_SHIFT, 0, KEYS_ALT_SHIFT, 0
                         };

// >0 when shift is on.
char shifted = 0;
// >0 when using numeric table
char numericed = 0;
// >0 when CTRL is on
char controlled = 0;
// >0 when ALT is on
char alted = 0;
// >0 When extra shift on
char extraed = 0;
// >0 when using function key table
char funced = 0;

void setup() {
  int i = 0;
#ifdef DEBUG
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    delay(200);
    // wait for serial port to connect. Needed for Leonardo/Micro only
    if (i++ > 12) break;
  }
#endif /* DEBUG */

  for (i = 0; i < NUM_KEYS; i++)
    pinMode(keyPorts[i], INPUT_PULLUP);
#ifdef DEBUG
  while (digitalRead(keyPorts[0]) == LOW)
    Serial.println("Testing...");
#endif /* DEBUG */
  // Prevent keyboard lockout.
  delay(5000);
  Keyboard.begin();
}

// Return the raw keybits
int keyBits() {
  int i, k = 0;

  for (i = 0; i < NUM_KEYS; i++) {
    k *= 2;
    if (digitalRead(keyPorts[i]) == LOW)
      k++;
  }
  return (k);
}

int keyWait() {
  int k, x;

  x = 0;
  k = 0;
  while (-1) {
    // Debounce
    while (k != keyBits()) {
      delay(10);
      k = keyBits();
    }
    x |= k;
    if ((x != 0) && (k == 0)) break;
  }
  return (x);
}

void everythingOff() {
  Keyboard.releaseAll();
  shifted = 0;
  numericed = 0;
  controlled = 0;
  alted = 0;
  extraed = 0;
  funced = 0;
}

/* Uses finger keys as LUDR, thumb as click 1 & 2.
   All 4 fingers exit mouse mode. */
void mouseMode() {
  int k = 0;
  int x, y;
  int mouseDelay = MOUSE_DELAY_MAX;

  Mouse.begin();
  while (true) {
    k = keyBits();
    if (k == 30) break; // Quit mousing if all 4 move keys hit.
    if (k == 0) mouseDelay = MOUSE_DELAY_MAX;
    if ((k & 2) != 0) {
      // Mouse left
      x = -1;
    }
    if ((k & 16) != 0) {
      // Mouse right
      x = 1;
    }
    if ((k & 4) != 0) {
      // Mouse up
      y = -1;
    }
    if ((k & 8) != 0) {
      // Mouse down
      y = 1;
    }

    // Mouse clicks
    if ((k & 1) != 0) {
      Mouse.press(MOUSE_LEFT);
      delay(50);
      while ((keyBits() & 1) != 0);
      Mouse.release(MOUSE_LEFT);
    }

    if ((k & 32) != 0) {
      Mouse.press(MOUSE_RIGHT);
      delay(50);
      while ((keyBits() & 32) != 0);
      Mouse.release(MOUSE_RIGHT);
    }

    // If keys moved, move mouse.
    if ((x != 0) || (y != 0)) {
      Mouse.move(x, y, 0);
      delay(mouseDelay--);
      x = y = 0;
    }
    // If acceleration is at maximum, do not exceed it!
    if (mouseDelay < 4) mouseDelay = 4;
  }
  Mouse.end();
  // Wait for all keys up
  while (keyBits() != 0);
}

void loop() {
  int x;

  x = keyWait();
#ifdef DEBUG
  Serial.println(x);
#endif /* DEBUG */
  if (x < 32) {
    if (numericed != 0)
      Keyboard.write(numericTable[x - 1]);
    else if (extraed != 0) {
#ifdef DEBUG
      Serial.print("Extra ");
      Serial.println(x);
#endif /* DEBUG */
      Keyboard.write(extraTable[x - 1]);
    } else if (funced != 0)
      Keyboard.write(funcTable[x - 1]);
    else
      Keyboard.write(alphaTable[x - 1]);

    if (shifted == 1) { // Clear a single shift.
      shifted = 0;
      Keyboard.release(KEY_LEFT_SHIFT);
    }
    if (controlled == 1) { // Clear a single control shift.
      controlled = 0;
      Keyboard.release(KEY_LEFT_CTRL);
    }
    if (alted == 1) { // Clear a single alt shift.
      alted = 0;
      Keyboard.release(KEY_LEFT_ALT);
    }
    // Clear temporary numeric, extra and func shift
    numericed &= 2;
    extraed &= 2;
    funced &= 2;
  } else {
    // Must be a shift function then
    x = shiftTable[x - 32];
    if (x < 256) {
      Keyboard.write(x);
    } else {
      // Its a more complicated keypress requiring a function.
      switch (x) {
        case KEYS_SHIFT_ON:
          if (++shifted > 2) shifted = 2;
          Keyboard.press(KEY_LEFT_SHIFT);
          break;

        case KEYS_CONTROL_SHIFT:
          if (++controlled > 2) controlled = 2;
          Keyboard.press(KEY_LEFT_CTRL);
          break;

        case KEYS_SHIFT_OFF:
          everythingOff();
          break;

        case KEYS_NUMERIC_SHIFT:
          if (++numericed > 2) numericed = 2;
          break;

        case KEYS_EXTRA_SHIFT:
          if (++extraed > 2) extraed = 2;
          break;

        case KEYS_ALT_SHIFT:
          if (++alted > 2) alted = 2;
          Keyboard.press(KEY_LEFT_ALT);
          break;

        case KEYS_FUNC_SHIFT:
          if (++funced > 2) funced = 2;
          break;

        case KEYS_MOUSE_MODE_ON:
          mouseMode();
          break;
      }
    }
  }
}
