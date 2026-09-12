#include <Arduino.h>
#include <avr/interrupt.h>
#include <string.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error Requires ATmega328P16MHz
#endif
// Original Hue DATA must stay isolated. D2 input until explicit D command.
// 80 units x10bytes +6byte captured footer +1safe preload sentinel.
static uint8_t packet[807];
static uint16_t current[2][5]={{0,0x25d6,0x7b54,0,0x043a},{0xa3d6,0x00ad,0,0x0234,0}};
static const uint8_t footer[6]={0x03,0x75,0xd7,0x5e,0x78,0x08};
__attribute__((noinline)) static void sendPacket(){
 uint8_t *cursor=packet;
 uint16_t remaining=806;
 const uint8_t lo=PORTD & ~_BV(PD2),hi=lo | _BV(PD2),savedSREG=SREG;
 cli();
 asm volatile(
    "ld r18, X+\n\t"
    "1:\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 7\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 6\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 5\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 4\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 3\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 2\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 1\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[hi]\n\t"
    "nop\n\t"
    "nop\n\t"
    "sbrs r18, 0\n\t"
    "out %[port], %[lo]\n\t"
    "ld r18, X+\n\t"
    "sbiw %[remaining], 1\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "out %[port], %[lo]\n\t"
    "nop\n\t"
    "breq 2f\n\t"
    "rjmp 1b\n\t"
    "2: nop\n\t"
 : "+x"(cursor), [remaining] "+w"(remaining)
 : [port] "I"(_SFR_IO_ADDR(PORTD)),[hi] "r"(hi),[lo] "r"(lo)
 : "r18","memory");
 SREG=savedSREG;
}
static void fillPacket(const uint16_t colors[2][5]){
 for(uint8_t half=0;half<2;half++){
  uint8_t word[10];
  for(uint8_t ch=0;ch<5;ch++){word[2*ch]=colors[half][ch]>>8;word[2*ch+1]=colors[half][ch]&255;}
  for(uint8_t unit=0;unit<40;unit++)memcpy(packet+(uint16_t(half)*40+unit)*10,word,10);
 }
 memcpy(packet+800,footer,6);packet[806]=0;
}
static void randomColor(uint16_t color[5]){
 const uint16_t hue=random(1536);
 const uint16_t v=uint32_t(hue%256)*30000/255;
 memset(color,0,10);
 switch(hue/256){
  case 0:color[0]=30000-v;color[1]=v;break;
  case 1:color[1]=30000-v;color[2]=v;break;
  case 2:color[2]=30000-v;color[0]=v;break;
  case 3:color[0]=30000-v;color[2]=v;break;
  case 4:color[2]=30000-v;color[1]=v;break;
  default:color[1]=30000-v;color[0]=v;break;
 }
}
static void demo(){
 digitalWrite(2,LOW);pinMode(2,OUTPUT);
 for(uint8_t stage=0;stage<12;stage++){
  uint16_t target[2][5];randomColor(target[0]);randomColor(target[1]);
  for(uint8_t step=0;step<=100;step++){
   // Smoothstep easing, integer scale1,000,000. No abrupt target jumps.
   const uint32_t t=step;
   const uint32_t eased=t*t*(300-2*t);
   uint16_t mixed[2][5];
   for(uint8_t h=0;h<2;h++)for(uint8_t c=0;c<5;c++){
    const int32_t delta=int32_t(target[h][c])-int32_t(current[h][c]);
    // Use64bits: delta * eased can exceed a32bit intermediate.
    mixed[h][c]=int32_t(current[h][c])+int64_t(delta)*eased/1000000L;
   }
   fillPacket(mixed);sendPacket();delay(13);
   if(Serial.available()){
    if(Serial.read()=='X'){
     memcpy(current,mixed,sizeof(current));digitalWrite(2,LOW);pinMode(2,INPUT);Serial.println(F("DEMO stopped; D2 INPUT"));return;
    }
   }
  }
  memcpy(current,target,sizeof(current));
 }
 digitalWrite(2,LOW);pinMode(2,INPUT);Serial.println(F("DEMO done; D2 INPUT"));
}

