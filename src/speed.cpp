

#include "speed.hpp"
#include "message.hpp"

#include <Servo.h>

#include <util/atomic.h>

void
SpeedControl::interrupt()
{
	unsigned long now = micros();
	unsigned long period = now - last_tick_micros;

	// debounce, is it necessary?
	if (period > minimum_tick_micros)
	{
		tick_array[tick_counter & tick_array_mask] = period;
		last_tick_micros = now;
		tick_counter += 1;
	}
}

float
SpeedControl::current_speed()
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{		
		unsigned int ticks  = tick_counter;
	}

	if (tick_counter < tick_array_size)
		return -1;
	
	int i;
	unsigned long sum = 0;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		for (i = 0; i < tick_array_size; i++)
			sum += tick_array[i];
	}

	float avg_period = sum/float(tick_array_size);
	
	return 1.0/avg_period;
}


// both set_throttle and set_steer take a value between -90 and 90.

void
SpeedControl::set_throttle(int throttle)
{
	throttle = constrain(throttle + throttle_offset, -90, 90);
	throttle_servo->write(throttle + 90);
}

int
SpeedControl::get_throttle()
{
	return throttle_servo->read() - 90; // offset?
}

void
SpeedControl::set_speed(int requested_speed)
{
	switch (state)
	{
	case DISABLED:
		target_speed = requested_speed;
		break;

	case DRIVING:
		if (requested_speed > 0)
		{
			target_speed = requested_speed;
			/* let the poll function update the throttle */
		}
		else
		{
			/**
 			 * what happens if the car is stopped atm?
 			 * will brake result in reverse?
 			 */ 
			set_throttle(brake_throttle);
			target_speed = requested_speed;
			state = BRAKING;
		}
		break;

	case STOPPED:
		if (requested_speed > 0)
		{
			target_speed = requested_speed;
			state = DRIVING;
		}
		else if (requested_speed < 0)
		{	target_speed = requested_speed;
			state = REVERSING;
		}
		break;

	case BRAKING:
	case REVERSE_WAIT:
		if (requested_speed > 0)
		{
			target_speed = requested_speed;
			state = DRIVING;
		}
		else
		{
			target_speed = requested_speed;
		}
		break;

	case REVERSING:
		if (requested_speed < 0)
		{
			target_speed = requested_speed;
		}
		else
		{
			target_speed = requested_speed;
			state = STOPPING_REVERSE;

			/**
			 * there is no brake from reverse, just wait for it to finish
			 * why not just go straight into drive ?
			 */
			set_throttle(0);
		}
		break;
	}
}

void
SpeedControl::init(SerialMessageHandler *message_handler, Servo *throttle_servo)
{
	this->throttle_servo = throttle_servo;
	this->message_handler = message_handler;
	last_tick_micros = micros();
	last_update_millis = millis();
}

void
SpeedControl::poll()
{
	unsigned long now = millis();

	if (now - last_update_millis < 100)
		return;

	bool is_stopped = tick_counter - last_update_ticks == 0;

	switch (state)
	{
	case DISABLED:
		break;

	case STOPPED:
		if (target_speed < 0)
		{
			state = REVERSE_WAIT;
			reverse_wait_start_millis = now;
			set_throttle(0);
		}
		else if (target_speed > 0)
			state = DRIVING;
		break;

	case BRAKING:
		if (is_stopped)
		{
			set_throttle(0);

			if (target_speed < 0)
				state = REVERSE_WAIT;
			else
				state = STOPPED;
		}

		break;

	case DRIVING:
		/* update the throttle using pid */
		break;

	case REVERSING:
		break;

	case REVERSE_WAIT:
		if (now - reverse_wait_start_millis >= 200)
			state = REVERSING;
		break;
	}

	last_update_millis = now;
	last_update_ticks = tick_counter;
}

void
SpeedControl::disable()
{
	state = DISABLED;
	set_throttle(0);
}

