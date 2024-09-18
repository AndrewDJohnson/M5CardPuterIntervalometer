// Based on a sketch will send out a Nikon D50 trigger signal (probably works with most Nikons)
// See the full tutorial at https://learn.adafruit.com/ir-sensor/making-an-intervalometer
// Adapted for M5-stack cardputer by A.D. Johnson in Sept 2024
//
// Can now be used as a manual remote control (single shot) or as an Intervalometer, with settings entered on the Cardputer's keyboard!
// Countown and Photo Counter are displayed on the Cardputer's small screen!
// The idea is then that multiple pictures are taken and can be made into a time-lapse video!
// Code has been included to trigger Canon, Nikon, Sony, Olympus, Pentax and Minolta cameras

#include "M5Cardputer.h"
#include "M5GFX.h"

//The screen will go off in 60 secs in Intervalometer mode!
#define SCREEN_OFF_TIME 60

typedef enum {
  ST_READING_VALUES,
  ST_INTERVALOMETER_RUNNING,
  ST_MANUAL_CONTROL
} CURRENT_STATE_LIST;

//In the cardputer, the IR LED is connected to Pin 44
int IRledPin = 44;

// Variables to track timing for sending IR pulses to take photos!
int interval;
int overall_period;
int screen_timout;
int current_state = ST_READING_VALUES;

unsigned long previousMillis = 0;
unsigned long intervalMillis;  // Interval in milliseconds
unsigned long durationMillis;  // Overall duration to milliseconds
unsigned long one_second, countdown;
unsigned long startTime = 0;
unsigned long photo_count, max_photos;

/*************************************************************************
* Function:      check_key
* Parameters:    delay value (ms) - if this is < 0 it will wait "forever"
* Description:   Checks and optionally waits for a keypress.
*
* Returns:       keypress as a string
**************************************************************************/
String check_key(int delay_val = 0) {
  String data = "";
  long screen_off_counter = SCREEN_OFF_TIME * 1000;

  unsigned long end_millis;
  end_millis = millis() + delay_val;

  do {
    delay(1);
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        if (status.del) {
          data += "\b";
        } else if (status.enter) {
          data += "\n";
        } else {
          for (auto i : status.word) {
            data += i;
          }
        }
      }
      screen_on();
      break;
    } else {
      if (!screen_off_counter--) {
        screen_off();
      }
    }
  } while ((millis() < end_millis) || (delay_val < 0));

  return data;
}

/*************************************************************************
* Function:      get_value (has to be positive, non-zero)
* Parameters:    Screen prompt (char *), vertical screen position for prompt (int)
* Description:   Read an integer value from the M5 cardputer keyboard, echoing characters.
*
* Returns:       integer value entered
**************************************************************************/
int get_value(char *prompt, int vert_pos = 0) {
  String key = "", data = "";
  int entered_value = 0;

  M5.Lcd.setCursor(0, vert_pos);
  M5.Lcd.printf(prompt);
  while (!entered_value) {
    key = check_key(-1);

    if (key == "\n") {
      //If non-digits are entered, this will give a zero value.
      entered_value = data.toInt();
      //Check that a valid non-zero value was entered...
      if (!entered_value) {
        M5.Lcd.clear();

        M5.Lcd.printf("\n  ENTER A VALID\n\n   NON ZERO VALUE!");
        delay(1500);
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, vert_pos);
        M5.Lcd.printf(prompt);
        data = "";
      }
    } else if (key == "\b") {
      data.remove(data.length() - 1);
    } else {
      data += key;
    }

    //Print the data entered so far to the screen.
    M5.Lcd.setCursor(0, vert_pos + 20);
    M5.Lcd.printf(">%s ", data.c_str());
  }
  return (entered_value);
}

/*************************************************************************
* Function:      screen_on
* Parameters:    None
* Description:   Set the M5 screen to 50% brightness and reset the timer
*
* Returns:       Nothing
**************************************************************************/
void screen_on() {
  M5.Lcd.setBrightness(50);
  M5.Lcd.wakeup();
  //Reset the timeout
  screen_timout = SCREEN_OFF_TIME;
}
/*************************************************************************
* Function:      screen_off
* Parameters:    None
* Description:   Set the M5 screen to 0% brightness and "sleep" to save
*                 battery!
*
* Returns:       Nothing
**************************************************************************/
void screen_off() {
  M5.Lcd.setBrightness(0);
  M5.Lcd.sleep();
}

