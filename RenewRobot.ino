/* -------------------------------------------------------------------- */ 
/*                Arduino Tank Robot - Team Tanks `R` US                */
/* Created by Tosin Daudu with help from USF Department of Engineering  */
/*                       Using Arduino IDE 1.8.19                       */
/*                     Edited and Tested on 11/29/22                    */
/* -------------------------------------------------------------------- */ 

#include "Arduino.h"
enum resolution_t { MICROS, MILLIS };
enum status_t {STOPPED, RUNNING, PAUSED};

/* Timer Library; Credit to Stefan Stuab at Timer/Timer.h (https://github.com/sstaub/Timer/blob/main/Timer.h) */
  class Timer
  {
  public:
	Timer(resolution_t resolution = MILLIS);
	~Timer();
	void start();
	void pause();
	void resume();
	void stop();
	uint32_t read();
	status_t state();

  private:
	uint32_t started;
	uint32_t elapsed;
	resolution_t resolution;
	status_t status;
  };

  Timer::Timer(resolution_t resolution) {
	this->resolution = resolution;
  }

  Timer::~Timer() {}

  void Timer::start() {
	elapsed = 0;
	if(resolution == MILLIS) started = millis();
	if(resolution == MICROS) started = micros();
	status = RUNNING;
  }

  void Timer::pause() {
	if (status == RUNNING) {
	  if(resolution == MILLIS) elapsed = elapsed + millis() - started;
	  if(resolution == MICROS) elapsed = elapsed + micros() - started;
	  status = PAUSED;
	}
  }

  void Timer::resume() {
	if (status == PAUSED) {
	  if(resolution == MILLIS) started = millis();
	  if(resolution == MICROS) started = micros();
	  status = RUNNING;
	}
  }

  void Timer::stop() {
	if (status == RUNNING) {
	  if(resolution == MILLIS) elapsed = millis() - started + elapsed;
	  if(resolution == MICROS) elapsed = micros() - started + elapsed;
	}
	status = STOPPED;
  }

  uint32_t Timer::read() {
	if (status == RUNNING) {
	  if(resolution == MILLIS) return millis() - started + elapsed;
	  if(resolution == MICROS) return micros() - started + elapsed;
	}
	return elapsed;
  }

  status_t Timer::state() { return status; }
/* End Timer Library */

#include <LiquidCrystal_I2C.h> /* Import liquid crystal library */

LiquidCrystal_I2C lcd(0x27, 16, 2); /* Specifices how to reference the LCD and it's characteristics */

#include <Servo.h>

Servo myservo;  // create servo object to control a servo


#define ECHOPIN 8 /* attach pin D8 to pin Echo of HC-SR04 */
#define TRIGPIN 7 /* attach pin D7 to pin Trig of HC-SR04 */
#define CHANGE_DIR_TIME 5000


const char* stateLabel = "Robot State:    "; /* Label for robotic states */ 

const char* robotStatesArr[] = { /* List of message and their cases */
	"STOPPED         ", /* Index: 0, stopped robot state */
	"BACKWARD        ", /* Index: 1, moving backwards robot state */
	"FORWARD         ", /* Index: 2, moving forwards robot state */
	"OUT OF RANGE    ",  /* Index: 3, robot can't find object in range */

	/* Animation */
	"  o            o  ", /* Index: 4, open eyes pt1 */
	"     ________     ", /* Index: 5, open eyes pt2 */
	"   --        --   ", /* Index: 6, closed eyes pt1 */
	"     ________     ", /* Index: 7, closed eyes pt2 */
	"       .  .       ", /* Index: 8, smiley pt1 */
	"      |_____|     " /* Index: 9, smiley pt2 */
};

void startUpAnimation() 
{
	for(int i = 0; i < 5; i++)
	{
		lcd.setCursor(0, 0); 
		lcd.print(robotStatesArr[4]); 
		lcd.setCursor(0, 1);
		lcd.print(robotStatesArr[5]);
		delay(500); 
		lcd.setCursor(0, 0); 
		lcd.print(robotStatesArr[6]); 
		lcd.setCursor(0, 1);
		lcd.print(robotStatesArr[7]);
		delay(500); 
	}
	lcd.setCursor(0, 0); 
	lcd.print(robotStatesArr[8]); 
	lcd.setCursor(0, 1);
	lcd.print(robotStatesArr[9]);
	delay(2000);  
}