static bool stopped(){
 if(Serial.available() && Serial.read()=='X'){
  digitalWrite(2,LOW);pinMode(2,INPUT);Serial.println(F("DEMO stopped; D2 INPUT"));return true;
 }
 return false;
}
static void unitColor(uint8_t unit,const uint16_t color[5]){
 for(uint8_t c=0;c<5;c++){packet[uint16_t(unit)*10+c*2]=color[c]>>8;packet[uint16_t(unit)*10+c*2+1]=color[c]&255;}
}
static bool fadeTo(const uint16_t target[2][5],uint8_t steps){
 for(uint8_t step=0;step<=steps;step++){
  uint32_t t=uint32_t(step)*100/steps,e=t*t*(300-2*t);
  uint16_t mixed[2][5];
  for(uint8_t h=0;h<2;h++)for(uint8_t c=0;c<5;c++)mixed[h][c]=int32_t(current[h][c])+int64_t(int32_t(target[h][c])-current[h][c])*e/1000000L;
  fillPacket(mixed);sendPacket();delay(13);if(stopped())return false;
 }
 memcpy(current,target,sizeof(current));return true;
}
static void wheel(uint16_t hue,uint16_t gain,uint16_t color[5]){
 hue%=768;uint16_t v=uint32_t(hue%256)*gain/255;memset(color,0,10);
 if(hue<256){color[0]=gain-v;color[1]=v;}else if(hue<512){color[1]=gain-v;color[2]=v;}else{color[2]=gain-v;color[0]=v;}
}
static uint16_t envelope(uint16_t frame,uint16_t total,uint16_t maximum){
 uint16_t edge=min(frame,uint16_t(total-1-frame));if(edge>=30)return maximum;
 uint32_t t=uint32_t(edge)*100/30;return uint64_t(maximum)*t*t*(300-2*t)/1000000L;
}
static void whiteFunky(){
 digitalWrite(2,LOW);pinMode(2,OUTPUT);memset(current,0,sizeof(current));
 Serial.println(F("PHASE white and warm channels"));Serial.flush();
 for(uint8_t stage=0;stage<5;stage++){
  uint16_t target[2][5]={{0}};
  for(uint8_t h=0;h<2;h++){
   if(stage==0){target[h][3]=10000;target[h][4]=10000;}
   else if(stage==1)target[h][3]=24000;
   else if(stage==2)target[h][4]=24000;
   else target[h][3+((h+(stage==4))%2)]=24000;
  }
  if(!fadeTo(target,85))return;
 }
 const uint16_t dark[2][5]={{0}};if(!fadeTo(dark,35))return;
 Serial.println(F("PHASE moving rainbow"));Serial.flush();
 for(uint16_t f=0;f<240;f++){
  uint16_t gain=envelope(f,240,24000);
  for(uint8_t u=0;u<80;u++){uint16_t color[5];wheel(uint16_t(u)*768/80+f*5,gain,color);unitColor(u,color);}
  sendPacket();delay(13);if(stopped())return;
 }
 Serial.println(F("PHASE soft comet"));Serial.flush();
 for(uint16_t f=0;f<260;f++){
  int16_t head=f%192;head=(head<96?head:191-head)-8;
  uint16_t gain=envelope(f,260,24000);
  for(uint8_t u=0;u<80;u++){
   int16_t distance=abs(int16_t(u)-head);uint16_t color[5]={0};
   if(distance<10){uint16_t strength=uint32_t(gain)*(10-distance)*(10-distance)/100;color[0]=strength/3;color[2]=strength*2UL/3;}
   unitColor(u,color);
  }
  sendPacket();delay(13);if(stopped())return;
 }
 memset(current,0,sizeof(current));
 Serial.println(F("PHASE candle glow"));Serial.flush();
 for(uint8_t stage=0;stage<8;stage++){
  uint16_t target[2][5]={{0}};
  for(uint8_t h=0;h<2;h++){target[h][3]=random(14000,23000);target[h][0]=2500;target[h][1]=600;}
  if(!fadeTo(target,35))return;
 }
 digitalWrite(2,LOW);pinMode(2,INPUT);Serial.println(F("DEMO done; D2 INPUT"));
}

