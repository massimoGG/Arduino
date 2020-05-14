static unsigned char cijfers[] {
  252,96,218,242,102,182,190,224,254,246
};// 0 1 2 3 4 5 6 7 8 9

static unsigned char letters[] {
  238,62,156,122,158,142,110,28,252
};// A b C D      E F H E  L   O

int RCLKPin = 3;
int SRCLKPin = 4;
int dataPin = 2;
int d[] = {5,6,7,8};

void setup() {
  Serial.begin(115200);
  pinMode(RCLKPin, OUTPUT);
  pinMode(SRCLKPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  for (int i=0;i<sizeof(d);i++){
    pinMode(d[i],OUTPUT);
    digitalWrite(d[i],true);
  }
}

void toonUur(int uur, int minuut){
  // Show point
}

void toonCijfer(unsigned char cijfer, unsigned char digidisplay, int duration){
  digitalWrite(digidisplay,false);
  digitalWrite(RCLKPin,LOW);
  shiftOut(dataPin,SRCLKPin,LSBFIRST,cijfer);
  digitalWrite(RCLKPin,HIGH);
  delay(duration);
  digitalWrite(digidisplay,true);
}

void toonGetal(int cijfer, int duration){
  //1234
  unsigned char duizendtal= cijfer/1000; // Afronden?
  // 1.234 -> 1
  unsigned char honderdtal = (cijfer-duizendtal*1000)/100;
  // 1234-1000 = 234/100 -> 2
  unsigned char tiental   = (cijfer-duizendtal*1000-honderdtal*100)/10;
  // 1234-1000-200 = 34/10 -> 3
  unsigned char eenheid   = (cijfer-duizendtal*1000-honderdtal*100-tiental*10);
  toonCijfer(cijfers[duizendtal],d[0],duration);
  toonCijfer(cijfers[honderdtal],d[1],duration);
  toonCijfer(cijfers[tiental],d[2],duration);
  toonCijfer(cijfers[eenheid],d[3],duration);
}

void loop() {
  while (true) {
    toonGetal(1234,100);
  }
}