enum Direction /* All robot directions */
{
	Forwards,
	Backwards,
	Nil
};

struct Motor /* Motor data structure */
{
	const int motorPinA; /* use both `A` and `B` pins to control directions */
	const int motorPinB;
	bool isMoving; /* track to see if motor is moving */
	enum Direction direction; /* motor direction data */ 

	void off() { /* set values to turn off motors */
		digitalWrite(motorPinA, LOW);
		digitalWrite(motorPinB, LOW);
		isMoving = false;
		direction = Direction::Nil;
	}

	void move(Direction dir) /* move motor in certain direction */
	{
		isMoving = true;
		direction = dir;
		switch (dir) /* match direction and move motor in that direction */
		{
		case Direction::Forwards:
			digitalWrite(motorPinA, LOW);
			digitalWrite(motorPinB, HIGH);
			break;
		case Direction::Backwards:
			digitalWrite(motorPinA, HIGH);
			digitalWrite(motorPinB, LOW);
			break;
		default:
			isMoving = !isMoving;
			break;
		}
	}
};

struct Motor leftMotor = { /* make left motor */
	.motorPinA = 4, /* L298N motor driver pin 1 for left motor */
	.motorPinB = 5, /* L298N motor driver pin 2 for left motor */
	.isMoving = false,
	.direction = Direction::Nil
};

struct Motor rightMotor = { /* make right motor */
	.motorPinA = 9, /* L298N motor driver pin 1 for right motor */
	.motorPinB = 10, /* L298N motor driver pin 2 for right motor */
	.isMoving = false,
	.direction = Direction::Nil
};


/* Global Variables */
	int LCDprev = 99; /* helps to update the LCD state tracks previous */
	int LCDcurr = 100; /* helps to update the LCD state tracks current */

	int ledpin = 13; //specify the LED pin to pin 13

	long duration; /* duration of sound wave travel */
	int distance; /* distance measurement */
	Timer timer; /* timer for turn sequence */
	bool turn; /* turn condition */
	int servoPos;    /* variable to store the servo position */
/* Global Variables end */

void setup() 
{
	lcd.init(); /* initializes LCD */
	lcd.backlight(); /* turn on backlight */

	pinMode(TRIGPIN, OUTPUT) ; /* sonar digital pin mode for trig */
	pinMode(ECHOPIN, INPUT); /* sonar digital pin mode for echo - input because it detects signal sent out */

	pinMode(leftMotor.motorPinA, OUTPUT); /* All motor pins are outputs */
	pinMode(leftMotor.motorPinB, OUTPUT); /* All motor pins are outputs */
	pinMode(rightMotor.motorPinA, OUTPUT); /* All motor pins are outputs */
	pinMode(rightMotor.motorPinB, OUTPUT); /* All motor pins are outputs */
	pinMode(ledpin, OUTPUT) ; /* setup LCD pin as an output */

	Serial.begin (9600) ; /* use default of 9600 baud rate for serial communication */

	startUpAnimation();

	lcd.setCursor(0, 0); /* set cursor at top left of lcd screen */
	lcd.print (stateLabel); /* prints the first line "Robot State: " */

	timer.start();
	turn = false;
	servoPos = 0;
	myservo.attach(12);  /* attaches the servo on pin 12 to the servo object */
}

/*  
	NOTE from USF ENG: only update the LCD screen when a change of state has occured
	Otherwise, the program become inefficient. LCDprey and LCDcurr variables help us to do this
*/

void calcDistance()
{
	/* Clears the TRIGPIN condition from previous run */
	digitalWrite (TRIGPIN, LOW); /* starts trig pin low */
	delayMicroseconds (2); /* waits for hardware to respond */
	digitalWrite (TRIGPIN, HIGH); /* makes trig pin high */
	delayMicroseconds (10) ; /* continues high trig pin state for 10 microseconds */
	digitalWrite (TRIGPIN, LOW); /* after 10 microseconds trig pin is brought low */
	duration = pulseIn (ECHOPIN, HIGH); /* reads echo as it receives the pulse and stores duration */
	distance = duration * 0.034 / 2; /* Converts the time of flight to distance to object */

	/* Displays the distance on the serial Monitor */
	Serial.print ("Distance: ");
	Serial.print (distance);
	Serial.println(" cm");
}


