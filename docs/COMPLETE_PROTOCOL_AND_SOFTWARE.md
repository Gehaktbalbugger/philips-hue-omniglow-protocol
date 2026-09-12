# Philips Hue OmniGlow — protocol en volledige software

Dit document bevat het gemeten werkmodel en de volledige nieuwste Arduino-software. Het protocolgedeelte is Engelstalig zodat het onderzoek ook internationaal bruikbaar is. De losse bestanden en gebruiksinstructies staan in deze repository.

Bevestigd door de gebruiker: rood/blauw en twee verschillend gekleurde helften op de intacte strip. De extra effecten zijn verstuurd; exacte witkanaalvolgorde, een afgeknipt deel en Hue Bridge-koppeling zijn nog niet bewezen.

## Protocol record

**Status:** independent reverse engineering of one user-owned strip, recorded 2026-09-12. This document is a measured working description, not an official Philips specification.

## Evidence and confidence

| Statement | Status | Evidence or limit |
|---|---|---|
| Captured symbol period is approximately1.25µs | Measured at187.5ns sample spacing | Clean red/blue captures; original clock not independently calibrated |
| Generated high intervals are4 and15cycles; generated slot is20cycles | Implemented and instruction-checked | Accepted red/blue replay; original pulse tolerances remain unknown |
| An active frame is 6,448 symbols, or 806 bytes | Measured for the working replay | 80-symbol repetition and the 48-symbol tail; first symbol is inferred from alignment |
| The first 6,400 symbols are an 80-symbol pattern repeated 80 times | Measured pattern structure | Exact repetition in the clean red and blue snapshot sets |
| The final 48 symbols are a common footer | Measured pattern structure | Same tail in the red and blue message-part captures |
| The footer is the bytes `03 75 D7 5E 78 08` | Measured in the working byte representation | A checksum or CRC meaning is not proved |
| 80 units × 10 bytes × five 16-bit values | Inferred wire model | Fits the 806-byte frame and successful generated tests; unit meaning is unknown |
| Values are sent MSB first | Inferred/implemented convention | Used by the working serializer; channel semantics remain unconfirmed |
| Red/blue channel mapping | Inferred from user-labelled captures | Green has not been independently measured; white/CCT order is unconfirmed |
| Full red, full blue, 40/40 red-blue, and swapped 40/40 | User-confirmed behavior | Confirmed on the tested intact strip after the original DATA line was isolated |
| A universal 80-pixel or LED count | Unsupported | “80 units” is a frame grouping, not a confirmed physical LED count |
| Controller IC or official protocol identity | Unknown | No chip identification or Philips specification has been established |

Published evidence: [red aligned captures](../evidence/red-aligned-captures.json), [blue aligned captures](../evidence/blue-aligned-captures.json), [red reconstruction](../evidence/red-reconstruction.json), [blue reconstruction](../evidence/blue-reconstruction.json), and [compiled waveform checks](../evidence/waveform-verification.json). Each aligned window samples1024logic states at187.5ns spacing. Windows at different offsets belong to different transmissions; reconstruction relies on stable repeated data and checks overlapping symbols for conflicts. It is not a single continuous8ms capture.

## Electrical notes

The test setup used:

- an Arduino Uno / ATmega328P at 16 MHz;
- nominal 5 V Arduino logic;
- a strip powered from its nominal 24 V supply;
- a common ground between the tested signal source and strip;
- D2 as the Arduino DATA pin in the verified harness.

The strip-side input threshold, protection, isolation, and any level-shifter requirement were not measured. Keep 24 V away from Arduino signal pins. The original Hue controller DATA connection must be physically isolated before Arduino output is connected or a replay command is sent.

Wire colours are not a universal pinout. Verify DATA, GND and supply on the actual device.

## Captured timing and accepted replay timing

At 16 MHz, one CPU cycle is 62.5 ns. The compiled output and waveform reconstruction use:

| Quantity | Cycles | Time |
|---|---:|---:|
| Symbol slot | 20 | 1.25 µs |
| Short high | 4 | 250 ns |
| Long high | 15 | 937.5 ns |
| Working frame | 6,448 symbols | 8.06 ms |
| Repeated body | 6,400 symbols | 8.00 ms |
| Footer | 48 symbols | 60 µs |

A short high pulse is labeled `0` and a long high pulse is labeled `1` in analysis files. That is a measurement label, not proof of the device’s logical bit polarity.

The coarse envelope capture measured complete inter-frame low gaps from approximately 12.889 ms to 14.304 ms. The generated Arduino tests use `delay(13)` after each frame; the generated low interval additionally includes software rendering overhead. The captured original-controller gap is a separate measurement. A measured start-to-start period was approximately 20.8–22.1 ms in the coarse run. These are practical test timings, not a claimed timing tolerance for every strip.

