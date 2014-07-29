
#include <Servo.h>

static const int odometer_interrupt_number = 0;
static volatile unsigned long odometer_tick;

static const size_t MSG_BUF_SIZE = 64;
static char msg_buf[MSG_BUF_SIZE];
static int next_msg_buf_index;

enum SpeedControlState
{
	SPEED_CONTROL_DISABLED,
	SPEED_CONTROL_STOPPED,
	SPEED_CONTROL_STOPPING,
	SPEED_CONTROL_DRIVING, /* can't think of a better word for going forwards */
	SPEED_CONTROL_REVERSING
};

static SpeedControlState speed_control_state = SPEED_CONTROL_DISABLED;

static int target_speed;
static int target_steer;
static int current_speed;
static int ticks_per_metre = 30;

Servo steer_servo;
Servo throttle_servo;

static int steer_offset;
static int throttle_offset;

unsigned long last_odometer_update;
unsigned long last_odometer_tick;

static const int odometer_update_period = 100; // milliseconds

static void
update_throttle(int current_speed, int target_speed)
{
	int throttle = 0; 

	throttle += throttle_offset + 90 +
	throttle_servo.write(throttle);
}

static void
speed_control_update(unsigned long dt, unsigned long ticks)
{
	switch (speed_control_state)
	{
	case SPEED_CONTROL_DISABLED:
		return;

	case SPEED_CONTROL_STOPPED:
		return;

	case SPEED_CONTROL_STOPPING:
		return;

	case SPEED_CONTROL_DRIVING:
		return;

	case SPEED_CONTROL_REVERSING:
		return;
	}
}

static void
odometer_interrupt()
{
	odometer_tick += 1;
}

static void
odometer_poll()
{
	unsigned long now = millis();
	unsigned long dt = now - last_odometer_update;

	if (dt < odometer_update_period)
		return;
	
	unsigned long tick_now = odometer_tick;
	
	speed_control_update(dt, tick_now - last_odometer_tick);

	last_odometer_tick = tick_now;
	last_odometer_update = now;
}

static void
odometer_init()
{
	last_odometer_update = millis();
}

static void
send_error(const __FlashStringHelper* msg)
{
	Serial.print(F("Error,"));
	Serial.println(msg);
}

static void
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

static void
set_stop()
{

}

static void
set_speed(int speed)
{

}

static void
set_throttle(int throttle)
{
	speed_control_state = SPEED_CONTROL_DISABLED;

	throttle += throttle_offset;
	if (throttle < 0 || throttle > 180)
	{
		send_error(F("throttle out of range."));
		return;
	}

	throttle_servo.write(throttle);
}

static void
set_steer(int steer)
{
	steer += steer_offset;

	if (steer < 0 || steer > 180)
	{
		send_error(F("steer out of range."));
		return;
	}

	steer_servo.write(steer);
}

#define STRMATCH(val, lit) (strcmp_P(val, PSTR(lit)) == 0)

static bool
next_int(char **p, int *val_ret)
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

static void
msg_process(char *buf, int len)
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
		set_stop();
	}
	else if (STRMATCH(msg_name, "Throttle"))
	{
		if (!next_int(&p, &val))
			set_throttle(val);
	}
	else if (STRMATCH(msg_name, "Steer"))
	{
		if (next_int(&p, &val))
			set_steer(val);
	}
	else if (STRMATCH(msg_name, "Speed"))
	{
		if (next_int(&p, &val))
			set_speed(val);		
	}
	else if (STRMATCH(msg_name, "SetTicksPerMetre"))
	{
		if (next_int(&p, &val))
			ticks_per_metre = val;		
	}
	else if (STRMATCH(msg_name, "SetThrottleOffset"))
	{
		if (next_int(&p, &val))
			throttle_offset = val;		
	}
	else if (STRMATCH(msg_name, "SetSteerOffset"))
	{
		if (next_int(&p, &val))
			steer_offset = val;		
	}
	else
	{
		send_error(F("Unknown command."));
	}
}

static void
msg_poll()
{
	while (Serial.available())
	{
		char c = Serial.read();
		if (c == '\n' || c == '\r')
		{
			msg_buf[next_msg_buf_index] = '\0';

			if (next_msg_buf_index > 0)
				msg_process(msg_buf, next_msg_buf_index);

			next_msg_buf_index = 0;
		}
		else
		{
			msg_buf[next_msg_buf_index] = c;
			next_msg_buf_index += 1;
			if (next_msg_buf_index == MSG_BUF_SIZE)
			{
				send_error(F("Msg buffer overflow."));
				next_msg_buf_index = 0;
			}
		}
	}
}

void
setup()
{
	Serial.begin(115200);
	attachInterrupt(0, odometer_interrupt, RISING);
	throttle_servo.attach(9);
	steer_servo.attach(10);	
}

void
loop()
{
	msg_poll();
	odometer_poll();
}

