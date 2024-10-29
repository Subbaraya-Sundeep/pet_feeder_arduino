#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Servo.h>

LiquidCrystal lcd(8, 9, 13, 12, 11, 10);
volatile int sw_delay;

typedef void (*func_t)(void);

extern func_t fptrs[];
extern int STATE;
extern volatile int KEY_INTR;
extern volatile int KEY;
extern struct config the_config;
void load_config(void);
void print_config(void);
void config_alarm(void);
void handle_alarm(void);

extern volatile int last_state;

Servo myservo1;
const byte interruptPin = 19;

#define BUTTON_CHECK_INTERVAL	200000	//us

void setup() {
	Serial.begin(9600);
	Wire.begin();
	lcd.begin(16, 2);
  lcd.print("DD:MM:YY DD:MM:YY");
	myservo1.attach(46);
	myservo1.write(98);

	pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  digitalWrite(A3, LOW);

  load_config();
  print_config();
  handle_alarm();

  pinMode(19, INPUT);
//  attachInterrupt(4, alarm_isr, FALLING);

	Timer1.initialize(BUTTON_CHECK_INTERVAL);
	Timer1.attachInterrupt(timerISR);
}

void loop() {
	Serial.print("LOOP..calling STATE");
	Serial.println(STATE, DEC);
  KEY_INTR = 0;

	fptrs[STATE]();
}

void timerISR(void)
{
	int sw;

	for (sw = 3; sw < 8; sw++) {
		if (digitalRead(sw) == LOW) {
			for (sw_delay = 0; sw_delay < 10000; sw_delay++);
			for (sw_delay = 0; sw_delay < 10000; sw_delay++);
			for (sw_delay = 0; sw_delay < 10000; sw_delay++);
			if (digitalRead(sw) == LOW) {
				KEY = sw;
				KEY_INTR = 1;
        digitalWrite(2, HIGH);
        for (sw_delay = 0; sw_delay < 10000; sw_delay++);
        for (sw_delay = 0; sw_delay < 10000; sw_delay++);
        digitalWrite(2, LOW); 
      }
    }
  }
}

void alarm_isr(void)
{
      last_state = STATE;
      KEY = 15;
      KEY_INTR = 1;
      Serial.println("Alarm ISR");    
}