## Working frame model

The current serializer writes 806 transmitted bytes:

```text
bytes 0..799   80 repeated units × 10 bytes
bytes 800..805 six-byte footer: 03 75 D7 5E 78 08
```

Each ten-byte unit is represented as five big-endian 16-bit values:

```text
value 0: byte 0, byte 1
value 1: byte 2, byte 3
value 2: byte 4, byte 5
value 3: byte 6, byte 7
value 4: byte 8, byte 9
```

This is a working representation. It should not be read as a confirmed Philips field map. The 80-unit grouping could describe receiver words or an internal packet layout rather than physical LEDs. The tested controller IC is unknown, and the captures do not establish a general LED count, address format, brightness law, checksum, or colour-space conversion.

The serializer keeps one extra byte as a non-transmitted sentinel so the final byte preload remains inside the buffer. The sentinel is not part of the frame.

## Observed values

Using the short-high/long-high convention and the inferred five-word grouping, the clean user-labelled captures produced these representative values:

| Capture label | Value 0 | Value 1 | Value 2 | Value 3 | Value 4 |
|---|---:|---:|---:|---:|---:|
| Red | `A3D6` | `00AD` | `0000` | `0234` | `0000` |
| Blue | `0000` | `25D6` | `7B54` | `0000` | `043A` |

An older blue capture differed in shade, so these values are examples from specific user-labelled states rather than fixed colour constants. Different blue selections yielded different payloads; no universal blue constant or calibrated color transform has been established.

The final 48 symbols were stable across the red and blue message-part captures. They are encoded in the working serializer as `03 75 D7 5E 78 08`. No evidence currently proves that those six bytes are a CRC, terminator, brightness field, or any other named protocol field.

## Generated colour limits

The smooth demo uses only the first three inferred channels for generated RGB-like targets. Each target channel is bounded to 30,000, the three-channel sum is 30,000, and the remaining two channels are zero. The initial measured states contain larger individual values, including approximately 41,942 for the red-labelled state, 31,572 for blue, and 9,686 for blue’s other active channel; interpolation stays between the current state and the bounded target.

The transition uses integer smoothstep interpolation over 101 points from each current state to a new target. The intermediate multiplication uses 64-bit arithmetic. This avoids arithmetic overflow in the easing calculation but does not prove that the inferred channel model is physically correct.

## Firmware behaviour

The current playground is intended for an Uno at 16 MHz. It starts with D2 as an input, waits for an explicit serial command at 115200 baud, writes a bounded sequence of frames, and returns D2 to input with a low output latch after stopping or completing.

The public command summary is:

| Command | Action |
|---|---|
| `D` | 12-stage smooth random-colour demo; each stage uses 101 interpolation steps and independent targets for the two 40-unit halves |
| `W` | White-channel experiment |
| `P` | Eight-effect playground |
| `X` | Stop between frames and release D2 to input |

The `D` demo checks for `X` between frames. The signal output itself remains interrupt-free for the fixed 8.06 ms frame, then the serial stop check runs during the low interval. A command is not an authorization to connect the original controller DATA line; that physical isolation must be completed first.

## Reproduction and review limits

The replay implementation was checked from compiled assembly, including:

- 6,448 symbols per frame;
- 20-cycle spacing for every adjacent symbol;
- 4/15-cycle high durations;
- correct all-zero, all-one, random, red, and blue payload simulations;
- safe final-byte sentinel preload;
- low final output level;
- output masking limited to D2.

These checks establish that the generated firmware emits the intended working waveform. They do not identify the proprietary protocol or guarantee operation on another Hue product, another strip revision, a different power supply, or an untested electrical interface.

## Next verification

A future single ESP32-C6 Zigbee prototype should be paired with a real Hue Bridge and checked for color, dimming, color temperature and power-cycle recovery. Only then should a second custom device be built. This is integration work, not driver-chip identification. Do not turn the 80-unit inference into a universal Philips Hue or WS2812-style protocol claim.

## Reference material

- [Espressif Zigbee color and color-temperature light endpoint](https://docs.espressif.com/projects/arduino-esp32/en/latest/zigbee/ep_color_dimmable_light.html): candidate future wireless interface; not implemented here.
- [Philips Hue Zigbee interoperability](https://developers.meethue.com/zigbee-3-0-support-in-hue-ecosystem/): custom-device pairing remains a hardware test.
- [Current full sketch](../firmware/omniglow_playground/omniglow_playground.ino) and [serial control tool](../tools/control.py).


# Volledige huidige Arduino-software

Bestand: `firmware/omniglow_playground/omniglow_playground.ino`

SHA-256 van deze broncode: `fe4b67d8e28485d955eb168c543d377af2494c73b71df11682758bb9b8a06f7c`

```cpp
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

```
