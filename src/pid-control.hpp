#ifndef PID_CONTROL_HPP
#define PID_CONTROL_HPP

class PIDController
{
public:
	unsigned long last_update;
	float last_error;
	float last_output;
	float integral_error;
	float Kp, Ki, Kd;
	float max_integral;
	float max_step;
	bool first_update;

	float update(unsigned long now, float current, float target)
	{
		float error = target - current;
		float u = Kp*error;

		if (!first_update)
		{
			unsigned long dt = now - last_update;
			float derivative = (error - last_error)/dt; 
			u += Ki*integral_error + Kd*derivative;
			integral_error += error;
			integral_error = constrain(integral_error, -max_integral, max_integral);
		}

		last_update = now;
		last_error = error;
		first_update = false;

		// rate limit the change in output signal.
		float output = constrain(u,  0, last_output + max_step);
		last_output = output;
		return output;
	}

	void
	reset()
	{
		last_output = 0;
		first_update = true;
		integral_error = 0;
	}

	PIDController()
	{
		first_update = true;
		last_output = 0;
		Kp = 0.0;
		Ki = 0.0;
		Kd = 0.0;
		max_integral = 100;
		max_step = 5;
	}
};

#endif /* PID_CONTROL_HPP */

