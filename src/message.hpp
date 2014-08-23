#ifndef SERIAL_MESSAGE_HANDLER_H
#define SERAIL_MESSAGE_HANDLER_H

#include <Arduino.h>

class SpeedControl;
class SteerControl;

class SerialMessageHandler
{
	static const size_t MSG_BUF_SIZE = 64;
	char buf[MSG_BUF_SIZE];
	int buf_index;

	SpeedControl *speed_control;
	SteerControl *steer_control;

	bool next_int(char **p, int *val_ret);
	void process();
	
public:
	void send_error(const __FlashStringHelper* msg);
	void send_state(int speed, int steer, int distance);

	void init(SpeedControl *speed_control, SteerControl *steer_control);
	void poll();
};

#endif /* SERIAL_MESSAGE_HANDLER_H */

