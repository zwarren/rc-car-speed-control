

#include "speed.hpp"
#include "message.hpp"

#include <Servo.h>

#include <util/atomic.h>

#define SSTR(s) Serial.print(F(s))
#define SVAL(val) Serial.print(val)
#define SSEP Serial.print(' ')
#define SENDL Serial.println()
#define SCOL Serial.print(':')

#define WARN_MSG(s) Serial.println(F(s))
#define ERROR_MSG(s) Serial.println(F(s))

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
	SSTR("State change ");
	SVAL(state_names[state]);
	SSTR(" to ");
	SVAL(state_names[new_state]);
	SENDL;

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
		unsigned int i = (unsigned int)(tick_counter & tick_array_mask);
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
	val = constrain(val + throttle_offset, -90, 90) + 90;
	SSTR("Set Throttle "); SVAL(val); SENDL;
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
		if (period_ticks != 0)
		{
			SSTR("Ticks while stopped!"); SENDL;
		}

		if (target_speed > 0)
			change_state(DRIVING);
		else if (target_speed < 0)
			change_state(REVERSING);

		reset_tick_counter();
		pid.reset();
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
		// if period_ticks is 0, just ramp the throttle until something happens.
		if (period_ticks == 0)
		{
			int throttle = abs(last_throttle) + throttle_step;

			if (throttle >= 90)
			{
				ERROR_MSG("Max throttle but not moving!");
				change_state(ERROR);
				set_throttle(0);
			}
			else
			{
				SSTR("Ramping."); SENDL;
				set_throttle(throttle*direction);
			}
		}
		else if (ticks < tick_array_size)
		{
			/* just let the first few ticks go by */
			SSTR("Ignoring initial ticks."); SENDL;
		}
		else
		{
			// period is microseconds/tick
			unsigned long average_tick_period = sum/tick_array_size;

			// speed is metres/second
			float current_speed = 1.0e6/average_tick_period/ticks_per_metre;

			int throttle = pid.update(now, current_speed, target_speed);
			throttle = constrain(throttle, -90, 90);

			set_throttle(throttle*direction);

			SSTR("Ticks "); SVAL(period_ticks); SSEP;
			SSTR("P.Avg "); SVAL(average_tick_period); SSEP;
			SSTR("Speed "); SVAL(current_speed); SSEP;
			SSTR("Throttle "); SVAL(throttle); SENDL;
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
		WARN_MSG("Set speed ignored while speed control disabled.");
		target_speed = 0;
		return;

	case ERROR:
		WARN_MSG("Set speed ignored while speed control error.");
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
	throttle_offset = 0;
	set_throttle(0);
	target_speed = 0;
	state = DISABLED;
	last_tick_micros = micros();
	last_update_millis = millis();
	tick_counter = 0;
}

#if 0
void
SpeedControl::poll_status()
{
	unsigned long now = millis();
	if (now - last_status_millis < 1000)
		return;

	unsigned long ticks;
	unsigned long p = average_period(&ticks);

	last_status_millis = now;
}
#endif


