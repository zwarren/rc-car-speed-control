#include "message.hpp"

#include "speed.hpp"
#include "steer.hpp"

#define STRMATCH(val, lit) (strcmp_P(val, PSTR(lit)) == 0)
#define SSTR(s) Serial.print(F(s))
#define SVAL(val) Serial.print(val)
#define SSEP Serial.print(' ')
#define SENDL Serial.println()

bool
SerialMessageHandler::next_int(char **p, int *val_ret)
{
	char *s = strsep(p, " ");

	if (s == NULL || *s == '\0')
		return false;

	char *endptr;

	*val_ret = strtol(s, &endptr, 10);
	
	if (*endptr != '\0')
		return false;

	return true;
}

bool
SerialMessageHandler::next_float(char **p, float *val_ret)
{
	char *s = strsep(p, " ");

	if (s == NULL || *s == '\0')
		return false;

	char *endptr;

	long int_part;
	float frac_part = 0;

	int_part = strtol(s, &endptr, 10);

	if (*endptr == '.')
	{
		char *frac_str = endptr + 1;
		size_t frac_len = strlen(frac_str);
		frac_part = strtol(frac_str, &endptr, 10);
		if (*endptr != '\0')
			return false;

		frac_part /= pow(10,frac_len);
	}
	else if (*endptr != '\0')
		return false;

	*val_ret = int_part + frac_part;

	return true;
}

void
SerialMessageHandler::invalid_param()
{
	send_error(F("Invalid param."));
}

void
SerialMessageHandler::process()
{
	char *p = buf;
	int val;

	char *msg_name = strsep(&p, " ");
	if (!msg_name)
	{
		send_error(F("Invalid message."));
		return;
	}

	if (STRMATCH(msg_name, "Stop"))
	{
		speed_control->set_speed(0);
	}
	else if (STRMATCH(msg_name, "Throttle"))
	{
		if (next_int(&p, &val))
			speed_control->set_throttle(val);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "Steer"))
	{
		if (next_int(&p, &val))
			steer_control->set_steer(val);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "Speed"))
	{
		if (next_int(&p, &val))
			speed_control->set_speed(val);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "SetBrakeThrottle"))
	{
		if (next_int(&p, &val))
			speed_control->brake_throttle = constrain(val, -90, 0);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "SetTicksPerMetre"))
	{
		if (next_int(&p, &val))
			speed_control->ticks_per_metre = val;
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "SetThrottleOffset"))
	{
		if (next_int(&p, &val))
			speed_control->throttle_offset = constrain(val, -90, 90);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "SetSteerOffset"))
	{
		if (next_int(&p, &val))
			steer_control->steer_offset = constrain(val, -90, 90);
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "SetPidParams"))
	{
		float kp, ki, kd, max_integral;
		if (next_float(&p, &kp) && next_float(&p, &ki)
			&& next_float(&p, &kd) && next_float(&p, &max_integral))
		{
			speed_control->pid.Kp = kp;
			speed_control->pid.Ki = ki;
			speed_control->pid.Kd = kd;
			speed_control->pid.max_integral = max_integral;
		}
		else
		{
			invalid_param();
		}
	}
	else if (STRMATCH(msg_name, "GetSpeedCtrlParams"))
	{
		if (next_int(&p, &val))
		{
			SSTR("SpeedCtrlParams"); SSEP;
			SSTR("Token"); SSEP; SVAL(val); SSEP;
			SSTR("TicksPerMetre"); SSEP; speed_control->ticks_per_metre; SSEP;
			SSTR("BrakeForce"); SSEP; speed_control->brake_throttle; SSEP;
			SSTR("ThrottleOffset"); SSEP; speed_control->throttle_offset; SENDL;
		}
		else
			invalid_param();
	}
	else if (STRMATCH(msg_name, "Test"))
	{
		SSTR("TestResponse");
		while (next_int(&p, &val))
		{
			SSEP;
			SVAL(val);
		}
		SENDL;
	}
	else
	{
		send_error(F("Unknown command."));
	}
}

void
SerialMessageHandler::send_error(const __FlashStringHelper* msg)
{
	SSTR("Error"); SSEP; SVAL(msg); SENDL;
}

void
SerialMessageHandler::send_speed_control_status(unsigned long now, int throttle, float speed, unsigned long ticks)
{
	SSTR("SpdCtrl"); SSEP; SVAL(now); SSEP; SVAL(throttle); SSEP; SVAL(speed); SSEP; SVAL(ticks); SENDL;
}

void
SerialMessageHandler::init(SpeedControl *speed_control, SteerControl *steer_control)
{
	this->speed_control = speed_control;
	this->steer_control = steer_control;
	buf_index = 0;
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

