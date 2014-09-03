
#include <Servo.h>
#include "message.hpp"
#include "speed.hpp"
#include "steer.hpp"

Servo steer_servo;
Servo throttle_servo;
SerialMessageHandler message_handler;
SpeedControl speed_control;
SteerControl steer_control;

bool led_on;

static void
tick_interrupt()
{
	digitalWrite(13, led_on);
	led_on = !led_on;
	speed_control.interrupt();
}

void
setup()
{
	Serial.begin(115200);
	Serial.println("RC-Car Speed Control");
	pinMode(2, INPUT);
	digitalWrite(2, HIGH); // sensor needs the pull-up.
	attachInterrupt(0, tick_interrupt, FALLING);
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);
	throttle_servo.attach(9);
	steer_servo.attach(10);
	message_handler.init(&speed_control, &steer_control);
	speed_control.init(&message_handler, &throttle_servo);
}

void
loop()
{
	//message_handler.poll();
	speed_control.poll();
}

