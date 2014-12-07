#ifndef STEER_CONTROL_H
#define STEER_CONTROL_H

#include <Arduino.h>
#include <Servo.h>

class SteerControl
{
public:
	Servo *steer_servo;
	int steer_offset;

	void
	set_steer(int val)
	{
		val = constrain(val + steer_offset, -90, 90);
		steer_servo->write(val + 90);
	}

	int
	get_steer()
	{
		return steer_servo->read() - 90 -  steer_offset;
	}

	void init(Servo *steer_servo)
	{
		steer_offset = 0;
		this->steer_servo = steer_servo;
		set_steer(0);
	}
};

#endif
