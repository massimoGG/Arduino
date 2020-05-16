/*
 * Arduino ELSY Advanced
 * Written by Massimo Giardina FIIW 2019-2020
 */
/*=========================*/
/*        LIBRARIES        */
/*=========================*/
// Clock module
#include <ThreeWire.h>  
#include <RtcDS1302.h>
// Capsense library
#include <CapacitiveSensor.h>

/*=========================*/
/*   TIJD CONFIGURATION    */
/*=========================*/
const int TimeModuleUpdate = 10000; // every 10 seconds
const int timeoutBuzzer    = 5000;
const int refreshTime      = 16;
const int SHOWTIME         = 5000;

/*=========================*/
/*   PIN CONFIGURATION     */
/*=========================*/
// Shiftregister
int RCLKPin = 3;
int SRCLKPin = 4;
int dataPin = 2;

// Common ground pins for 7segment displays
int gndSeg[] = {5,6,7,8};
int tempLED[] = {A2,A1,13};
int capsenseSend = A5;
int capsenseSensor = A4;

// Pin config for RTC 1302 module
ThreeWire myWire(10,9,11); // IO, SCLK, CE
// Pin config for Capsense
CapacitiveSensor   cs = CapacitiveSensor(capsenseSend,capsenseSensor);
// Buzzer pin
int buzzerPin = A0;//14;//A0;
// Temperature pin
int tempPin   = A3;
// Button of customer
int buttonCustomer = 0;
// Button of register
int buttonRegister = 1;
// LED pin
int ledPin    = 12;



/*=========================*/
/*        Main Code        */
/*=========================*/
// bits van cijfers
static unsigned char cijfers[] {
  252,96,218,242,102,182,190,224,254,246
};//0, 1,  2,  3,  4,  5,  6,  7,  8,  9,

static unsigned char letters[] {
  238,62,156,122,158,142,0,110,0,0,0,28,0,0,252,206,230,0,182,0,124,0,0,0,218
};//A, b,  C,  D,  E,  F,G,H,  I,J,K, L,M,N,  O,  P,  Q,R,  S,T,  U,V,X,Y,Z

// POSSIBLE SEGMENT ASCII TABLE
// -48 for offset!
static unsigned char ascii[] {
  252,96,218,242,102,182,190,224,254,246,0,0,0,0,0,0,0,238,62,156,122,158,142,0,110,0,0,0,28,0,0,252,206,230,0,182,0,124,0,0,0,218
};

// TIMERS for program -> Do not touch :)
int afwisseling = 0;
int currentKlokTime   = 0;
int currentScreenTime = 0;
int currentBuzzerTime = -1;
int currentTimeText   = -1;
int showText          = 0;

// Initialisation of RTC module
RtcDS1302<ThreeWire> Rtc(myWire);

