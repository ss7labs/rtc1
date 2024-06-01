#include <TimeLib.h>
#include <EEPROM.h>
/*
09 ON_DOM
10 OFF_DOM

07 ON_ZAD
08 OFF_ZAD

06 LED_DOM
05 BTN_DOM

04 LED_ZAD GREEN
03 BTN_ZAD GREEN
*/
#define OFF_DOM 10
#define ON_DOM  9
#define OFF_ZAD 8
#define ON_ZAD  7
#define LED_DOM 6
#define BTN_DOM 5
#define LED_ZAD 4
#define BTN_ZAD 3
#define DEBUG 0

bool LockByCron = false;

void setup() {
  pinMode(ON_DOM,OUTPUT);
  pinMode(OFF_DOM,OUTPUT);
  pinMode(ON_ZAD,OUTPUT);
  pinMode(OFF_ZAD,OUTPUT);
  pinMode(LED_DOM,OUTPUT);
  pinMode(LED_ZAD,OUTPUT);
  pinMode(BTN_DOM, INPUT_PULLUP);
  pinMode(BTN_ZAD, INPUT_PULLUP);
  digitalWrite(OFF_ZAD, HIGH);
  digitalWrite(ON_ZAD, HIGH);
  digitalWrite(OFF_DOM, HIGH);
  digitalWrite(ON_DOM, HIGH);

  if (DEBUG) {
    Serial.begin(115200);
  }
  //Serial.end();
  setupTime();
  powerDownAll();
}

void powerDownAll() {
  //return;
  onOff(LED_ZAD,OFF_ZAD,false);
  onOff(LED_DOM,OFF_DOM,false);
}
void onOff(int led, int servo, bool on) {
  int signal = LOW;
  if (on) {signal = HIGH;}
  digitalWrite(servo, LOW);
  blink(led, signal);
  digitalWrite(servo, HIGH);
  digitalWrite(led, signal);
}

void blink(int led, int ledState){
  int dt = 20;
  while (dt > 0) {
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(led, ledState);
    delay(1000);
    dt--;
  } 
}

void loop() {
  cron();
  detectButtonPressed();
  delay(500);
}

bool btnPressed = false;
void detectButtonPressed() {
  if (btnPressed) {return;}
  if (LockByCron) {return;}
  int zadBtn = digitalRead(BTN_ZAD);
  if (zadBtn != LOW) {
    btnPressed = true;
    printDebugString("BTN_ZAD pressed");
    switchState("ZAD");
    btnPressed = false;
    return;
  }
  int domBtn = digitalRead(BTN_DOM);
  if (domBtn != LOW) {
    btnPressed = true;
    printDebugString("BTN_DOM pressed");
    switchState("DOM");
  }
  btnPressed = false;
}

bool zad_state_on = false;
bool dom_state_on = false;
void switchState(const char* stateName) {
  int led, servo;
  bool on = true;
  if (stateName == "ZAD") {
    printDebugString("ZAD switching");
    led = LED_ZAD;
    servo = ON_ZAD;
    if (zad_state_on) {
      on = false;
      servo = OFF_ZAD;
      zad_state_on = false;
    } else {
      zad_state_on = true;
    }
  }
  if (stateName == "DOM") {
    printDebugString("DOM switching");
    led = LED_DOM;
    servo = ON_DOM;
    if (dom_state_on) {
      on = false;
      servo = OFF_DOM;
      dom_state_on = false;
    } else {
      dom_state_on = true;
    }
  }
  onOff(led,servo,on);
}

void cron() {
  int m = month();
  switch (m) {
  case 12:
    printDebugString("Winter 12");
    break;
  case 1 ... 2:
    printDebugString("Winter 1,2");
    break;
  case 3 ... 4:
    printDebugString("Spring 3,4,5");
    break;
  case 9 ... 11:
    printDebugString("Autumn 9,10,11");
    break;
  default:
    //printDebugString("Summer 6,7,8");
    eachHour();
    //eachMin(3);
    break;
  }
}

void printDebugString(const char* msg) {
  if (!DEBUG) {return;}
  if (!Serial) {return;}
  Serial.println(msg);
}
void printDebugStringInt(const char* msg,int d) {
  if (!DEBUG) {return;}
  if (!Serial) {return;}
  Serial.print(msg);
  Serial.println(d);
}
void printDebugStringTime(const char* msg,time_t t) {
  if (!DEBUG) {return;}
  if (!Serial) {return;}
  Serial.print(msg);
  Serial.println(t);
}

int prevMin = 0;
void eachMin(int min) {
  int m = minute();
  if (m % min != 0) {return;}
  if ((m - prevMin) == 0) {return;}
  prevMin = m;
  printDebugStringInt("Trigger min: ",m);
  runTrigger(m);
  saveTime();
}

int prevHour = -1;
void eachHour() {
  int h = hour();
  if ((h - prevHour) == 0) { return;}
  /*
  if ((h % 12) == 0) {
    saveTime();
  }
  */
  prevHour = h;
  printDebugStringInt("Trigger hour: ",h);
  runTrigger(h);
  saveTime();
}

void saveTime() {
  time_t t = now();
  int eeAddress = 0;
  EEPROM.put(eeAddress, t);
  printDebugStringTime("Put time to EEPROM: ",t);
}

void runTrigger(int h) {
  LockByCron = true;
  //A number is "even" if the least significant bit is zero.
  // Run ZAD
  if ( (h & 0x01) == 0) { 
    printDebugStringInt("Even hour: ",h);
    if (dom_state_on) {
      dom_state_on = false;
      onOff(LED_DOM, OFF_DOM,false);
    }
    if (zad_state_on) {
      LockByCron = false;
      return;
    }
    zad_state_on = true;
    onOff(LED_ZAD, ON_ZAD,true);
    LockByCron = false;
    return;
  }
  //Run DOM
  printDebugStringInt("Odd hour: ",h);
  if (zad_state_on) {
      zad_state_on = false;
      onOff(LED_ZAD, OFF_ZAD,false);
  }
  if (dom_state_on) {
      LockByCron = false;
      return;
  }
  dom_state_on = true;
  onOff(LED_DOM, ON_DOM,true);
  LockByCron = false;
}

void setupTime() {
  time_t t;
  t = cvt_date(__DATE__, __TIME__);
  printDebugStringTime("Unix Time: ",t);
  printDebugStringTime("Unix Time size: ",t);

  time_t tEE;
  int eeAddress = 0;
  EEPROM.get(eeAddress, tEE);

  if (tEE > t) {
    printDebugStringTime("Set time from EEPROM: ",tEE);
    setTime(tEE);
  } else {
    printDebugStringTime("Set compiled time: ",t);
    setTime(t);
  }
}

time_t cvt_date(char const *date, char const *time)
{
    char s_month[5];
    int year, day;
    int hour, minute, second;
    tmElements_t t;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(date, "%s %d %d", s_month, &day, &year);
    //sscanf(time, "%2hhd %*c %2hhd %*c %2hhd", &t.Hour, &t.Minute, &t.Second);
    sscanf(time, "%2d %*c %2d %*c %2d", &hour, &minute, &second);
    // Find where is s_month in month_names. Deduce month value.
    t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;
    t.Day = day;
    // year can be given as '2010' or '10'. It is converted to years since 1970
    if (year > 99) t.Year = year - 1970;
    else t.Year = year + 50;
    t.Hour = hour;
    t.Minute = minute;
    t.Second = second;
    return makeTime(t);
}