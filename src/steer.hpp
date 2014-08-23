#ifndef STEER_CONTROL_H
#define STEER_CONTROL_H

#include <Arduino.h>
#include <Servo.h>

class SteerControl
{
	Servo *steer_servo;
public:
	int steer_offset;

	void
	set_steer(int steer)
	{
		steer = constrain(steer + steer_offset, -90, 90);
		steer_servo->write(steer + 90);
	}

	int
	get_steer()
	{
		return steer_servo->read() - 90; // offset?
	}

	void init(Servo *steer_servo) {this->steer_servo = steer_servo; }

};


#endif
