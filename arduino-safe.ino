#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Servo.h>
#include "SafeState.h"

// pin 
#define SERVO_PIN 6
#define SERVO_LOCK_POS   20
#define SERVO_UNLOCK_POS 90

Servo lockServo;

// display lcd
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {5, 4, 3, 2};
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// crea keypad con configurazione scritta
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// questa classe usa la eprom per leggere e salvare il codice in modo sicuro
SafeState safeState;


void setup() {
  lcd.begin(16, 2);
  // init_icons(lcd);

  lockServo.attach(SERVO_PIN);

  /* Make sure the physical lock is sync with the EEPROM state */
  Serial.begin(115200);
  if (safeState.locked()) {
    lock();
  } else {
    unlock();
  }

  showStartupMessage();
}

/* gac:start
   ✅ Keep your `loop()` function short.
   In this case, the app has two main states: the safe is either
   currently open or locked. Each of these states is handled by
   a dedicated function. This separation makes sure that each
   function is smaller and focused on a single task, and makes
   the code easier to understand and reason about.
*/
void loop() {
  if (safeState.locked()) {
    safeLockedLogic();
  } else {
    safeUnlockedLogic();
  }
}

/* quando la cassaforte è sbloccata */
void safeUnlockedLogic() {
  lcd.clear();

  //lcd.setCursor(0, 0);
  // lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(2, 0);
  lcd.print(" # per chiudere");
  // lcd.setCursor(15, 0);
  // lcd.write(ICON_UNLOCKED_CHAR);

  bool newCodeNeeded = true;

  if (safeState.hasCode()) {
    lcd.setCursor(0, 1);
    lcd.print("  A = per impostare nuovo codice");
    newCodeNeeded = false;
  }

  auto key = keypad.getKey();
  while (key != 'A' && key != '#') {
    key = keypad.getKey();
  }

  bool readyToLock = true;
  if (key == 'A' || newCodeNeeded) {
    readyToLock = setNewCode();
  }

  if (readyToLock) {
    lcd.clear();
    // lcd.setCursor(5, 0);
    // lcd.write(ICON_UNLOCKED_CHAR);
    // lcd.print(" ");
    // lcd.write(ICON_RIGHT_ARROW);
    // lcd.print(" ");
    // lcd.write(ICON_LOCKED_CHAR);

    safeState.lock();
    lock();
    showWaitScreen(100);
  }
}


/* cassaforte bloccata */
void safeLockedLogic() {
  lcd.clear();
  lcd.setCursor(0, 0);
  // lcd.write(ICON_LOCKED_CHAR);
  lcd.print(" Cassaforte bloccata! ");
  // lcd.write(ICON_LOCKED_CHAR);

  // input del codice segreto da parte dell'utente
  String userCode = inputSecretCode();

  // true se il codice è corretto altrimenti falso
  bool unlockedSuccessfully = safeState.unlock(userCode);
  showWaitScreen(200);

  if (unlockedSuccessfully) {
    showUnlockMessage();
    unlock();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Accesso negato!");
    showWaitScreen(1000);
  }
}


/* imposta il servo alla posizione di lock */
void lock() {
  lockServo.write(SERVO_LOCK_POS);
  safeState.lock();
}

/* imposta il servo alla posizione di unlock */
void unlock() {
  lockServo.write(SERVO_UNLOCK_POS);
}

/* messaggio di benvenuto inziiale */
void showStartupMessage() {
  lcd.setCursor(4, 0);
  lcd.print("Benvenuto!");
  delay(1000);

  lcd.setCursor(0, 2);
  String message = "ArduinoSafe";
}


/* prende il codice dal numpad e lo returna come stringa */
String inputSecretCode() {
  lcd.setCursor(5, 1);
  lcd.print("[____]");
  lcd.setCursor(6, 1);
  String result = "";
  while (result.length() < 4) {
    char key = keypad.getKey();
    
    if (key >= '0' && key <= '9') {
      lcd.print('*');
      result += key;
    }
  }
  return result;
}



/* simula caricamento cassaforte */
void showWaitScreen(int delayMillis) {
  lcd.setCursor(2, 1);
  lcd.print("[..........]");
  lcd.setCursor(3, 1);
  for (byte i = 0; i < 10; i++) {
    delay(delayMillis);
    lcd.print("=");
  }
}


/* imposta nuovo codice che viene salvato nella eprom */
bool setNewCode() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter new code:");
  String newCode = inputSecretCode();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Confirm new code");
  String confirmCode = inputSecretCode();

  if (newCode.equals(confirmCode)) {
    safeState.setCode(newCode);
    return true;
  } else {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Code mismatch");
    lcd.setCursor(0, 1);
    lcd.print("Safe not locked!");
    delay(2000);
    return false;
  }
}

/* messaggio cassaforte sbloccata */
void showUnlockMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  // lcd.write(ICON_UNLOCKED_CHAR);
  lcd.setCursor(4, 0);
  lcd.print("Cassaforte sbloccata!");
  lcd.setCursor(15, 0);
  // lcd.write(ICON_UNLOCKED_CHAR);
  delay(1000);
}
