#ifndef PID_CONTROL_HPP
#define PID_CONTROL_HPP

class PIDController
{
public:
	unsigned long last_update;
	float last_error;
	float integral_error;
	float Kp, Ki, Kd;
	float max_integral;
	bool first_update;

	float update(unsigned long now, float current, float target)
	{
		float error = target - current;
		float output = Kp*error;

		if (!first_update)
		{
			unsigned long dt = now - last_update;
			float derivative = (error - last_error)/dt; 
			output += Ki*integral_error + Kd*derivative;
			integral_error += error;
			integral_error = constrain(integral_error, -max_integral, max_integral);
		}

		last_update = now;
		last_error = error;
		first_update = false;
		return output;
	}

	void
	reset()
	{
		first_update = true;
		integral_error = 0;
	}

	PIDController()
	{
		first_update = true;
		Kp = 1.0;
		Ki = 0.01;
		Kd = 0.0;
		max_integral = 45;
	}
};

#endif /* PID_CONTROL_HPP */

