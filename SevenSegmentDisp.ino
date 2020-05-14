#include <ThreeWire.h>  
#include <RtcDS1302.h>

// Configuratie
const int TimeModuleUpdate = 10000; // every 10 seconds
const int timeoutBuzzer    = 5000;
const int refreshTime      = 16;

int RCLKPin = 3;
int SRCLKPin = 4;
int dataPin = 2;
int d[] = {5,6,7,8};
ThreeWire myWire(10,9,11); // IO, SCLK, CE
int buzzerPin = 14;//A0;
int tempPin   = A2;
int knopKlant = A4;
int knopKassa = A5;
int ledPin    = 12;

// bits van cijfers
static unsigned char cijfers[] {
  252,96,218,242,102,182,190,224,254,246
};// 0 1 2 3 4 5 6 7 8 9

static unsigned char letters[] {
  238,62,156,122,158,142,110,28,252
};// A b C D      E F H E  L   O

RtcDS1302<ThreeWire> Rtc(myWire);
void setup() {
  pinMode(RCLKPin, OUTPUT);
  pinMode(SRCLKPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  for (int i=0;i<sizeof(d);i++){
    pinMode(d[i],OUTPUT);
    digitalWrite(d[i],true);
  }
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
  Rtc.SetDateTime(compiled);
  
  if (!Rtc.IsDateTimeValid()) {
    Rtc.SetDateTime(compiled);
  }
  
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Rtc.SetDateTime(compiled);
  }
  pinMode(buzzerPin,OUTPUT);
  pinMode(ledPin,OUTPUT);
  digitalWrite(buzzerPin,false);
}

void toonCijfer(unsigned char cijfer, unsigned char digidisplay, int duration){
  digitalWrite(digidisplay,false);
  digitalWrite(RCLKPin,LOW);
  shiftOut(dataPin,SRCLKPin,LSBFIRST,cijfer);
  digitalWrite(RCLKPin,HIGH);
  delay(duration);
  digitalWrite(digidisplay,true);
}

void toonUur(const RtcDateTime *dt, int duration){
  unsigned char uur     = dt->Hour();
  unsigned char minuut  = dt->Minute();
  unsigned char u1      = uur/10;
  unsigned char u2      = (uur-u1*10);
  unsigned char m1      = minuut/10;
  unsigned char m2      = (minuut-m1*10);
  // Show hour on first two displays
  toonCijfer(cijfers[u1],d[0],duration/4);
  toonCijfer(cijfers[u2]|0x01,d[1],duration/4);
  toonCijfer(cijfers[m1],d[2],duration/4);
  toonCijfer(cijfers[m2],d[3],duration/4);
}

void toonGetal(int cijfer, int duration){
  unsigned char duizendtal= cijfer/1000;
  unsigned char honderdtal = (cijfer-duizendtal*1000)/100;
  unsigned char tiental   = (cijfer-duizendtal*1000-honderdtal*100)/10;
  unsigned char eenheid   = (cijfer-duizendtal*1000-honderdtal*100-tiental*10);
  toonCijfer(cijfers[duizendtal],d[0],duration/4);
  toonCijfer(cijfers[honderdtal],d[1],duration/4);
  toonCijfer(cijfers[tiental],d[2],duration/4);
  toonCijfer(cijfers[eenheid],d[3],duration/4);
}

int currentTime       = 0;
int currentKlokTime   = 0;
int currentScreenTime = 0;
int currentBuzzerTime = -1;

void loop() {
  // Ontvang huidige tijd van RTC module
  RtcDateTime now = Rtc.GetDateTime();

  // Voor eeuwig laten runnen
  while (1) {
    int begint = millis();
    
    /*=========================*/
    /*         KNOPPEN         */
    /*=========================*/
    // Klant knop 
    if (analogRead(knopKlant)>1000) {
      // Boven 5V
      // Zet de led aan
      digitalWrite(ledPin,true);
      // Start timer
      currentBuzzerTime=0;
    }
    // Kassa knop
    if (analogRead(knopKassa)>1000) {
      // Zet de led terug uit
      digitalWrite(ledPin,false);
      // Zet ook de buzzer uit indien dit het geval was
      digitalWrite(buzzerPin,false);
      currentBuzzerTime=-1;
    }
    // Zijn de 5 seconden verlopen voor de buzzer? 
    if (currentBuzzerTime>=timeoutBuzzer) {
      // Zet de buzzer aan
      digitalWrite(buzzerPin,true);
    }

    
    /*=========================*/
    /*       TEMPERATUUR       */
    /*=========================*/
    float mv = analogRead(tempPin)/1024*5000;
    float celcius = mv/10;
    

    /*=========================*/
    /*     KLOK & SEGMENT      */
    /*=========================*/
    // Moeten we de klok updaten?
    if (currentKlokTime>=TimeModuleUpdate){
      // get newest time from module
      now = Rtc.GetDateTime();
      currentKlokTime=0;
    }
    // Moeten we het scherm updaten?
    if (currentScreenTime>=refreshTime){
      // Om de 4 ms
      toonUur(&now,refreshTime);
      // Ook currentTime updaten
      currentTime+=refreshTime;
      currentScreenTime=0;
    }

    // we updaten de timer met 1 ms
    delay(1);
    int eindet = millis();
    // Tijdsverschil berekenen
    int delta = (eindet-begint);
    
    // Alle huidige tijden optellen
    currentTime       += delta;
    currentKlokTime   += delta;
    currentScreenTime += delta;

    // Enkel optellen als de timer aan staat
    if (currentBuzzerTime>=0) {
      currentBuzzerTime+=delta;
    }
  }
}
