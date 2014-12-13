
#include <Servo.h>
#include <util/atomic.h>

#include "speed.hpp"
#include "message.hpp"
#include "utils.hpp"

static const char *state_names[SpeedControl::NUM_STATES] = {
	"DISABLED",
	"ERROR",
	"STOPPED",
	"BRAKING",
	"DRIVING",
	"REVERSING",
	"STOPPING_REVERSE"
};

void
SpeedControl::change_state(State new_state)
{
	DEBUG_MSG("State %s to %s", state_names[state], state_names[new_state]);

	state = new_state;
}

void
SpeedControl::interrupt()
{
	unsigned long now = micros();
	unsigned long period = now - last_tick_micros;

	// debounce
	if (period > minimum_tick_micros)
	{
		unsigned int i = (unsigned int)(tick_counter) & tick_array_mask;
		tick_array[i] = period;
		last_tick_index = i;
		last_tick_micros = now;
		tick_counter += 1;
	}
}

void
SpeedControl::reset_tick_counter()
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		tick_counter = 0;
	}
}

// both set_throttle and set_steer take a value between -90 and 90.

void
SpeedControl::set_throttle(int val)
{
	last_throttle = val;
	val = constrain(val + throttle_offset, -30, 30) + 90;
	throttle_servo->write(val);
}

int
SpeedControl::get_throttle()
{
	return throttle_servo->read() - 90 - throttle_offset;
}

void
SpeedControl::poll()
{
	unsigned long now = millis();

	if (now - last_update_millis < poll_period_millis)
		return;

	unsigned long sum = 0;
	unsigned long ticks;
	unsigned long last_period;
	unsigned int last_index;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		int i;
		ticks = tick_counter;
		last_period = tick_array[last_tick_index];
		last_index = last_tick_index;
		for (i = 0; i < tick_array_size; i++)
		{
			sum += tick_array[i];
		}
	}

	unsigned long period_ticks = ticks - last_update_ticks;

	int direction = 1;

	switch (state)
	{
	case DISABLED:
		break;

	case ERROR:
		if (last_throttle != 0)
			set_throttle(0);
		break;

	case STOPPED:
		if (period_ticks != 0) {
			WARN_MSG("Ticks while stopped!");
			set_throttle(0);
		}
		else {
			if (target_speed > 0)
				change_state(DRIVING);
			else if (target_speed < 0)
				change_state(REVERSING);

			reset_tick_counter();
			pid.reset();
		}
		break;

	case BRAKING:
	case STOPPING_REVERSE:
		if (period_ticks == 0)
		{
			no_tick_period += poll_period_millis;
			if (no_tick_period >= stop_period_millis)
			{
				set_throttle(0);
				change_state(STOPPED);
			}
		}
		else
		{
			no_tick_period = 0;
		}
		break;

	case REVERSING:
		direction = -1;
	case DRIVING:
		//DEBUG_MSG("A %lu %lu %lu %lu",
		//	tick_array[0], tick_array[1], tick_array[2], tick_array[3]);
		{
			unsigned long current_speed = 0;
			unsigned long average_tick_period = 0;

			if (period_ticks >= tick_array_size)
			{
				average_tick_period = sum/tick_array_size;

				// period is in uS, so period in seconds is p/1000000
				// frequency is then 1000000/p
				// speed is centimetres/second (cm/tick)*ticks/second
				// given ticks/m, ticks/cm = (ticks/m)/100 and cm/tick = 100/(ticks/m)
				// so we've got speed cm/s = (100/(ticks/m))*(1000000/p)
				// simplifying, this is 10000000
				current_speed =
					100000000UL/(average_tick_period*ticks_per_metre);
			}

			long u = pid.update(now, current_speed, abs(target_speed));

			if (u >= 90)
			{
				ERROR_MSG("Max throttle but not moving!");
				change_state(ERROR);
				set_throttle(0);
				break;
			}

			int throttle = u*direction;
			set_throttle(throttle);

			DEBUG_MSG("C:%3lu P:%6lu S:%3lu E:%3ld I:%3ld U:%2ld T:%2d",
				period_ticks, average_tick_period, current_speed,
				long(pid.last_error), long(pid.integral_error),
				current_speed, u, throttle);
		}
		break;

	default:
		ERROR_MSG("Unknown state.");
		set_throttle(0);
		change_state(ERROR);
	}

	last_update_ticks = ticks;
	last_update_millis = now;
}

void
SpeedControl::set_speed(int requested_speed)
{
	switch (state)
	{
	case DISABLED:
		/* the speed control should be enabled first and then call set_speed()
		 * don't want enable and surprise
		 */
		WARN_MSG("Set speed ignored (disabled)");
		target_speed = 0;
		return;

	case ERROR:
		WARN_MSG("Set speed ignored (error).");
		return;

	case STOPPED:
		if (requested_speed > 0)
			change_state(DRIVING);
		else if (requested_speed < 0)
			change_state(REVERSING);

		target_speed = requested_speed;
		reset_tick_counter();
		pid.reset();
		break;

	case DRIVING:
		if (requested_speed <= 0)
		{
			set_throttle(brake_throttle);
			change_state(BRAKING);
		}
		break;

	case BRAKING:
		if (requested_speed > 0)
		{
			pid.reset();
			change_state(DRIVING);
		}
		break;

	case REVERSING:
		if (requested_speed >= 0)
		{
			change_state(STOPPING_REVERSE);
			set_throttle(reverse_stop_throttle);
		}
		break;

	case STOPPING_REVERSE:
		if (requested_speed < 0)
		{
			pid.reset();
			change_state(REVERSING);
		}
		break;

	default:
		ERROR_MSG("unknown state");
		change_state(ERROR);
		set_throttle(0);
		return;
	}

	target_speed = requested_speed;
}

void
SpeedControl::init(Servo *throttle_servo)
{
	this->throttle_servo = throttle_servo;
	throttle_offset = 2; /* fixme: save in eeprom */
	set_throttle(0);
	target_speed = 0;
	state = DISABLED;
	last_tick_micros = micros();
	last_update_millis = millis();
	tick_counter = 0;

	pid.Kp = 0.01;
	pid.Ki = 0.01;
	pid.Kd = 0.0;
	pid.max_integral = 1000;
}


