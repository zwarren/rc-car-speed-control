#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

#include <Arduino.h>

class Servo;
class SerialMessageHandler;

class SpeedControl
{
	/**
	 * external state transitions (initiated by the user)
	 * - STOPPED -> DRIVING or REVERSING
	 * - DRIVING -> STOPPING
	 * - REVERSING -> DRIVING or STOPPING
	 * - REVERSE_WAIT -> DRIVING or STOPPING
	 *
	 * internal transitions
	 * - STOPPING -> REVERSE_WAIT -> REVERSING
	 * - STOPPING -> STOPPED
	 *
	 */

	enum State
	{
		DISABLED,
		STOPPED,
		BRAKING,
		DRIVING,
		REVERSING,
		REVERSE_WAIT,
		STOPPING_REVERSE
	};

	static const unsigned long minimum_tick_micros = 50;
	static const unsigned int tick_array_size = 4; // tick array size must be a power of 2
	static const unsigned int tick_array_mask = tick_array_size -1;

	SerialMessageHandler *message_handler;
	Servo *throttle_servo;

	SpeedControl::State state = DISABLED;

	unsigned long last_update_millis;
	unsigned long last_update_ticks;
	unsigned long reverse_wait_start_millis;

	unsigned long tick_array[tick_array_size];
	unsigned int tick_index;
	unsigned long last_tick_micros;
	unsigned long tick_counter;
	int target_speed;

public:
	int ticks_per_metre = 30;
	int brake_throttle = 40;
	int throttle_offset = 0;

	void set_throttle(int throttle);
	int get_throttle();
	float current_speed();
	void set_speed(int requested_speed);

	void set_throttle_offset(int amount) {throttle_offset = constrain(amount, -40, 40);}
	void set_brake_force(int amount) {brake_throttle = constrain(amount, 0, 90);}
	void set_ticks_per_metre(int amount) {ticks_per_metre = constrain(amount, 0, 1000);}
	
	void init(SerialMessageHandler *message_handler, Servo *throttle_servo);
	void poll();
	void disable();
	void interrupt();
};

#endif /* SPEED_CONTROL_H */

