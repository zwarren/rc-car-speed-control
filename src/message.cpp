#include "message.hpp"

#include "speed.hpp"
#include "steer.hpp"
#include "utils.hpp"

#define STRMATCH(val, lit) (strcmp_P(val, PSTR(lit)) == 0)

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
	WARN_MSG("Invalid param.");
}

enum MsgType {STOP, SET, GET};

#define FP2(arg) int(arg),int(arg*100) - 100*int(arg)

void
SerialMessageHandler::process()
{
	char *p = buf;
	int val;

	MsgType msg_type;
	char *msg_type_str = strsep(&p, " ");
	if (!msg_type_str) {
		WARN_MSG("Invalid message.");
		return;
	}

	if (STRMATCH(msg_type_str, "Stop")) {
		speed_control->disable();
		return;
	}
	else if (STRMATCH(msg_type_str, "Start")) {
		speed_control->enable();
		return;
	}
	else if (STRMATCH(msg_type_str, "Set")) {
		msg_type = SET;
	}
	else if (STRMATCH(msg_type_str, "Get")) {
		msg_type = GET;
	}
	else {
		WARN_MSG("Invalid message.");
		return;
	}

	char *var_name = strsep(&p, " ");

	if (STRMATCH(var_name, "Throttle")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				speed_control->set_throttle(val);
			else
				invalid_param();
		}
		else
			MSG("Throttle %d", speed_control->get_throttle());
	}
#if 0
	else if (STRMATCH(var_name, "ThrottleRaw")) {
		if (msg_type == GET)
			MSG("ThrottleRaw %d", speed_control->throttle_servo->read());
		else
			invalid_param();
	}
#endif
	else if (STRMATCH(var_name, "Steer")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				steer_control->set_steer(val);
			else
				invalid_param();
		}
		else
			MSG("Steer %d", steer_control->get_steer());
	}
#if 0
	else if (STRMATCH(var_name, "SteerRaw")) {
		if (msg_type == GET)
			MSG("SteerRaw %d", steer_control->steer_servo->read());
		else
			invalid_param();
	}
#endif
	else if (STRMATCH(var_name, "Speed")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				speed_control->set_speed(val);
			else
				invalid_param();
		}
		else
			MSG("TargetSpeed %d", speed_control->target_speed);
	}
#if 0
	else if (STRMATCH(var_name, "SpeedControl"))
	{
		if (msg_type == SET) {
			if (next_int(&p, &val)) {
				if (val != 0)
					speed_control->enable();
				else
					speed_control->disable();
			}
			else
				invalid_param();
		}
		else
			MSG("SpeedControl %d", speed_control->state);
	}
#endif
	else if (STRMATCH(var_name, "BrakeThrottle")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				speed_control->brake_throttle = constrain(val, -90, 0);
			else
				invalid_param();
		}
		else
			MSG("BrakeThrottle %d", speed_control->brake_throttle);
	}
#if 0
	else if (STRMATCH(var_name, "TicksPerMetre"))
	{
		if (msg_type == SET) {
			if (next_int(&p, &val))
				speed_control->ticks_per_metre = val;
			else
				invalid_param();
		}
		else
			MSG("TicksPerMetre %d", speed_control->ticks_per_metre);
	}
#endif
	else if (STRMATCH(var_name, "ThrottleOffset")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				speed_control->throttle_offset = constrain(val, -90, 90);
			else
				invalid_param();
		}
		else
			MSG("ThrottleOffset %d", speed_control->throttle_offset);
	}
	else if (STRMATCH(var_name, "SteerOffset")) {
		if (msg_type == SET) {
			if (next_int(&p, &val))
				steer_control->steer_offset = constrain(val, -90, 90);
			else
				invalid_param();
		}
		else
			MSG("SteerOffset %d", steer_control->steer_offset);
	}
	else if (STRMATCH(var_name, "PIDParams")) {
		PIDController *pid = &speed_control->pid;
		if (msg_type == SET) {
			float kp, ki, kd, max_integral;
			if (next_float(&p, &kp) && next_float(&p, &ki)
				&& next_float(&p, &kd) && next_float(&p, &max_integral))
			{
				pid->Kp = kp;
				pid->Ki = ki;
				pid->Kd = kd;
				pid->max_integral = max_integral;
			}
			else {
				invalid_param();
			}
		}
		else {
#if 0
			MSG("Kp %d.%02d Ki %d.%02d Kd %d.%02d Max.I %d.%02d",
				FP2(pid->Kp), FP2(pid->Ki), FP2(pid->Kd), FP2(pid->max_integral));
#endif
		}
	}
#if 0
	else if (STRMATCH(var_name, "SpeedCtrlParams"))
	{
		if (next_int(&p, &val))
		{
			MSG("Fixme");
			MSG("SpeedCtrlParams"); SSEP;
			SSTR("Token"); SSEP; SVAL(val); SSEP;
			SSTR("TicksPerMetre"); SSEP; speed_control->ticks_per_metre; SSEP;
			SSTR("BrakeForce"); SSEP; speed_control->brake_throttle; SSEP;
			SSTR("ThrottleOffset"); SSEP; speed_control->throttle_offset; SENDL;
		}
		else
			invalid_param();
	}
#endif
	else {
		ERROR_MSG("Unknown command.");
	}
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
				ERROR_MSG("Msg buffer overflow.");
				buf_index = 0;
			}
		}
	}
}

