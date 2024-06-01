#include <TimeLib.h>
#include <CronAlarms.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
//#define TIME_HEADER  255   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define ZAD 0
#define DOM 1

time_t t;

ThreeWire myWire(1, 2, 0);        // DAT, CLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);    // RTC Object

void setup() {
  Serial.begin(115200);
  Rtc.Begin();
  if(!Rtc.GetIsRunning()) {
    Serial.println("RTC is not running");
    RtcDateTime now = Rtc.GetDateTime();
    printDateTime(now);
    timeSync();
    rtcSetup();
  }
  //Serial.end();
  powerDownAll();
  setupCronJobs();
}
void rtcSetup() {
  if (Rtc.GetIsRunning()) {
    return;
  }
  if (Rtc.IsDateTimeValid()) {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing
    return;
  }
  Serial.println("RTC lost confidence in the DateTime!");
  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }
  Serial.println("Compiled1: ");
  RtcDateTime st = RtcDateTime(year(),month(),day(), hour(), minute(), second());
  printDateTime(st);
  Rtc.SetDateTime(st);

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsWriteProtected(true);
    Rtc.SetIsRunning(true);
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
void setupCronJobs() {
  Cron.create("*/2 * * * 1 *", Repeats, false);           // timer for every 15 seconds
}

void Repeats() {
  Serial.println("2 second timer");
}

void powerDownAll() {
  offServo(ZAD);
  offServo(DOM);
}
void offServo(int servo) {
  if (servo == DOM)
  Serial.println("DOM Motor down");
}

void loop() {
  RtcDateTime st = Rtc.GetDateTime();
  printDateTime(st);
  #ifdef __AVR__
  system_tick(); // must be implemented at 1Hz
  #endif
  //time_t tnow = time(nullptr);
  time_t tnow = now();
  Serial.println(asctime(gmtime(&tnow)));
  Cron.delay();// if the loop has nothing else to do, delay in ms
               // should be provided as argument to ensure immediate
               // trigger when the time is reached
  delay(1000);
}
void timeSync() {
  if (!Serial) {
    return;
  }
  bool detected = false;
  t = now();
  Serial.println("Unsynced time");
  Serial.println(t);
  while (!detected) {
    if (Serial.available() > 0) {
      //setupTime();
      char c = Serial.read() ; 
      Serial.print(c);  
      if( c == TIME_HEADER ) {       
       time_t pctime = 0;
       for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial.read();          
        if( c >= '0' && c <= '9'){   
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
        }
       }   
       setTime(pctime);   // Sync Arduino clock to the time received on the serial port
      }  

      detected = true;
      t = now();
      Serial.println(t);
      Serial.print("TIMESYNCED");
    } else {
      Serial.print("SYNCTIME");
    }
    delay(500);
  }
}

void setupTime() {
  t = now();
  int y = year(t);
  if (y < 2024) {
    Serial.print("Time not synced , year ");
    Serial.println(y);
  }
  t = now();
  Serial.println(t);
  Serial.print("TIMESYNCED");
}