/*************************************************************************
* Function:      setup
* Parameters:    None
* Description:   Standard arduino entry point. Here sets up hardware and
                 program variables.
*
* Returns:       Nothing
**************************************************************************/
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  //Set the IR LED pin as an output!
  pinMode(IRledPin, OUTPUT);
  //Configure speaker for beeping later
  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(30);
  Serial.begin(9600);
  current_state = ST_READING_VALUES;
}

/*************************************************************************
* Function:      show_startup_options
* Parameters:    None
* Description:   Shows startup options then waits for an option key to be
                 pressed. Sets the next state.
* Returns:       Nothing
**************************************************************************/
void show_startup_options() {
  //Setup the screen....
  screen_on();
  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 10);

  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.printf("  INTERVALOMETER\n");
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.printf("CANON-NIKON-OLYMPUS\n");
  M5.Lcd.printf("SONY-PENTAX-MINOLTA");
  M5.Lcd.setTextColor(WHITE, BLACK);

  //Now we can wait to either enter intervalometer mode, or single shot!
  M5.Lcd.printf("\n    PRESS R FOR\n  SIMPLE REMOTE\n      OR I FOR\n  INTERVALOMETER!");

  do {
    String key = check_key(-1);
    //Chose between keyboard controlled remote and interval timer! 
    if (key == "i") {
      
      //Get the 2 values we need - read them from the keyboard!
      read_values();
      //Set up the time values....
      intervalMillis = interval * 1000;             // Convert interval to milliseconds
      durationMillis = overall_period * 60 * 1000;  // Convert duration to milliseconds

      //Reset control variables.
      photo_count = 0;
      max_photos = overall_period * 60 / interval;
      //Set a blue screen for intervalometer.
      M5.Lcd.clear(BLUE);
      M5.Lcd.setBrightness(50);

      M5.Lcd.setTextSize(3);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(0, 0);

      //Report the settings that were entered.
      M5.Lcd.printf("%dSEC, %dMIN\n%d PHOTOS", interval, overall_period, max_photos);
      Serial.printf("%d secs and %d mins -  set", interval, overall_period);
      M5.Lcd.setTextColor(YELLOW, BLUE);
      //Next, the "loop" will start!
      current_state = ST_INTERVALOMETER_RUNNING;
      // Initialize start time
      startTime = one_second = millis();
      countdown = intervalMillis / 1000;
      break;

    } else 
    //Check for single shot mode selection.
    if (key == "r") {
      current_state = ST_MANUAL_CONTROL;
      
      show_single_shot_info();
      break;
    }
  } while (true);
  screen_timout = SCREEN_OFF_TIME;
}

/*************************************************************************
* Function:      show_single_shot_info
* Parameters:    None
* Description:   Just shows simple instructions for single shot mode.
* Returns:       The key that was pressed
**************************************************************************/
void show_single_shot_info() {
  M5.Lcd.clear(GREEN);
  //Set up screen for manual remote.
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(BLACK, GREEN);
  M5.Lcd.printf("\n PRESS KEY\n TO SHOOT\n\n X TO ABORT");
}

/*************************************************************************
* Function:      start_single_shot
* Parameters:    None
* Description:   Performs "single shot" mode - calling the TakePhoto()
*                function whenever a key is pressed - X aborts
* Returns:       The key that was pressed
**************************************************************************/
String start_single_shot() {
  //Now you can just press a key to take a photo!
  //We stay here and don't enter the loop!
  String key;

  //Check if a key was pressed.
  key = check_key(1);
  if (key != "") {
    if (key != "x") {
      M5.Lcd.clear(YELLOW);
      M5.Lcd.setTextColor(BLACK, YELLOW);
      M5.Lcd.setCursor(10, 20);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf("\n TAKING PHOTO");

      //Send IR pulses/codes to trigger camera shutter release!
      TakePhoto();
      show_single_shot_info();
      M5.Speaker.tone(4000, 20);
      
    }
  }
  return (key);
}

