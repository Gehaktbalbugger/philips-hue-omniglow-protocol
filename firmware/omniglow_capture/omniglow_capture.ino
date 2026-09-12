#include <Arduino.h>
#include <avr/interrupt.h>

#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error This capture is cycle-counted for an ATmega328P running at 16 MHz only.
#endif

// Read-only probe on D2 / PD2. No pull-up, no signal output.
// A command captures 1024 samples, one every 3 CPU cycles = 187.5 ns.
// This is a short snapshot, not continuous acquisition or a protocol decoder.
static uint8_t samples[1024];

__attribute__((noinline)) static void capture() {
  uint8_t *cursor = samples;
  const uint8_t savedSREG = SREG;
  cli();
  asm volatile(
    ".rept 1024\n\t"
    "in r18, %[port]\n\t"
    "st X+, r18\n\t"
    ".endr\n\t"
    : "+x" (cursor)
    : [port] "I" (_SFR_IO_ADDR(PIND))
    : "r18", "memory"
  );
  SREG = savedSREG;
}

// Coarse burst-envelope capture: 503 cycles between samples =31.4375us.
// This intentionally undersamples individual data bits; use only to find gaps.
__attribute__((noinline)) static void captureEnvelope() {
  uint8_t *cursor = samples;
  const uint8_t savedSREG = SREG;
  cli();
  asm volatile(
    "ldi r24, 0\n\t"
    "ldi r25, 4\n\t"
    "1: in r18, %[port]\n\t"
    "st X+, r18\n\t"
    "ldi r19, 165\n\t"
    "2: dec r19\n\t"
    "brne 2b\n\t"
    "nop\n\t"
    "sbiw r24, 1\n\t"
    "brne 1b\n\t"
    : "+x" (cursor)
    : [port] "I" (_SFR_IO_ADDR(PIND))
    : "r18", "r19", "r24", "r25", "memory"
  );
  SREG = savedSREG;
}

void setup() {
  pinMode(2, INPUT);
  digitalWrite(2, LOW); // explicitly disable pull-up; remains an input
  Serial.begin(115200);
  Serial.println(F("HUE_CAPTURE_V1 D2 INPUT 16MHz 3cycles 1024samples"));
}

#define SAMPLE_1 "in r18, %[port]\n\tst X+, r18\n\t"
#define SAMPLE_2 SAMPLE_1 SAMPLE_1
#define SAMPLE_4 SAMPLE_2 SAMPLE_2
#define SAMPLE_8 SAMPLE_4 SAMPLE_4
#define SAMPLE_16 SAMPLE_8 SAMPLE_8
#define SAMPLE_32 SAMPLE_16 SAMPLE_16
#define SAMPLE_64 SAMPLE_32 SAMPLE_32
#define SAMPLE_128 SAMPLE_64 SAMPLE_64
#define SAMPLE_256 SAMPLE_128 SAMPLE_128
#define SAMPLE_512 SAMPLE_256 SAMPLE_256
#define SAMPLE_1024 SAMPLE_512 SAMPLE_512

// Capture relative to the first latched rising edge after >=1ms without edges.
// Timer1 is owned by this firmware. INT0 is latched but its ISR stays disabled.
// Offset units are four CPU cycles (250ns); trigger latency remains several cycles.
__attribute__((noinline)) static bool captureAligned(uint16_t offset) {
  uint8_t *cursor = samples;
  const uint8_t savedSREG = SREG;
  const uint8_t savedEICRA = EICRA;
  const uint8_t savedEIMSK = EIMSK;
  cli();
  EIMSK &= ~_BV(INT0);
  EICRA = (EICRA & ~(_BV(ISC01) | _BV(ISC00))) | _BV(ISC01) | _BV(ISC00);
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 60000; // 30ms timeout at 0.5us per tick.
  TIFR1 = _BV(OCF1A) | _BV(TOV1);
  EIFR = _BV(INTF0);
  TCCR1B = _BV(CS11);
  uint16_t lastEdge = 0;
  bool gap = false;
  while (!(TIFR1 & _BV(OCF1A))) {
    uint16_t now = TCNT1;
    if (EIFR & _BV(INTF0)) {
      EIFR = _BV(INTF0);
      lastEdge = now;
    } else if (uint16_t(now - lastEdge) >= 2000) {
      gap = true;
      break;
    }
  }
  uint8_t matched = 0;
  if (gap) {
    asm volatile(
      "ldi %[matched], 1\n\t"
      "1: sbis %[edgeport], %[edgebit]\n\t"
      "rjmp 2f\n\t"
      "rjmp 3f\n\t"
      "2: sbis %[timerport], %[timerbit]\n\t"
      "rjmp 1b\n\t"
      "ldi %[matched], 0\n\t"
      "jmp 5f\n\t"
      "3: adiw %[offset], 0\n\t"
      "breq 6f\n\t"
      "4: sbiw %[offset], 1\n\t"
      "brne 4b\n\t"
      "6:\n\t"
      SAMPLE_1024
      "5:\n\t"
      : "+x" (cursor), [offset] "+w" (offset), [matched] "=&d" (matched)
      : [port] "I" (_SFR_IO_ADDR(PIND)),
        [edgeport] "I" (_SFR_IO_ADDR(EIFR)), [edgebit] "I" (INTF0),
        [timerport] "I" (_SFR_IO_ADDR(TIFR1)), [timerbit] "I" (OCF1A)
      : "r18", "memory"
    );
  }
  TCCR1B = 0;
  EICRA = savedEICRA;
  EIFR = _BV(INTF0);
  EIMSK = savedEIMSK;
  SREG = savedSREG;
  return matched != 0;
}

void loop() {
  if (!Serial.available()) return;
  const int command = Serial.read();
  if (command == 'A') {
    uint8_t offsetBytes[2];
    if (Serial.readBytes(offsetBytes, 2) != 2) { Serial.println(F("ERROR offset")); return; }
    uint16_t offset = uint16_t(offsetBytes[0]) | (uint16_t(offsetBytes[1]) << 8);
    if (offset > 36000) { Serial.println(F("ERROR offset range")); return; }
    Serial.flush();
    if (!captureAligned(offset)) { Serial.println(F("TIMEOUT")); return; }
    Serial.println(F("CAPTURE_V1 1024 3 16000000 2"));
    Serial.write(samples, sizeof(samples));
    Serial.println();
    Serial.println(F("END"));
  } else if (command == 'C' || command == 'T' || command == 'G') {
    Serial.flush();
    if (command == 'T') {
      const uint8_t initial = PIND & _BV(PD2);
      const unsigned long start = millis();
      bool changed = false;
      while (!changed && (unsigned long)(millis() - start) < 1000UL) {
        for (uint8_t i = 0; i < 128; ++i) {
          if ((PIND & _BV(PD2)) != initial) { changed = true; break; }
        }
      }
    }
    if (command == 'G') {
      captureEnvelope();
      Serial.println(F("CAPTURE_V1 1024 503 16000000 2"));
    } else {
      capture();
      Serial.println(F("CAPTURE_V1 1024 3 16000000 2"));
    }
    Serial.write(samples, sizeof(samples));
    Serial.println();
    Serial.println(F("END"));
  } else if (command == '?') {
    Serial.println(F("HUE_CAPTURE_V1 D2 INPUT 16MHz 3cycles 1024samples"));
  }
}
