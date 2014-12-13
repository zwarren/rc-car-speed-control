
#include <Servo.h>
#include "message.hpp"
#include "speed.hpp"
#include "steer.hpp"
#include "utils.hpp"

Servo steer_servo;
Servo throttle_servo;
SerialMessageHandler message_handler;
SpeedControl speed_control;
SteerControl steer_control;

static bool led_on;

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

	// configure the interrupt pin and the led pin
	pinMode(2, INPUT);
	digitalWrite(2, HIGH); // sensor needs the pull-up.
	attachInterrupt(0, tick_interrupt, FALLING);
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);

	steer_servo.attach(9);
	throttle_servo.attach(10);

	steer_control.init(&steer_servo);
	speed_control.init(&throttle_servo);

	message_handler.init(&speed_control, &steer_control);
}

void
loop()
{
	message_handler.poll();
	speed_control.poll();
}