/*************************************************************************
* Function:      loop
* Parameters:    None
* Description:   Standard Arduino looping entry point!
*
* Returns:       Nothing
**************************************************************************/
void loop() {
  unsigned long currentMillis;
  static bool interval_loop_running = true;
  String key_pressed = "";
  int bat_level;

  // Get the current time
  currentMillis = millis();

  // Check if it's time to call the send_ir function etc - check the countdown value!
  if (currentMillis >= one_second) {
    one_second = currentMillis + 1000;
    //Decrement the screen timer every second and switch it off if timed out!
    if (!--screen_timout) {
      screen_off();
    }
    if (current_state == ST_INTERVALOMETER_RUNNING) {
      bat_level=M5.Power.getBatteryLevel();
      if (bat_level>10)
      {
          M5.Lcd.setTextColor(YELLOW, BLUE);
      }
      else
      {
          M5.Lcd.setTextColor(RED, BLUE);
      }
      //Show the photo counter and countdown etc on the screen
      M5.Lcd.setCursor(0, 55);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf(" %03d/%03d\n", photo_count, max_photos);
      M5.Lcd.setTextSize(2);
      
      M5.Lcd.printf("\nNEXT IN %02d (X-ABORT)\nBAT LEVEL %d%%", countdown - 1,bat_level);
      if (!--countdown) {
        //Reset the countdown  figure to that of the interval.
        countdown = intervalMillis / 1000;

        TakePhoto();

        //Count the no of photos taken....
        photo_count++;

        //Have we taken all the photos?
        if (photo_count == max_photos) {
          //The interval has now elapsed!
          Serial.println("Duration completed.");

          M5.Lcd.clear(BLACK);
          M5.Lcd.setTextSize(3);
          M5.Lcd.setCursor(0, 0);
          M5.Lcd.setTextColor(GREEN);
          M5.Lcd.printf("\n  FINISHED!\n\n PRESS A KEY!");
          //Sound tones to alert user to completion.                    
          for (int i=0; i<3; i++)
          {
            M5.Speaker.tone(6000, 400);
            delay (400);           
            M5.Speaker.tone(4000, 400);
            delay (400);           
          }
          screen_on();
          check_key(-1);
          current_state = ST_READING_VALUES;

        }
      }
    }
  }
  //Now do different things depending on the state we're in.
  switch (current_state) 
  {
    case  ST_MANUAL_CONTROL:
      //We need to check if "x" was pressed in single shot mode/
      key_pressed = start_single_shot();
      break;
    //Menu to be displayed etc.  
    case ST_READING_VALUES:
      show_startup_options();
      //Now exit this iteration of the "loop" function so the display isn't changed (below)
      return;
  //During intervalometer mode, we need to check the keyboard
  //Otherwise, we will just have the key pressed in single shot mode!
    case ST_INTERVALOMETER_RUNNING:
      key_pressed = check_key(1);
      break;
  }
  //Was a key pressed?
  if (key_pressed != "") {
    //Switch the screen back on!
    screen_on();

    //Should we abort the Intervalometer?
    if (key_pressed == "x") {
      M5.Lcd.clear();
      M5.Lcd.setCursor(25, 50);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf("ABORTED");
      delay(2000);
      current_state = ST_READING_VALUES;
    }
  }
}