void searchTurn(bool isStatic = false) /* essentially magic simple but took me soo long to make please dont touch omg */
{
	if (!isStatic){
		Direction left, right;
		if(turn) { left = Direction::Forwards; right = Direction::Backwards; }
		else { left = Direction::Backwards; right = Direction::Forwards; }
			leftMotor.move(left);
			rightMotor.move(right); 
		Serial.println(timer.read());
		if(timer.read() > CHANGE_DIR_TIME) { turn = !turn; timer.stop(); timer.start(); }  
	} else {
		   myservo.write(servoPos);
		   if(servoPos < 180) { // goes from 0 degrees to 180 degrees
			  servoPos += 10;
			  myservo.write(servoPos);// tell servo to go to position in variable 'pos'
		   } else if (servoPos == 180) {
			  servoPos = 0;
		   } else { // goes from 180 degrees to 0 degrees
			  servoPos -= 10;
			  myservo.write(servoPos);// tell servo to go to position in variable 'pos'
		   }
	}
	
}

void loop() 
{
	LCDprev = LCDcurr;
	calcDistance();

	if (distance <= 15) { /* Too close move backwards */
		LCDcurr = 1; /* set message state to compare with previous state */
		leftMotor.move(Direction::Backwards); /* Move motors backwards */
		rightMotor.move(Direction::Backwards);
		digitalWrite(ledpin, HIGH); /* Turn on lcd to write to screen */

		if (LCDprev != LCDcurr) { /* whether or not to change message is necessary */
			Serial.println(robotStatesArr[1]);
			lcd.setCursor(0, 1);
			lcd.print(robotStatesArr[1]);
			delay(10);
		}
	} else if ((distance <= 30) && (distance > 15)) { /* Stop case */
		LCDcurr = 2;
		leftMotor.off(); /* Both motors off */
		rightMotor.off();
		digitalWrite(ledpin, LOW);

		if (LCDprev != LCDcurr) {  /* whether or not to change message is necessary */
			Serial.println(robotStatesArr[0]);
			lcd.setCursor(0, 1);
			lcd.print(robotStatesArr[0]);
			delay(10);
		}
	} else if ((distance <= 45) && (distance > 30)) { /* Close enough to keep moving forward */
		LCDcurr = 3;
		leftMotor.move(Direction::Forwards);
		rightMotor.move(Direction::Forwards);
		digitalWrite(ledpin, HIGH); /* Turn on lcd to write to */

		if (LCDprev != LCDcurr) {  /* whether or not to change message is necessary */
			Serial.println(robotStatesArr[2]);
			lcd.setCursor(0, 1);
			lcd.print(robotStatesArr[2]);
			delay(10);
		}
	} else { /* values not in range too far case */
		LCDcurr = 4;
		//leftMotor.off();
		//rightMotor.off();
		searchTurn();
		digitalWrite(ledpin, LOW);

		if (LCDprev != LCDcurr) { /* whether or not to change message is necessary */
			Serial.println(robotStatesArr[3]);
			lcd.setCursor(0, 1);
			lcd.print(robotStatesArr[3]);
			delay(10);
		}
	}
	delay(10);
}

/*
  Changelog:
	overhaul code for motor data structs and direction,
	movement functions
	searching algorithm instead of idle out of range 
	timing use
	compartimentalize functions
	improve readability
	startup animation                 ?
	back to vs code for spacing
	Better Debug info for states in Serial Monitor
	credit to timer lib
*/

/* todo servo motor, improve movement, experiment with ranges */

/* 
	Update: with limited time for making our robot we decided to cut the servo feature.
			Additionally, adding such a feature with the searchturn function working so well it 
			would only add to complexity and would decrease the effectiveness of the ultrasonic sensor.
	End development Log: Robot recieved good grade and software working amazing. :)
*/