// Spatial effects act on the80 observed driver units, not individual LED dies.
static uint16_t triangle(uint16_t p){p%=512;return p<256?p:511-p;}
static uint16_t glow(int16_t distance,uint16_t radius,uint16_t gain){
 uint16_t d=abs(distance);if(d>=radius)return 0;
 return uint32_t(gain)*(radius-d)/radius;
}
static void playground(){
 digitalWrite(2,LOW);pinMode(2,OUTPUT);
 memcpy(packet+800,footer,6);packet[806]=0;
 for(uint8_t effect=0;effect<8;effect++){
  switch(effect){
   case 0:Serial.println(F("PHASE aurora curtains"));break;
   case 1:Serial.println(F("PHASE twin comet dance"));break;
   case 2:Serial.println(F("PHASE pond ripples"));break;
   case 3:Serial.println(F("PHASE bouncing marbles"));break;
   case 4:Serial.println(F("PHASE candy conveyor"));break;
   case 5:Serial.println(F("PHASE firefly garden"));break;
   case 6:Serial.println(F("PHASE sunrise wave"));break;
   default:Serial.println(F("PHASE liquid plasma"));break;
  }
  Serial.flush();
  for(uint16_t f=0;f<200;f++){
   const uint16_t gain=envelope(f,200,26000);
   const int16_t headA=uint32_t(triangle(f*5))*79*256/255;
   const int16_t headB=uint32_t(triangle(f*7+256))*79*256/255;
   for(uint8_t u=0;u<80;u++){
    uint16_t color[5]={0};int16_t pos=int16_t(u)*256;
    switch(effect){
     case 0:{
      uint16_t a=triangle(uint16_t(u)*9+f*3),b=triangle(uint16_t(u)*17+512-f*2%512);
      color[1]=uint32_t(gain)*a/510;color[2]=uint32_t(gain)*b/510;color[0]=uint32_t(gain)*(255-a)/1530;break;
     }
     case 1:{
      uint16_t a=glow(pos-headA,2304,gain/2),b=glow(pos-headB,2304,gain/2);
      color[0]=a;color[2]=b;color[4]=min(a,b);break;
     }
     case 2:{
      int16_t center=effect*13+19;
      uint16_t radius=(f*3)%160;
      uint16_t ring=glow(int16_t(abs(int16_t(u)-center)*2)-int16_t(radius),12,gain);
      color[1]=ring/2;color[2]=ring-ring/2;break;
     }
     case 3:{
      for(uint8_t k=0;k<5;k++){
       int16_t head=uint32_t(triangle(f*(k+2)+k*97))*79*256/255;
       uint16_t light=glow(pos-head,1536,gain/5),blob[5];wheel(k*151+f,light,blob);
       for(uint8_t c=0;c<3;c++)color[c]+=blob[c];
      }break;
     }
     case 4:{
      uint16_t p=(uint16_t(u)*32+f*7)%512,blend=triangle(p);
      color[0]=uint32_t(gain)*blend/510;color[2]=uint32_t(gain)*(255-blend)/510;color[4]=gain/3;break;
     }
     case 5:{
      uint16_t seed=(uint16_t(u)*137+43)%512;
      uint16_t pulse=triangle(seed+f*(2+u%3));
      uint16_t light=pulse>195?uint32_t(gain)*(pulse-195)/60:0;
      color[0]=light/2;color[1]=light/3;color[3]=light/6;break;
     }
     case 6:{
      uint16_t sun=triangle(uint16_t(u)*3+f*2);
      color[0]=uint32_t(gain)*(255-sun)/765;color[3]=uint32_t(gain)*sun/510;color[4]=uint32_t(gain)*sun/1530;break;
     }
     default:{
      uint16_t a=triangle(uint16_t(u)*11+f*3),b=triangle(uint16_t(u)*5+512-f*2%512);
      wheel(a+b+f*2,gain,color);break;
     }
    }
    // Conservative aggregate command amplitude, independent of channel count.
    uint32_t total=0;for(uint8_t c=0;c<5;c++)total+=color[c];
    if(total>28000)for(uint8_t c=0;c<5;c++)color[c]=uint32_t(color[c])*28000/total;
    unitColor(u,color);
   }
   sendPacket();delay(13);if(stopped())return;
  }
 }
 // Finish by gently returning to a warm glow rather than leaving the strip dark.
 memset(current,0,sizeof(current));uint16_t rest[2][5]={{0}};rest[0][3]=16000;rest[1][3]=16000;
 if(!fadeTo(rest,65))return;
 digitalWrite(2,LOW);pinMode(2,INPUT);Serial.println(F("DEMO done; D2 INPUT"));
}
void setup(){pinMode(2,INPUT);digitalWrite(2,LOW);Serial.begin(115200);randomSeed(micros());Serial.println(F("OMNIGLOW_PLAYGROUND INPUT; D=colors W=white P=play X=stop"));}
void loop(){if(Serial.available()){int c=Serial.read();if(c=='D'||c=='W'||c=='P'){Serial.println(F("DEMO begin"));Serial.flush();if(c=='P')playground();else if(c=='W')whiteFunky();else demo();}}}