/*************************************************************************
* Function:      pulseIR
* Parameters:    microsecs - pulse length.
* Description:   This procedure sends a 38KHz pulse to the IRledPin 
 for a certain # of microseconds. We'll use this whenever we need to send codes
*
* Returns:       Nothing
**************************************************************************/
void pulseIR(long microsecs) {

  cli();  // this turns off any background interrupts

  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working

    // so 26 microseconds altogether
    microsecs -= 26;
  }

  sei();  // this turns interrupts back on
}
/*************************************************************************
* Function:      SendCanonCode
* Parameters:    None
* Description:   Should send the code to trigger shutter release on a 
*                Canon Camera!
*
* Returns:       Nothing
**************************************************************************/
void SendCanonCode() {

  for (int i = 0; i < 16; i++) {
    digitalWrite(IRledPin, HIGH);
    delayMicroseconds(11);
    digitalWrite(IRledPin, LOW);
    delayMicroseconds(11);
  }
  delayMicroseconds(7330);

  for (int i = 0; i < 16; i++) {
    digitalWrite(IRledPin, HIGH);
    delayMicroseconds(11);
    digitalWrite(IRledPin, LOW);
    delayMicroseconds(11);
  }
  sei();  // this turns off any background interrupts
}
/*************************************************************************
* Function:      SendNikonCode
* Parameters:    None
* Description:   Sends IR pulses for Nikon camera shutter release.
*
* Returns:       Nothing
**************************************************************************/
void SendNikonCode() {
  // This is the code for my particular Nikon, for others use the tutorial
  // to 'grab' the proper code from the remote

  pulseIR(2080);
  delay(27);
  pulseIR(440);
  delayMicroseconds(1500);
  pulseIR(460);
  delayMicroseconds(3440);
  pulseIR(480);

  delay(65);  // wait 65 milliseconds before sending it again
  pulseIR(2000);
  delay(27);
  pulseIR(440);
  delayMicroseconds(1500);
  pulseIR(460);
  delayMicroseconds(3440);
  pulseIR(480);
}
/*************************************************************************
* The following functions were copied/converted from older CPP/INO 
* code so that the other makes of camera can be supported.
* Each function is called (elsewhere) in turn to avoid needing to select a
* particular camera!
**************************************************************************/
void SendPentaxCode() {
  pulseIR(13000);
  delayMicroseconds(3000);
  for (int i = 0; i < 7; i++) {
    pulseIR(1000);
    delayMicroseconds(1000);
  };
}
//*************************************************************************
void SendOlympusCode() {
  const byte seq[] = { 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 };
  const byte count = sizeof(seq) / sizeof(seq[0]);
  pulseIR(8972);
  delayMicroseconds(4384);
  pulseIR(624);
  for (int i = 0; i < count; i++) {
    if (seq[i]) {
      delayMicroseconds(1600);
      pulseIR(600);
    } else {
      delayMicroseconds(488);
      pulseIR(500);
    }
  }
}
//*************************************************************************
void SendMinoltaCode() {
  const byte seq[] = { 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
  const byte count = sizeof(seq) / sizeof(seq[0]);
  pulseIR(3750);
  delayMicroseconds(1890);
  for (int i = 0; i < count; i++) {
    if (seq[i]) {
      pulseIR(456);
      delayMicroseconds(1430);
    } else {
      pulseIR(456);
      delayMicroseconds(487);
    }
  }
}
//*************************************************************************
void SendSonyCode() {
  const byte seq[] = { 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1 };
  const byte count = sizeof(seq) / sizeof(seq[0]);
  for (int j = 0; j < 3; j++) {
    pulseIR(2320);
    delayMicroseconds(650);
    for (int i = 0; i < count; i++) {
      if (seq[i]) {
        pulseIR(1175);
        delayMicroseconds(650);
      } else {
        pulseIR(575);
        delayMicroseconds(650);
      }
    }
    delayMicroseconds(10000);
  }
}

//*************************************************************************
void SendCanonWLDC100Code() {
  const byte seq[] = { 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
  const byte count = sizeof(seq) / sizeof(seq[0]);
  pulseIR(9042);
  delayMicroseconds(4379);

  for (int i = 0; i < count; i++) {
    if (seq[i]) {
      pulseIR(612);
      delayMicroseconds(1621);
    } else {
      pulseIR(612);
      delayMicroseconds(512);
    }
  };
  pulseIR(599);
}

#define SEND_DELAY 60
/*************************************************************************
* Function:      TakePhoto
* Parameters:    None
* Description:   Calls functions to take photos with the various cameras.
*                We delay between each send, so as not to "confuse" each 
                 camera's IR sensing etc
* Returns:       Nothing
**************************************************************************/
void TakePhoto() {
  cli();  // this turns off any background interrupts
  SendNikonCode();
  delay(SEND_DELAY);
  SendCanonCode();
  delay(SEND_DELAY);
  SendCanonWLDC100Code();
  delay(SEND_DELAY);
  SendPentaxCode();
  delay(SEND_DELAY);
  SendOlympusCode();
  delay(SEND_DELAY);
  SendMinoltaCode();
  delay(SEND_DELAY);
  SendSonyCode();
  sei();  //Reenable interrupts
}

/*************************************************************************
* Function:      read_values
* Parameters:    None
* Description:   Read the Interval and Overall Period and assign the vars
*
* Returns:       Nothing
**************************************************************************/
void read_values() {
  M5.Lcd.clear();
  M5.Lcd.setTextSize(2);

  interval = get_value("Interval? (sec):");
  overall_period = get_value("Duration? (min): ", 35);
}
