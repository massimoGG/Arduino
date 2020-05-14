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
int buzzerPin = A0;//14;//A0;
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
  pinMode(knopKlant, INPUT_PULLUP);
  pinMode(knopKassa, INPUT_PULLUP);
}

/*
 * Toon een Cijfer of letter op een bepaalde digidisplay voor duration ms
 */
void toonASCII(unsigned char c, unsigned char digidisplay, int duration) {
  
}
void toonString(const char *string, int lengte) {
  
}

/*
 * Toon een cijfer op een digidisplay voor duration ms
 */
void toonCijfer(unsigned char cijfer, unsigned char digidisplay, int duration){
  digitalWrite(digidisplay,false);
  digitalWrite(RCLKPin,LOW);
  shiftOut(dataPin,SRCLKPin,LSBFIRST,cijfer);
  digitalWrite(RCLKPin,HIGH);
  delay(duration);
  digitalWrite(digidisplay,true);
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
  toonCijfer(cijfers[u1],d[0],duration/4);
  toonCijfer(cijfers[u2]|0x01,d[1],duration/4);
  toonCijfer(cijfers[m1],d[2],duration/4);
  toonCijfer(cijfers[m2],d[3],duration/4);
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
  toonCijfer(cijfers[tiental],d[0],duration/4);
  toonCijfer(cijfers[eenheid]|0x01,d[1],duration/4);
  toonCijfer(cijfers[komma1],d[2],duration/4);
  toonCijfer(letters[2],d[3],duration/4);
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

int afwisseling = 0;
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
    if (digitalRead(knopKlant)==1) {
      // Zet de led aan
      digitalWrite(ledPin,true);
      // Start timer
      currentBuzzerTime=0;
    }
    // Kassa knop
    if (digitalRead(knopKassa)==1) {
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
    int val = analogRead(A3);
    float mv = (float)val/1024*5000;
    float celcius = mv/10;
    
    

    /*=========================*/
    /*     KLOK & SEGMENT      */
    /*=========================*/
    // Moeten we de klok updaten?
    if (currentKlokTime>=TimeModuleUpdate){
      // Ontvang huidige tijd van RTC module
      now = Rtc.GetDateTime();
      currentKlokTime=0;
    }
    // Moeten we het scherm updaten?
    if (currentScreenTime>=refreshTime){
      // Om de 16 ms
      if (afwisseling<2500) {
        toonUur(&now,refreshTime);
      } else {
        toonTemp(celcius,refreshTime);
      }
      currentScreenTime=0;
    }

    // we updaten de timer met 1 ms
    delay(1);
    int eindet = millis();
    // Tijdsverschil berekenen
    int delta = (eindet-begint);
    
    // Alle huidige tijden optellen
    currentKlokTime   += delta;
    currentScreenTime += delta;

    // Enkel optellen als de timer aan staat
    if (currentBuzzerTime>=0) {
      currentBuzzerTime+=delta;
    }

    afwisseling+=delta;
    if (afwisseling>=5000){
      afwisseling=0;
    }
  }
}
