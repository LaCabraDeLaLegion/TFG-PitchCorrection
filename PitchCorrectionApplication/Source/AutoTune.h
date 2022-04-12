#pragma once
#include <vector>

class AutoTune {

public:

	AutoTune(float eps, float min_freq, float max_freq, float sample_rate);
	double get_fundamental_frequency();
	void add_sample(float sample);
	void add_decimated_sample();
	void calculate_resample_rate(float real_freq);
	void reset_resample_rate();
	void update_output_addr(float Lmin);
	double get_output_sample(float Lmin);

private:

	std::vector<float> linspace(float start, float end, float num);
	double find_L();
	double get_real_lag(double L);
	double get_desired_frequency(double real_freq);

	float epsilon = 0.2;
	float decay = 0;
	float min_L = 2;
	float max_L = 110;
	float sample_rate = 44100;
	float alpha = 0;
	float resample_rate = 1;
	int input_addr = 10;
	double output_addr = 0;
	std::vector<double> E;
	std::vector<double> H;
	std::vector<double> decimated_samples;
	std::vector<double> all_samples_filtered;
	std::vector<double> x_index;

	std::vector<float> notes = { 55.00,58.2,61.74,
		65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,
		138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,
		277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,
		554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50 };
};