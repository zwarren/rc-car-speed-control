#include "message.hpp"

#include "speed.hpp"
#include "steer.hpp"

#define STRMATCH(val, lit) (strcmp_P(val, PSTR(lit)) == 0)

bool
SerialMessageHandler::next_int(char **p, int *val_ret)
{
	char *s = strsep(p, ",");

	if (s == NULL || *s == '\0')
		goto error;

	char *endptr;
	
	*val_ret = strtol(s, &endptr, 10);
	
	if (*endptr != '\0')
		goto error;

	return true;

error:
	send_error(F("Invalid param."));
	return false;
}

void
SerialMessageHandler::process()
{
	char *p = buf;
	int val;
	
	char *msg_name = strsep(&p, ",");
	if (!msg_name)
	{
		send_error(F("Invalid message."));
		return;
	}

	if (STRMATCH(msg_name, "Stop"))
	{
		speed_control->set_speed(0);
	}
	else if (STRMATCH(msg_name, "Brake"))
	{
		if (!next_int(&p, &val))
			speed_control->set_brake_force(val);
	}
	else if (STRMATCH(msg_name, "Throttle"))
	{
		if (!next_int(&p, &val))
			speed_control->set_throttle(val);
	}
	else if (STRMATCH(msg_name, "Steer"))
	{
		if (next_int(&p, &val))
			steer_control->set_steer(val);
	}
	else if (STRMATCH(msg_name, "Speed"))
	{
		if (next_int(&p, &val))
			speed_control->set_speed(val);		
	}
	else if (STRMATCH(msg_name, "SetTicksPerMetre"))
	{
		if (next_int(&p, &val))
			speed_control->ticks_per_metre = val;		
	}
	else if (STRMATCH(msg_name, "SetThrottleOffset"))
	{
		if (next_int(&p, &val))
			speed_control->throttle_offset = val;		
	}
	else if (STRMATCH(msg_name, "SetSteerOffset"))
	{
		if (next_int(&p, &val))
			steer_control->steer_offset = val;		
	}
	else
	{
		send_error(F("Unknown command."));
	}
}

void
SerialMessageHandler::send_error(const __FlashStringHelper* msg)
{
	Serial.print(F("Error,"));
	Serial.println(msg);
}

void
send_state(int speed, int steer, int distance)
{
	Serial.print(F("State,"));
	Serial.print(speed);
	Serial.print(F(","));
	Serial.print(steer);
	Serial.print(F(","));
	Serial.print(distance);
	Serial.println();
}

void
SerialMessageHandler::init(SpeedControl *speed_control, SteerControl *steer_control)
{
	this->speed_control = speed_control;
	this->steer_control = steer_control;
	buf_index=0;
}

void	
SerialMessageHandler::poll()
{
	while (Serial.available())
	{
		char c = Serial.read();
		if (c == '\n' || c == '\r')
		{
			buf[buf_index] = '\0';

			if (buf_index > 0)
				process();

			buf_index = 0;
		}
		else
		{
			buf[buf_index] = c;
			buf_index += 1;
			if (buf_index == MSG_BUF_SIZE)
			{
				send_error(F("Msg buffer overflow."));
				buf_index = 0;
			}
		}
	}
}

