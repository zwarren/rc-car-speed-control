
#include <Servo.h>
#include "message.hpp"
#include "speed.hpp"
#include "steer.hpp"

Servo steer_servo;
Servo throttle_servo;
SerialMessageHandler message_handler;
SpeedControl speed_control;
SteerControl steer_control;

static void
tick_interrupt()
{
	speed_control.interrupt();
}

void
setup()
{
	Serial.begin(115200);
	attachInterrupt(0, tick_interrupt, RISING);
	throttle_servo.attach(9);
	steer_servo.attach(10);
	message_handler.init(&speed_control, &steer_control);
	speed_control.init(&message_handler, &throttle_servo);
}

void
loop()
{
	message_handler.poll();
	speed_control.poll();
}

