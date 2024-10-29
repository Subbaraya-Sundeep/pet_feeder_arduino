#include <DS3231.h>
#include <LiquidCrystal.h>
#include<Servo.h>
#include <EEPROM.h>

extern LiquidCrystal lcd;

DS3231 Clock;
bool Century=false;
bool h12;
bool PM;
byte ADay, AHour, AMinute, ASecond, ABits;
bool ADy, A12h, Apm;

#define KEY_LEFT    3
#define KEY_RIGHT   7
#define KEY_SELECT  5
#define KEY_UP      6
#define KEY_DOWN    4
#define KEY_ALARM   15
#define BUZZER      2
#define ALARM       19

#define SERVO_ZERO  98
#define SERVO_FEED  0

typedef void (*func_t)(void);

int STATE;
volatile int last_state;
volatile int KEY_INTR = 0;
volatile int KEY;
int interval = 20;
int angle = 70;
extern Servo myservo1;

struct config {
	int interval;
	byte a1_hour;
	byte a1_min;
	byte a1_sec;
	byte a2_hour;
	byte a2_min;
	byte a2_sec;
	byte a3_hour;
	byte a3_min;
	byte a3_sec;
  byte a1_en;
  byte a2_en;
  byte a3_en;
};

struct config the_config;

void config_alarm(void)
{
  int mins = Clock.getHour(h12, PM) * 60 + Clock.getMinute();
  int a1_mins = the_config.a1_hour * 60 + the_config.a1_min;
  int a2_mins = the_config.a2_hour * 60 + the_config.a2_min;
  int a3_mins = the_config.a3_hour * 60 + the_config.a3_min;
  byte hh, mm, ss;
  byte match_hh_mm = 0b01000000;
  
  /* Set next occuring Alarm */
  if (mins >= a1_mins && mins < a2_mins) {
      hh = the_config.a2_hour;
      mm = the_config.a2_min;
  } else if (mins >= a2_mins && mins < a3_mins) {
    hh = the_config.a3_hour;
    mm = the_config.a3_min;
  } else if (mins < a1_mins || mins >= a3_mins){
    hh = the_config.a1_hour;
    mm = the_config.a1_min;
  }

  Clock.setA2Time(0, hh, mm, match_hh_mm, false, false, false);
  Clock.turnOnAlarm(2);
}

void handle_alarm(void)
{
  int mins = Clock.getHour(h12, PM) * 60 + Clock.getMinute();
  int a1_mins = the_config.a1_hour * 60 + the_config.a1_min;
  int a2_mins = the_config.a2_hour * 60 + the_config.a2_min;
  int a3_mins = the_config.a3_hour * 60 + the_config.a3_min;
  byte hh, mm, ss;
  bool en;

  if (Clock.checkIfAlarm(2)) {
    Serial.print(" A2!");
  } else {
    KEY_INTR = 0;
    STATE = last_state;
    return;
  }
  
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  digitalWrite(A4, LOW);
  
  /* Check which Alarm occured*/
  if (mins >= a1_mins && mins < a2_mins) {
    en = the_config.a1_en;
    if (the_config.a1_en)
      digitalWrite(A2, HIGH);
  } else if (mins >= a2_mins && mins < a3_mins) {
    en = the_config.a2_en;
    if (the_config.a2_en)
      digitalWrite(A3, HIGH);
  } else if (mins < a1_mins || mins >= a3_mins){
    en = the_config.a3_en;
    if (the_config.a3_en)
      digitalWrite(A4, HIGH);
  }

  config_alarm();
  
  if (en == false)
      goto exit;

  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW); 
 
  myservo1.write(SERVO_FEED);
  delay(50 + the_config.interval);
  myservo1.write(SERVO_ZERO);
  delay(50);
    
exit:
  KEY_INTR = 0;
  STATE = last_state;
  return;
}

void save_config(void)
{
  /* temporary fix for disabling A2 */
   the_config.a1_en = true;
   the_config.a2_en = false;
   the_config.a3_en = true;

	 EEPROM.put(0, the_config);	
}

void load_config(void)
{
	 EEPROM.get(0, the_config);
   interval = the_config.interval;
}

