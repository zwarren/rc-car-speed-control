
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

	steer_servo.attach(9);
	throttle_servo.attach(10);

	steer_control.init(&steer_servo);
	speed_control.init(&message_handler, &throttle_servo);

	message_handler.init(&speed_control, &steer_control);

	Serial.print("Steer = ");
	Serial.println(steer_servo.read());

	Serial.print("Steer Offset = ");
	Serial.println(steer_control.steer_offset);

	Serial.print("Throttle = ");
	Serial.println(throttle_servo.read());

	Serial.print("Throttle Offset = ");
	Serial.println(speed_control.throttle_offset);
}

void
loop()
{
	message_handler.poll();
	speed_control.poll();
}

