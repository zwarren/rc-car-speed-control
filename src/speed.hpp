#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

#include <Arduino.h>

#include "pid-control.hpp"

class Servo;
class SerialMessageHandler;

class SpeedControl
{
public:
	/**
	 * external state transitions (set by the user)
	 * - STOPPED -> DRIVING or REVERSING
	 * - DRIVING -> STOPPING
	 * - REVERSING -> STOPPING_REVERSE
	 *
	 * internal transitions (can't be set directly by user)
	 * - STOPPING -> STOPPED
	 *
	 */

	enum State
	{
		DISABLED,
		ERROR,
		STOPPED,
		BRAKING,
		DRIVING,
		REVERSING,
		STOPPING_REVERSE
	};

	static const unsigned long minimum_tick_micros = 100;
	static const unsigned int tick_array_size = 4; // tick array size must be a power of 2
	static const unsigned int tick_array_mask = tick_array_size - 1;
	static const unsigned long poll_period_millis = 250;
	static const unsigned long stop_period_millis = 500;
	
	unsigned int throttle_step = 5;
	int last_throttle = 0;

	SerialMessageHandler *message_handler;
	Servo *throttle_servo;

	SpeedControl::State state = DISABLED;

	unsigned long last_update_millis;
	unsigned long last_update_ticks;
	unsigned long no_tick_period;
	unsigned long reverse_wait_start_millis;

	volatile unsigned long tick_array[tick_array_size];
	volatile unsigned long tick_counter;
	volatile unsigned long last_tick_micros;
	volatile unsigned int last_tick_index;

	unsigned long last_status_millis;

	int target_speed;

	void reset_tick_counter();

	float ticks_per_metre = 16;
	int brake_throttle = -40;
	int reverse_stop_throttle = 5;
	int throttle_offset = 0;

	PIDController pid;

	void set_throttle(int throttle);
	int get_throttle();
	void set_speed(int requested_speed);

	void init(SerialMessageHandler *message_handler, Servo *throttle_servo);
	void poll();
	void poll_status();
	void enable() {set_throttle(0); state = STOPPED;}
	void disable() {set_throttle(0); state = DISABLED;}
	void interrupt();
};

#endif /* SPEED_CONTROL_H */

