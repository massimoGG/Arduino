#include <ThreeWire.h>  
#include <RtcDS1302.h>

// Configuratie
const int TimeModuleUpdate= 10000; // every 10 seconds
const int refreshTime = 4; // 4 ms

int RCLKPin = 3;
int SRCLKPin = 4;
int dataPin = 2;
int d[] = {5,6,7,8};
ThreeWire myWire(10,9,11); // IO, SCLK, CE

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
  toonCijfer(cijfers[u1],d[0],duration);
  toonCijfer(cijfers[u2]|0x01,d[1],duration);
  toonCijfer(cijfers[m1],d[2],duration);
  toonCijfer(cijfers[m2],d[3],duration);
}

void toonGetal(int cijfer, int duration){
  unsigned char duizendtal= cijfer/1000;
  unsigned char honderdtal = (cijfer-duizendtal*1000)/100;
  unsigned char tiental   = (cijfer-duizendtal*1000-honderdtal*100)/10;
  unsigned char eenheid   = (cijfer-duizendtal*1000-honderdtal*100-tiental*10);
  toonCijfer(cijfers[duizendtal],d[0],duration);
  toonCijfer(cijfers[honderdtal],d[1],duration);
  toonCijfer(cijfers[tiental],d[2],duration);
  toonCijfer(cijfers[eenheid],d[3],duration);
}

void loop() {
  int currentTime = 0;
  RtcDateTime now = Rtc.GetDateTime();
  while (currentTime<TimeModuleUpdate) {
    toonUur(&now,refreshTime);
    currentTime+=refreshTime*4;
  }
}
