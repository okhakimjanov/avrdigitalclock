/*
 * Clock.c
 *
 * Created: 28.04.2020 1:04:18
 * Author : CD Projekt Green
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>

#include "../../Headers/_main.h"
#include "../../Headers/lcd_n.h"
// function definitions
void showScreen();
void LCD_cleanup();
void isLeapYear();
void clearSingleLine(char line);
void interruptInit(void);
void startTime(void);
void invokeAlarm();
void initDevices();
void welcomeScreen();

// Constant for computing milliseconds for stopwatch
const float DELAY = 6.9444444;
// time for showing menu when you press interrupt_0
const char MODE_DURATION = 2;
// structures for clock and date to make code and implementation cleaner
struct Clock
{
	char hour;
	char minute;
	char second;
	int partOfDay;
};
struct Date
{
	int year;
	int month;
	int day;
	int weekDay;
};
// separate stopwatch struct as there are also milliseconds
struct Stopwatch{
	char hour;
	char minute;
	char second;
	char ms;
	};
// mode. Basically there are 3 modes
// Mode = 0 => Clock
// Mode = 1 => Alarm
// Mode = 2 => Stopwatch
// isModeChanged and modeCnt are for screen to show which mode it is for MODE_DURATION time
int mode = 0; 
char isModeChanged = 0;
char modeCnt = 0;

int cursor = 0;					// Changes with Interrupt_1. Serves to change particular values in structs
int isStopwatchRunning = 0;		// Serves as boolean to activate and deactivate time counter for stopwatch
unsigned char cnt;				// cnt to count seconds
unsigned char swCnt = 0;		// separate counter for stopwatch
char isAlarm = 0;				// boolean for alarm. In main() there is while loop and it checks whether to ring the alarm or not 
char isAlarmOn = 0;				// is alarm set by the user or not. On/Off

// Predefined arrays that are used in the program
char *partOfDay[] = {"AM", "PM"};
char *weekDays[] = {"Mon", "Tue", "Wed", "Thi", "Fri", "Sat", "Sun"};
// February has 28 days, but in Leap years it has 29 days
// So, the array is modified every time the year is changed by the user
// by using function isLeapYear(). So if it is leap year, then February has
// 29 days => daysInMonth[1] = 29. If not then daysInMonth[1] = 28
int	daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int	daysInMonthAlarm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
// Initialize structure objects with the initial values
struct Date timeDate = {2020, 1, 1, 3};
struct Date alarmDate = {2020, 1, 1, 3};
struct Clock time = {12, 0, 0, 0};
struct Clock alarm = {12, 0, 0, 0};
struct Stopwatch stopwatch = {0, 0, 0, 0};

// Initialize interrupts
void interruptInit(void){
	// Falling edge for INT0, INT1, INT2, INT3
	EICRA = 0xAA;	// 1010 1010
	EIMSK = 0x0F;	// 0000 1111 enable INT0, INT1, INT2, INT3
	DDRD = 0x00;	// PORTD as input
	sei();			// set Interrupt flag to 1
}
// TOV0 = (1 / (14.7456Mhz) * 1024(prescale) * 100 = 6.94444... ms
// 6.94444...ms * 144 = 1 sec
// start clock
void startTime(void){
	TCCR0 = 0x0f;	// CTC mode, Prescale 1024
	OCR0 = 99;		// count 0 to 99
	TIMSK = 0x02;	// Output compare interrupt enable 0000 00010
}

// Timer interrupt
ISR(TIMER0_COMP_vect){
	cnt++;
	if(cnt == 144){
		cnt = 0;
		time.second++;
		if(mode != 2){
			// occasionally refresh the screen line by line in order
			// to fix the bug with old values
			if(!(time.second % 6) || !(time.second % 5)){
				clearSingleLine(time.second % 2);	
			}
			
		}
		
		
		if(time.second >= 60){
			time.minute++;
			time.second = 0;
		}
		if(time.minute >= 60){
			time.minute = 0;
			time.hour++;
			// to make clock to work in AM/PM 12 hour system
			if(time.hour > 12){
				time.hour = 1;
			}
			if(time.hour > 11){
				time.partOfDay++;
			} 
		}
		// AM/PM
		if(time.partOfDay > 1){
			time.partOfDay = 0;
			timeDate.day++;
		}
		if(timeDate.day > daysInMonth[timeDate.month - 1]){
			timeDate.day = 1;
			timeDate.month++;
		}
		if(timeDate.month > 12){
			timeDate.month = 1;
			timeDate.year++;
		}
		// Alarm checks whether the clock structure values are the same as alarm structure values
		// and checks whether isAlarmOn is set on On (1)
		// if it is so, then it sets isAlarm to 1
		// and in while loop in main() function it checks for this
		// and rings
		if(isAlarmOn && time.hour == alarm.hour &&
		time.minute == alarm.minute && time.second == alarm.second
		&& timeDate.day == alarmDate.day
		&& timeDate.month == alarmDate.month
		&& timeDate.year == alarmDate.year){
			isAlarm = 1;
		}
		// end Alarm
		// Mode name counter
		// when you change mode, for instance, from Clock to Alarm
		// then menu is shown for 2 seconds
		// After 1 second, it clears the screen to avoid old values
		// then it stands for another second
		// and isModeChanged is set to 0
		// so that the screen for example with Alarm is shown
		if(isModeChanged != 0){
			modeCnt++;
			if(modeCnt == 1){
				LCD_cleanup();
			}
			if(modeCnt >= MODE_DURATION){
				isModeChanged = 0;
				LCD_cleanup();
			}
			
		}
	}
	
	// Stopwatch counter. It considers milliseconds also
	if(isStopwatchRunning != 0){
		swCnt++;
		// calculates values for milliseconds
		// uses DELAY value of 6.9444444 multiplied by swCnt
		// then it is divided by 10 and floor value is taken for more
		// precise update of milliseconds 
		// then the logic is almost the same as for clock
		stopwatch.ms = (int)(swCnt * DELAY) / 10;
		if(stopwatch.ms >= 100){
			swCnt = 0;
			stopwatch.ms = 0;
			stopwatch.second++;
		}
		if(stopwatch.second >= 60){
			stopwatch.minute++;
			stopwatch.second = 0;
		}
		if(stopwatch.minute >= 60){
			stopwatch.minute = 0;
			stopwatch.hour++;
		}
		// hours are from 0 to 99
		if(stopwatch.hour > 99){
			stopwatch.hour = 0;
		}
	}
	
}
SIGNAL(INT0_vect){
	// Interrupt 0
	// Changes modes == changes screens
	++mode;
	mode %= 3;				// there are 3 modes
	cursor = 0;				// sets cursor to initial value
	LCD_Clear();			
	modeCnt = 0;			// sets modeCnt to 0, which is used to hold up the screen for 2 seconds
	isModeChanged = 1;			
}
SIGNAL(INT1_vect){
	// Interrupt 1
	// Move cursor to right
	switch(mode){
		case 0:
			// move cursor to the needed position
			++cursor;
			cursor %= 8;		// for Clock there are 8 fields to be changeable
			break;
		case 1:
			++cursor;
			cursor %= 9;		// for Alarm there are one field more than in the Clock (isAlarmOn)
			break;
		case 2:
			// no values should be changed. Only set stopwatch to Run/Stop
			if(isStopwatchRunning == 0){
				// start stopwatch	
				isStopwatchRunning = 1;
			}
			else{
				// stop stopwatch (pause)
				isStopwatchRunning = 0;
			}
			break;
		default:
			break;	
	}
	 
}
// ArrowUp, increment the value, the cursor is pointing to
SIGNAL(INT2_vect){
	// Interrupt 2
	switch(mode){
		case 0:
			// watches
			switch(cursor){
				case 0:
					timeDate.year++;
					isLeapYear();	// checks whether it is leap year or not for February to be 28 or 29 days
					break;
				case 1:
					if(++timeDate.day > daysInMonth[timeDate.month - 1]){	
						timeDate.day = 1;
					};
						
					break;
				case 2:
					if(++timeDate.month > 12){
						timeDate.month = 1;
					}; 	
					break;
				case 3:
					timeDate.weekDay = ++timeDate.weekDay % 7;
					break;
				case 4:
					time.partOfDay = ++time.partOfDay % 2;
					break;
				case 5:
					if(++time.hour > 12){
						time.hour = 1;
					}
					break;
				case 6:
					time.minute = ++time.minute % 60;
					break;
				case 7:
					time.second = ++time.second % 60;
					break;
				default:
					break;
			}
			break;
		case 1:
			// Alarm mode
			// the logic is the same except there is a case 8 to make isAlarmOn = 0 or 1
			switch(cursor){
				case 0:
					alarmDate.year++;
					isLeapYear();
					break;
				case 1:
					if(++alarmDate.day > daysInMonth[alarmDate.month - 1]){
						alarmDate.day = 1;	
					}
					break;
				case 2:
					if(++alarmDate.month > 12){
						alarmDate.month = 1;
					}
					break;
				case 3:
					alarmDate.weekDay = ++alarmDate.weekDay % 7;
					break;
				case 4:
					alarm.partOfDay = ++alarm.partOfDay % 2;
					break;
				case 5:
					if(++alarm.hour > 12){
						alarm.hour = 1;
					}
					break;
				case 6:
					alarm.minute = ++alarm.minute % 60;
					break;
				case 7:
					alarm.second = ++alarm.second % 60;
					break;
				case 8:
					isAlarmOn = ++isAlarmOn % 2;
					break;
				default:
					break;
			}
			break;
			case 2:
				// Resets stopwatch
				stopwatch.hour = 0;
				stopwatch.minute = 0;
				stopwatch.second = 0;
				stopwatch.ms = 0;
				swCnt = 0;
				isStopwatchRunning = 0;
				break;
			default:
				break;
	}	
}

// Interrupt_3
// ArrowDown, Decrement the value
// Same logic as in In Interrupt_2, but decrements values or makes them maximum when there is "borrow"
// like if minute is 0, then it is set to 59  
// And also there is no case for stopwatch as it does not need any other functionalities
SIGNAL(INT3_vect){
	// Interrupt 1
	// Move cursor to right
	//LCD_cleanup();
	switch(mode){
		case 0:
		// watches
			switch(cursor){
				case 0:
					if(--timeDate.year < 1000){
						timeDate.year = 2050;
					}
					isLeapYear();
					break;
				case 1:
					if(--timeDate.day < 1){
						timeDate.day = daysInMonth[timeDate.month - 1]; 
					}
					break;
				case 2:
					if(--timeDate.month < 1){
						timeDate.month = 12;
					}
					break;
				case 3:
					if(--timeDate.weekDay < 0){
						timeDate.weekDay = 6;
					}
					break;
			
				case 4:
					if(--time.partOfDay < 0){
						time.partOfDay = 1;
					}
					break;
				case 5:
					if(--time.hour < 1){
						time.hour = 12;
					}
					break;
				case 6:
					if(time.minute == 0){
						time.minute = 59;
					}
					else{
						--time.minute;
					}
					break;
				case 7:
					if(time.second == 0){
						time.second = 59;
					}
					else{
						--time.second;
					}
					break;
				default:
				break;
			}
			break;
		case 1:
		// alarm
			switch(cursor){
				case 0:
					if(--alarmDate.year < 1000){
						alarmDate.year = 2050;
					}
					isLeapYear();
					break;
				case 1:
					if(--alarmDate.day < 1){
						alarmDate.day = daysInMonth[alarmDate.month - 1];
					}
					break;
				case 2:
					if(--alarmDate.month < 1){
						alarmDate.month = 12;
					}
					break;
				case 3:
					if(--alarmDate.weekDay < 0){
						alarmDate.weekDay = 6;
					}
					break;
				case 4:
					if(--alarm.partOfDay < 0){
						alarm.partOfDay = 1;
					}
					break;
				case 5:
					if(--alarm.hour < 1){
						alarm.hour = 12;
					}
					break;
				case 6:
					if(alarm.minute == 0){
						alarm.minute = 59;
					}
					else{
						--alarm.minute;
					}
					break;
				case 7:
					if(alarm.second == 0){
						alarm.second = 59;
					}
					else{
						--alarm.second;
					}
					break;
				case 8:
					isAlarmOn = ++isAlarmOn % 2;
					break;
				default:
					break;
			}
			break;
			default:
			break;
	}
	
	
}
// clean up the screen and sets cursor position to 0
void LCD_cleanup(){
	LCD_Clear();
	LCD_pos(0,0);
}
// Devices initialization
void initDevices(){
	cli(); // disable all interrupts
	PortInit();
	LCD_Init();
	interruptInit();
	LCD_cleanup();
	sei(); // re-enable all interrupts
}

// "Welcome!" is shown for 2 seconds whenever the program is started
void welcomeScreen(){
	char str[] = "Welcome!";
	LCD_STR(str);
	_delay_ms(2000);
	startTime();
	
}

// Basically this function shows text on LCD screen in first and second lines respectively
// depending on the mode (Clock, Alarm, Stopwatch)
// This function is called in main() function's while loop
void showScreen(){
	//LCD_cleanup();
	LCD_pos(0,0);
	char fl[16] = {0};
	char sl[16] = {0};

	char *alarmStatus = "Off";
	if(isAlarmOn){
		alarmStatus = "On ";
	}
	 
	// get first line
	switch(mode){
		case 0:
			// If mode is changed (Interrupt_0 is pressed), then it shows title of the
			// mode for 2 seconds
			if(isModeChanged) {
				sprintf(fl, "%s", "Clock");
				sprintf(sl, "%s", "next: Alarm");
			}else{
				sprintf(fl, "%d %d/%d %s",
				timeDate.year,
				timeDate.day,
				timeDate.month,
				weekDays[timeDate.weekDay]);
				sprintf(sl, "%s %c%c:%c%c:%c%c",
				partOfDay[time.partOfDay],
				time.hour / 10 + '0',	// + '0' to match it to ASCII table number, as numbers there start from 48
				time.hour % 10 + '0',
				time.minute / 10 + '0',
				time.minute % 10 + '0',
				time.second / 10 + '0',
				time.second % 10 + '0');
			}
			
			LCD_STR(fl);
			LCD_pos(1, 0);
			LCD_STR(sl);
			break;
		case 1:
		if(isModeChanged != 0) {
			sprintf(fl, "%s", "Alarm");
			sprintf(sl, "%s", "next: Stopwatch");
			}else{
			sprintf(fl, "%d %d/%d %s",
			alarmDate.year,
			alarmDate.day,
			alarmDate.month,
			weekDays[alarmDate.weekDay]);
			sprintf(sl, "%s %c%c:%c%c:%c%c %s", 
				partOfDay[alarm.partOfDay], 
				alarm.hour / 10 + '0',
				alarm.hour % 10 + '0',
				alarm.minute / 10 + '0',
				alarm.minute % 10 + '0',
				alarm.second / 10 + '0',
				alarm.second % 10 + '0',
				alarmStatus
				);}
			LCD_STR(fl);
			LCD_pos(1, 0);
			LCD_STR(sl);
			break;
		case 2:
		if(isModeChanged != 0) {
			sprintf(fl, "%s", "Stopwatch");
			sprintf(sl, "%s", "next: Clock");
			}
			else{
				if(isStopwatchRunning == 0){
					sprintf(fl, "%s","Press to run ");
					}else{
					sprintf(fl, "%s","Press to stop");
				}
				
				sprintf(sl,
				"%c%c:%c%c:%c%c:%c%c",
				stopwatch.hour / 10 + '0',
				stopwatch.hour % 10 + '0',
				stopwatch.minute / 10 + '0',
				stopwatch.minute % 10 + '0',
				stopwatch.second / 10 + '0',
				stopwatch.second % 10 + '0',
				stopwatch.ms / 10 + '0',
				stopwatch.ms % 10 + '0'
				);
			}
			LCD_pos(0, 0);
			LCD_STR(fl);
			LCD_pos(1, 0);
			LCD_STR(sl);
			
			break;
		default:
			break;
	}
	//LCD_cleanup();
	
}
void invokeAlarm(){
	// disable all LEDs
	PORTB = 0xFF;
	
	// We could just make for loop iterate for 7 times instead of 8
	// to get rid of PORTB = 0xFF line, but what if we want to make led blink more times?
	// So, this is more universal chunk of code
	for(int i = 0; i < 8; i++){
		PORTB = ~PORTB;
		LCD_cleanup();
		_delay_ms(200);
		PORTB = ~PORTB;				// Enable/Disable LEDs
		LCD_STR("WAKE UP!!!");		// Output "WAKE UP!!!" on LCD display
		_delay_ms(200);
		
	}
	isAlarm = 0;					// Reset alarm
	PORTB = 0xFF;					// disable LEDs
}

// Clears the line faster than LCD_clear()
void clearSingleLine(char line){
	LCD_pos(line, 0);
	char *str = "                ";
	LCD_STR(str);
}
// Calculates whether it is leap year or not and then
// puts the value in daysInMonth array for February
// Is Leap year => daysInMonth[1] = 29
// Is NOT Leap year => daysInMonth[1] = 28
void isLeapYear(){
	// is leap year for clock
	if((timeDate.year % 4 == 0 && timeDate.year % 100 != 0) || timeDate.year % 400 == 0){
		daysInMonth[1] = 29;
	}
	else{
		daysInMonth[1] = 28;
	}
	// is leap year for alarm
	if((alarmDate.year % 4 == 0 && alarmDate.year % 100 != 0) || alarmDate.year % 400 == 0){
		daysInMonthAlarm[1] = 29;
	}
	else{
		daysInMonthAlarm[1] = 28;
	}
}
int main(void)
{
	DDRD = 0x00;		// PortD is Input
	DDRB = 0xFF;		// PortB is Output
	PORTB = 0xFF;		// Disable all LEDs
	initDevices();		// Initialize devices
	welcomeScreen();	// Initially show "Welcome!" for 2 seconds 
	isLeapYear();		// Calculate whether it is leap year or not
	while(1){
		// Shows the content on the screen
		showScreen();
		if(isAlarm != 0){
			// If alarm is set and it should ring now, then it rings
			invokeAlarm();
		}
	}
}