void print_config(void)
{
  Serial.print(the_config.a1_hour, DEC);
  Serial.print(the_config.a1_min, DEC);
  Serial.print(the_config.a1_sec, DEC);
  Serial.print(the_config.a2_hour, DEC);
  Serial.print(the_config.a2_min, DEC);
  Serial.print(the_config.a2_sec, DEC);
  Serial.print(the_config.a3_hour, DEC);
  Serial.print(the_config.a3_min, DEC);
  Serial.print(the_config.a3_sec, DEC);
  Serial.print(the_config.interval, DEC);
  Serial.print(the_config.a1_en, DEC);
  Serial.print(the_config.a2_en, DEC);
  Serial.print(the_config.a3_en, DEC);
  Serial.println("");
}

static void home_page(void)
{
	lcd.clear();
	lcd.setCursor(2, 0);
	lcd.print("< PET FEEDER >");

	while (1) {
		if (!KEY_INTR)
			continue;
		KEY_INTR = 0;

		switch (KEY) {
    		case KEY_LEFT:
			STATE = 11;
			return;
    		case KEY_RIGHT:
			STATE = 1;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_1(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SHOW DATE >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 6;
			return;
		case KEY_RIGHT:
			STATE = 2;
			return;
		case KEY_LEFT:
			STATE = 0;
			return;
		case KEY_UP:
			STATE = 0;
			return;
    case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_2(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SHOW TIME >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 7;
			return;
		case KEY_RIGHT:
			STATE = 3;
			return;
		case KEY_LEFT:
			STATE = 1;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_3(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SHOW ALARM1 >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 8;
			return;
		case KEY_LEFT:
			STATE = 2;
			return;
		case KEY_RIGHT:
			STATE = 4;
			return;
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_4(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SHOW ALARM2 >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 9;
			return;
		case KEY_LEFT:
			STATE = 3;
			return;
		case KEY_RIGHT:
			STATE = 5;
			return;
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_5(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SHOW ALARM3 ");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 10;
			return;
		case KEY_LEFT:
			STATE = 4;
			return;
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void show_date(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Date ");

	lcd.print(Clock.getDate(), DEC);
	lcd.print("/");
  
	lcd.print(Clock.getMonth(Century), DEC);
	lcd.print("/");

	lcd.print("2");
	if (Century) {      // Won't need this for 89 years.
		lcd.print("1");
	} else {
		lcd.print("0");
	}

	lcd.print(Clock.getYear(), DEC);

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void show_time(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Time: ");
	lcd.print(Clock.getHour(h12, PM), DEC);
	lcd.print(':');
	lcd.print(Clock.getMinute(), DEC);
	lcd.print(':');
	lcd.print(Clock.getSecond(), DEC);

	while (1) {
		if (!KEY_INTR) {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Time: ");
			lcd.print(Clock.getHour(h12, PM), DEC);
			lcd.print(':');
			lcd.print(Clock.getMinute(), DEC);
			lcd.print(':');
			lcd.print(Clock.getSecond(), DEC);
			delay(500);
			continue;
		}
		switch (KEY) {
		case KEY_UP:
			STATE = 0;
			return;
    case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void show_alarm1(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Alarm1: ");
	lcd.print(the_config.a1_hour, DEC);
	lcd.print(':');
	lcd.print(the_config.a1_min, DEC);
	lcd.print(':');
	lcd.print(the_config.a1_sec, DEC);

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void show_alarm2(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Alarm2: ");
	lcd.print(the_config.a2_hour, DEC);
	lcd.print(':');
	lcd.print(the_config.a2_min, DEC);
	lcd.print(':');
	lcd.print(the_config.a2_sec, DEC);

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_UP:
			STATE = 0;
			return;
    case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void show_alarm3(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Alarm3: ");
	lcd.print(the_config.a3_hour, DEC);
	lcd.print(':');
	lcd.print(the_config.a3_min, DEC);
	lcd.print(':');
	lcd.print(the_config.a3_sec, DEC);

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_11(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SET DATE >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 18;
			return;
		case KEY_RIGHT:
			STATE = 0;
			return;
		case KEY_LEFT:
			STATE = 12;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_12(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SET TIME >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 19;
			return;
		case KEY_RIGHT:
			STATE = 11;
			return;
		case KEY_LEFT:
			STATE = 13;
			return;
		case KEY_UP:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_13(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SET ALARM1 >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 20;
			return;
		case KEY_LEFT:
			STATE = 14;
			return;
		case KEY_RIGHT:
			STATE = 12;
			return;
		case KEY_UP:
			STATE = 0;
			return;
    case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_14(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SET ALARM2 >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 21;
			return;
		case KEY_LEFT:
			STATE = 15;
			return;
		case KEY_RIGHT:
			STATE = 13;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_15(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print("< SET ALARM3 >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 22;
			return;
		case KEY_LEFT:
			STATE = 16;
			return;
		case KEY_RIGHT:
			STATE = 14;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_16(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("< Config Feeder >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 23;
			return;
		case KEY_LEFT:
			STATE = 17;
			return;
		case KEY_RIGHT:
			STATE = 15;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

static void state_17(void)
{
	lcd.clear();
	lcd.setCursor(1, 0);
	lcd.print(" Feed Now >");

	while (1) {
		if (!KEY_INTR)
			continue;
		switch (KEY) {
		case KEY_SELECT:
			STATE = 24;
			return;
		case KEY_RIGHT:
			STATE = 16;
			return;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break; 		
		};
	}
}

#define TIME    1
#define DATE    2
#define ALRM    3

static void lcd_ui(char *title, char *initial, byte high1, byte high2, byte high3, byte low1, byte low2, byte low3,
                      byte type, byte alarm)
{
  int row, col;
  byte digit = 0;
  byte first, second, third;
  byte d1, d2, d3, d4, d5, d6;

  d1 = d2 = d3 = d4 = d5 = d6 = 0;

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(title);
  lcd.setCursor(2, 1);
  lcd.print(initial);

  row = 1;
  col = 2;

  while (1) { 
    if (!KEY_INTR)
      continue;
    KEY_INTR = 0;

    switch (KEY) {
    case KEY_UP:
      if (col == 2) {
        digit = d1;
        digit++;
        if ((digit * 10 + d2) > high1)
          break;
        d1 = digit;
      }
      else if (col == 3) {
        digit = d2;
        digit++;
        if ((d1 * 10 + digit) > high1)
          break;
        d2 = digit;
      }
      else if (col == 5) {
        digit = d3;
        digit++;
        if ((digit * 10 + d4) > high2)
          break;
        d3 = digit;
      }
      else if (col == 6) {
        digit = d4;
        digit++;
        if ((d3 * 10 + digit) > high2)
          break;
        d4 = digit;
      }
      else if (col == 8) {
        digit = d5;
        digit++;
        if ((digit * 10 + d6) > high3)
          break;
        d5 = digit;
      }
      else if (col == 9) {
        digit = d6;
        digit++;
        if ((d5 * 10 + digit) > high3)
          break;
        d6 = digit;
      } else {
        break;
      }    
      lcd.setCursor(col, row);    
      lcd.print(digit, DEC);
      break;
    case KEY_DOWN:
      if (col == 2) {
        digit = d1;
        digit--;
        if ((digit * 10 + d2) < low1)
          break;
        d1 = digit;
      }
      else if (col == 3) {
        digit = d2;
        digit--;
        if ((d1 * 10 + digit) < low1)
          break;
        d2 = digit;
      }
      else if (col == 5) {
        digit = d3;
        digit--;
        if ((digit * 10 + d4) < low2)
          break;
        d3 = digit;
      }
      else if (col == 6) {
        digit = d4;
        digit--;
        if ((d3 * 10 + digit) < low2)
          break;
        d4 = digit;
      }
      else if (col == 8) {
        digit = d5;
        digit--;
        if ((digit * 10 + d6) < low3)
          break;
        d5 = digit;
      }
      else if (col == 9) {
        digit = d6;
        digit--;
        if ((d5 * 10 + digit) < low3)
          break;
        d6 = digit;
      } else {
        break;
      }
      lcd.setCursor(col, row);
      lcd.print(digit, DEC);
      break;
    case KEY_RIGHT:
      col++;
      if (col == 10) {
        col = 9;
        break;
      }
      break;
    case KEY_LEFT:
      col --;
      if (col == 1) {
        col = 2;
        break;
      }
      break;
    case KEY_SELECT:
      STATE = 0;
 
      first = (d1 * 10) + d2;
      second = (d3 * 10) + d4;
      third = (d5 * 10) + d6;
 
      switch (type) {
      case TIME:
          Clock.setHour(first);
          Clock.setMinute(second);
          Clock.setSecond(third);
          return;
      case DATE:
          Clock.setYear(first);
          Clock.setMonth(second);
          Clock.setDate(third);
          return;
      case ALRM:
          if (alarm == 1) {
              the_config.a1_hour = first;
              the_config.a1_min = second;
              the_config.a1_sec = third;
          } else if (alarm == 2) {
              the_config.a2_hour = first;
              the_config.a1_min = second;
              the_config.a1_sec = third;
          } else if (alarm == 3) {
              the_config.a3_hour = first;
              the_config.a3_min = second;
              the_config.a3_sec = third;
          }
          save_config();
          config_alarm();
          return;
       default:   
          return;
      }
    case KEY_ALARM:
      STATE = 25;
      return;
    default:
      break;
    };
  }  
}

static void set_date(void)
{
  lcd_ui("Date DD:MM:YY", "00:00:20", 31, 12, 30, 1, 1, 20, DATE, 0);
}

static void set_time(void)
{
    lcd_ui("Time HH:MM:SS", "00:00:00", 23, 59, 59, 0, 0, 0, TIME, 0);
}

static void set_alarm1(void)
{
    lcd_ui("ALARM1 HH:MM:SS", "00:00:00", 23, 59, 59, 0, 0, 0, ALRM, 1);
}

static void set_alarm2(void)
{
    lcd_ui("ALARM2 HH:MM:SS", "00:00:00", 23, 59, 59, 0, 0, 0, ALRM, 2);
}

static void set_alarm3(void)
{
    lcd_ui("ALARM3 HH:MM:SS", "00:00:00", 23, 59, 59, 0, 0, 0, ALRM, 3);
}

static void config_feeder(void)
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Config Feeder");
	lcd.setCursor(0, 1);
	lcd.print("Interval: ");
	lcd.print(interval, DEC);

  	while (1) {
		if (!KEY_INTR)
			continue;
		KEY_INTR = 0;

 		switch (KEY) {
		case KEY_UP:
			interval += 10;
			if (interval > 8000)
				interval = 10;
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Config Feeder");
			lcd.setCursor(0, 1);
			lcd.print("Interval: ");
			lcd.print(interval, DEC);
			break;
		case KEY_DOWN:
			interval -= 10;
			if (interval <= 10)
				interval = 10;
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Config Feeder");
			lcd.setCursor(0, 1);
			lcd.print("Interval: ");
			lcd.print(interval, DEC);
			break;
		case KEY_SELECT:
			the_config.interval = interval;
			save_config();
			break;
		case KEY_LEFT:
			STATE = 0;
			return;
     case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break;
		};
	}
}

static void feed_now()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print(" Feed ");

	while (1) {
		if (!KEY_INTR)
			continue;
		KEY_INTR = 0;
 
		switch (KEY) {
		case KEY_SELECT:
			myservo1.write(SERVO_FEED);
			delay(20 + the_config.interval);
			myservo1.write(SERVO_ZERO);
			delay(50);
			break;
		case KEY_UP:
			STATE = 0;
			return;
      case KEY_ALARM:
      STATE = 25;
      return;
		default:
			break;
		};
	}
}

func_t fptrs[] = 
{
	home_page,	//0
	state_1,	//1
	state_2,	//2
	state_3,	//3
	state_4,	//4
	state_5,	//5
	show_date,	//6
	show_time,	//7
	show_alarm1,	//8
	show_alarm2,	//9
	show_alarm3,	//10

	state_11,	//11
	state_12,	//12
	state_13,	//13
	state_14,	//14
	state_15,	//15
	state_16,	//16
	state_17,	//17
	set_date,	//18
	set_time,	//19
	set_alarm1,	//20
	set_alarm2,	//21
	set_alarm3,	//22
	config_feeder,	//23
	feed_now,	//24
  handle_alarm,
};