void setup() {
  pinMode(RCLKPin, OUTPUT);
  pinMode(SRCLKPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  for (int i=0;i<sizeof(gndSeg);i++){
    pinMode(gndSeg[i],OUTPUT);
  }
  for (int i=0;i<sizeof(tempLED);i++){
    pinMode(tempLED[i],OUTPUT);
  }
  pinMode(buzzerPin,OUTPUT);
  pinMode(ledPin,OUTPUT);
  pinMode(buttonCustomer, INPUT_PULLUP);
  pinMode(buttonRegister, INPUT_PULLUP);

  // Start the RTC klok module & 
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
  if (!Rtc.IsDateTimeValid()) {
    Rtc.SetDateTime(compiled);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Rtc.SetDateTime(compiled);
  }
}

/*
 * Toon een Cijfer of letter op een bepaalde digidisplay voor duration ms
 */
void toonASCII(unsigned char c, unsigned char digidisplay, int duration) {
  // Ground op low zetten -> 0V
  digitalWrite(gndSeg[digidisplay],false);
  // RClock pin op low zetten -> Nieuwe seriele data komt binnen 
  digitalWrite(RCLKPin,LOW);
  // Shift de bits
  shiftOut(dataPin,SRCLKPin,LSBFIRST,ascii[c-48]);
  // RClock op high zetten -> Data is aangekomen
  digitalWrite(RCLKPin,HIGH);
  // Toon het voor duration time
  delay(duration);
  // Zet scherm uit
  digitalWrite(gndSeg[digidisplay],true);
}

/*
 * Toon een string die rolt.
 * aantal -> aantal keren
 * We geven dit een tijd. Als currentTime >timepershift -> shift
 * van links naar rechts
 */
void toonString(const char *string, int strsize, int showtime,int timepershift, int *currentTime) {
  static int currentShift; // -> the end

  if (*currentTime<0) {
    // We start at the beginning -> Variables stay unchanged
    *currentTime=0;
    currentShift = strsize-1;
  } else {
    if (*currentTime>=timepershift) {
      currentShift--;
      *currentTime=0;
    }
    if (currentShift<-strsize) {
      // End of string
      *currentTime=-1;
    }
  }

  // toon voor 4 disps
  for (int d=0;d<4;d++)
    if (currentShift+d<strsize and currentShift+d>=0)
      toonASCII(string[currentShift+d],d,showtime/4);
}

/*
 * Toon een cijfer op een digidisplay voor duration ms
 */
void toonCijfer(unsigned char cijfer, unsigned char digidisplay, int duration){
  digitalWrite(gndSeg[digidisplay],false);
  digitalWrite(RCLKPin,LOW);
  shiftOut(dataPin,SRCLKPin,LSBFIRST,cijfer);
  digitalWrite(RCLKPin,HIGH);
  delay(duration);
  digitalWrite(gndSeg[digidisplay],true);
}

/*
 * Toon de gegeven pointer dt tijd
 *  2 decimalen voor het uur afgesplitst met een '.' vervolgd door de minuten
 */
void toonUur(const RtcDateTime *dt, int duration){
  unsigned char uur     = dt->Hour();
  unsigned char minuut  = dt->Minute();
  unsigned char u1      = uur/10;
  unsigned char u2      = (uur-u1*10);
  unsigned char m1      = minuut/10;
  unsigned char m2      = (minuut-m1*10);
  // Show hour on first two displays
  toonCijfer(cijfers[u1],0,duration/4);
  toonCijfer(cijfers[u2]|0x01,1,duration/4);
  toonCijfer(cijfers[m1],2,duration/4);
  toonCijfer(cijfers[m2],3,duration/4);
}

/*
 * Toon de huidige temperatuur -> 2 decimalen voor komma en 1 na.
 *  ->voor een duration periode
 */
void toonTemp(float temp, int duration){
  unsigned char tiental   = (char)(temp/10);
  unsigned char eenheid   = (char)(temp-tiental*10);
  unsigned char komma1    = (char)((temp-tiental*10-eenheid)*10); // 0.52 ofzo*10->5.2
  unsigned char komma2    = (char)((temp-tiental*10-eenheid-komma1)); //0.52->
  // Show hour on first two displays
  toonCijfer(cijfers[tiental],0,duration/4);
  toonCijfer(cijfers[eenheid]|0x01,1,duration/4);
  toonCijfer(cijfers[komma1],2,duration/4);
  toonCijfer(letters[2],3,duration/4);
}

void toonGetal(int cijfer, int duration){
  unsigned char duizendtal= cijfer/1000;
  unsigned char honderdtal = (cijfer-duizendtal*1000)/100;
  unsigned char tiental   = (cijfer-duizendtal*1000-honderdtal*100)/10;
  unsigned char eenheid   = (cijfer-duizendtal*1000-honderdtal*100-tiental*10);
  toonCijfer(cijfers[duizendtal],0,duration/4);
  toonCijfer(cijfers[honderdtal],1,duration/4);
  toonCijfer(cijfers[tiental],2,duration/4);
  toonCijfer(cijfers[eenheid],3,duration/4);
}

/*
 * SetPin function
 */
void sp(int p, int s) {
  digitalWrite(p,s);
}

/*=========================*/
/*       TEMPERATUUR       */
/*=========================*/
double getC() {
  double celcius = (double)analogRead(tempPin)/1024*500;
  if(celcius>10){
    sp(tempLED[0],true);
    sp(tempLED[1],false);
    sp(tempLED[2],false);
  } else sp(tempLED[0],false);
  if(celcius>20) sp(tempLED[1],true);
  else sp(tempLED[2],true);
  return celcius;
}

void loop() {
  // Ontvang huidige tijd van RTC module
  RtcDateTime now = Rtc.GetDateTime();
  /*=========================*/
  /*       TEMPERATUUR       */
  /*=========================*/
  double celcius = getC();

  // Voor eeuwig laten runnen
  while (1) {
    int begint = millis();
    
    /*=========================*/
    /*         KNOPPEN         */
    /*=========================*/
    //if (digitalRead(buttonCustomer)==1 or
    if (cs.capacitiveSensor(30)>1000) {
      digitalWrite(ledPin,true);
      // Start timer
      currentBuzzerTime=0;
    }
    if (digitalRead(buttonRegister)==1) {
      digitalWrite(ledPin,false);
      digitalWrite(buzzerPin,false);
      currentBuzzerTime=-1;
      showText=2;
    }
    if (currentBuzzerTime>=timeoutBuzzer) {
      digitalWrite(buzzerPin,true);
    }

    /*=========================*/
    /*     KLOK & SEGMENT      */
    /*=========================*/
    // Moeten we de klok updaten?
    if (currentKlokTime>=TimeModuleUpdate){
      // Ontvang huidige tijd van RTC module
      now = Rtc.GetDateTime();
      currentKlokTime=0;
      celcius = getC();
    }
    // Moeten we het scherm updaten?
    if (currentScreenTime>=refreshTime){
      /*=========================*/
      /*      ROLLING TEXT       */
      /*=========================*/
      if (showText>0) {
        // Show HELLO for two times
        toonString("HELLO", 5, refreshTime, 200, &currentTimeText);
        if (currentTimeText==-1) {
          // End of string
          showText--;
        }
      } else {
        // Show clock & temp
        if (afwisseling<SHOWTIME) {
          toonUur(&now,refreshTime);
        } else {
          toonTemp(celcius,refreshTime);
        }
      }
      currentScreenTime=0;
    }
    
    /*=========================*/
    /*        TIMER CORE       */
    /*=========================*/
    // we updaten de timer met 1 ms
    delay(1);
    int eindet = millis();
    int delta = (eindet-begint);
    
    currentKlokTime   += delta;
    currentScreenTime += delta;
    if (currentTimeText>=0) currentTimeText += delta;

    if (currentBuzzerTime>=0) currentBuzzerTime += delta;

    afwisseling+=delta;
    if (afwisseling>=SHOWTIME*2){
      afwisseling=0;
    }
  }
}
