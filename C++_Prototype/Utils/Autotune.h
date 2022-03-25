#pragma once
#include <vector>
#include <math.h>
#include <cmath>
#include "spline.h"

class Autotune
{
#define PI 3.14

public:

	Autotune(float eps, float min, float max, float sample_rate) {
		this->epsilon = eps;
		this->min_L = sample_rate / (max*8);
		this->max_L = sample_rate / (min*8);
		this->sample_rate = sample_rate;
		E = std::vector<double>(110 + 1, 0);
		H = std::vector<double>(110 + 1, 0);
		decimated_samples = std::vector<double>(0);
		all_samples_filtered = std::vector<double>(0);
		x_index = std::vector<double>(0);
		this->alpha = std::exp(-2 * PI * max / sample_rate);
		this->resample_rate = 1;
		this->input_addr = 10;
		this->output_addr = 0;
		this->decay = 0.5;
	}

	std::vector<float> linspace(float start, float end, float num)
	{
		std::vector<float> linspaced;

		if (num == 0) { return linspaced; }
		if (num == 1)
		{
			linspaced.push_back(start);
			return linspaced;
		}

		double delta = (end - start) / (num - 1);

		for (int i = 0; i < num - 1; ++i)
		{
			linspaced.push_back(start + delta * i);
		}
		linspaced.push_back(end);

		return linspaced;
	}
	
	double get_fundamental_frequency() {
		float L = find_L();
		float final_lag = get_real_lag(L);
		return this->sample_rate / final_lag;
	}

	double get_fundamental_frequency_python() {
		if (decimated_samples.size() == 221) {
			for (int L = this->min_L; L < this->max_L; L++) {
				int k = 221 - L;
				for (int j = 221-2*L; j < 221; j++) {
					E[L] += decimated_samples[j] * decimated_samples[j];
					if (j < 221 - L) {
						H[L] += decimated_samples[j] * decimated_samples[k];
					}
					k++;
				}
			}
		}
		else if (decimated_samples.size() > 221) {
			for (int L = this->min_L; L < this->max_L; L++) {
				float sample = decimated_samples[decimated_samples.size() - 1];
				float one_ago = decimated_samples[decimated_samples.size() - 1 - L];
				float two_ago = decimated_samples[decimated_samples.size() - 1 - 2*L];
				E[L] = E[L] + sample * sample - two_ago * two_ago;
				H[L] = H[L] + sample * one_ago - one_ago * two_ago;
				if (E[L] - 2 * H[L] < this->epsilon * E[L] && L > this->min_L && L < this->max_L) {
					double real_L = this->get_real_lag(L);
					return this->sample_rate / (real_L);
				}
			}
		}

		return 0;
	}

	void add_sample(float sample) {
		x_index.push_back(all_samples_filtered.size());

		if (all_samples_filtered.size() == 0) {
			all_samples_filtered.push_back(sample);
			return;
		}
		float previous_filtered_sample = all_samples_filtered[all_samples_filtered.size() - 1];
		float filtered_sample = previous_filtered_sample + this->alpha * (sample - previous_filtered_sample);
		all_samples_filtered.push_back(filtered_sample);
		this->input_addr++;
	}

	void add_decimated_sample() {
		decimated_samples.push_back(all_samples_filtered[all_samples_filtered.size() - 1]);
	}

	void calculate_resample_rate(float real_freq) {
		float desired_freq = get_desired_freq(real_freq);
		float desired_cycle_period = this->sample_rate / desired_freq;
		float cycle_period = this->sample_rate / real_freq;
		
		float resample_raw_rate = cycle_period / desired_cycle_period;
		float resample_rate1 = this->decay * this->resample_rate + (1 - this->decay) * resample_raw_rate;
		
		this->resample_rate = resample_rate1;
	}

	void reset_resample_rate() {
		this->resample_rate = 1;
	}

	void udpate_output_addr(float Lmin) {
		this->output_addr = this->output_addr + this->resample_rate;

		if (this->resample_rate > 1) {
			if (this->output_addr > this->input_addr) {
				this->output_addr = this->output_addr - Lmin;
			}
		}
		else {
			if (this->output_addr + Lmin < this->input_addr) {
				this->output_addr = this->output_addr + Lmin;
			}
		}
	}

	double get_output_sample(float Lmin) {

		
		float output_sample = 0;

		float x = output_addr - 5;
		float x1 = std::floor(x);
		float x2 = std::ceil(x);

		if (Lmin == 0) {
			output_sample = 0;
		}
		else if (this->resample_rate == 1 && this->output_addr - 5 >= 0) {
			output_sample = all_samples_filtered[all_samples_filtered.size() - 1];
		}
		else if (x1 >= 0 && x2 < all_samples_filtered.size()) {
			float y1 = this->all_samples_filtered[x1];
			float y2 = this->all_samples_filtered[x2];
			output_sample = y1 + (((y1 - y1) / (x2 - x1)) * (x - x1));
		}

		if (output_sample == 0) {
			int hola = 0;
		}
		else {
			int hol_a = 0;
		}

		return output_sample;
		

		/*
		double output_sample = 0;
		if ((int)output_addr != output_addr) {
			int hola = 0;
		}
		if (output_addr - 5 >= 0) {
			tk::spline interpolator(x_index, all_samples_filtered);
			output_sample = interpolator(output_addr - 5);
		}

		return output_sample;
		*/

	}

private:

	float epsilon;
	float min_L;
	float max_L;
	float sample_rate;
	float alpha;
	float resample_rate;
	int input_addr;
	double output_addr;
	std::vector<double> E;
	std::vector<double> H;
	std::vector<double> decimated_samples;
	std::vector<double> all_samples_filtered;
	std::vector<double> x_index;
	float decay;

	double find_L() {

		//De momento sin epsilon
		
		float min = 9999;
		int argmin = -1;
		for (int L = 2; L < 110; L++) {
			E[L] += decimated_samples[decimated_samples.size() - 1] * decimated_samples[decimated_samples.size() - 1];
			if ((int)(decimated_samples.size() - 1 - (2 * L)) >= 0) {
				E[L] -= decimated_samples[decimated_samples.size() - 1 - (2 * L)] * decimated_samples[decimated_samples.size() - 1 - (2 * L)];
				H[L] -= decimated_samples[decimated_samples.size() - 1 - L] * decimated_samples[decimated_samples.size() - 1 - (2 * L)];
			}
			if ((int)(decimated_samples.size() - 1 - L) >= 0) {
				H[L] += decimated_samples[decimated_samples.size() - 1] * decimated_samples[decimated_samples.size() - 1 - L];
			}

			if (E[L] - (2 * H[L]) < min) {
				min = E[L] - (2 * H[L]);
				argmin = L;
			}
		}

		return argmin;
	}

	double get_real_lag(double L) {

		//De momento busco solo entre los enteros. Mas adelante incluir interpolacion para buscar en valores decimales
		float full_L = 8 * L;

		std::vector<double> real_line(19);
		std::vector<double> real_mins(19, 0);

		std::vector<float> E_full(19, 0);
		std::vector<float> H_full(19, 0);
		for (int i = 0; i < E_full.size(); i++) {
			int lag = full_L - 9 + i;
			real_line[i] = lag;
			if (lag >= 0) {
				int k = all_samples_filtered.size() - 1 - lag;
				for (int j = int(all_samples_filtered.size() - 1 - (2 * lag)); j < int(all_samples_filtered.size() - 1); j++) {
					if (j >= 0)
						E_full[i] += all_samples_filtered[j] * all_samples_filtered[j];
					if (j >= 0 && j < int(all_samples_filtered.size() - 1 - lag) && k < int(all_samples_filtered.size() - 1))
						H_full[i] += all_samples_filtered[j] * all_samples_filtered[k];
					k++;
				}
			}

			real_mins[i] = (E_full[i] - 2*H_full[i])/H_full[i];
		}

		tk::spline interpolator(real_line, real_mins);

		std::vector<float> interpolation_line = linspace(real_line[0], real_line[real_line.size() - 1], 100);
		std::vector<double> interpolated(100);		

		float min = 9999;
		int argmin = -1;

		for (int i = 0; i < 100; i++) {
			interpolated[i] = interpolator(interpolation_line[i]);
			if (interpolated[i] < min) {
				min = interpolated[i];
				argmin = i;
			}
		}

		return interpolation_line[argmin];
	}

	double get_desired_freq(double real_freq) {

		std::vector<float> notes = { 55.00,58.2,61.74,
			65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.83,110.00,116.54,123.47,130.81,
			138.59,146.83,155.56,164.81,174.61,185.00,196.00,207.65,220.00,233.08,246.94,261.63,
			277.18,293.66,311.13,329.63,349.23,369.99,392.00,415.30,440.00,466.16,493.88,523.25,
			554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880.00,932.33,987.77,1046.50 };

		float min = 9999;
		int argmin = -1;
		for (int i = 0; i < notes.size(); i++) {
			if (std::abs(notes[i] - real_freq) < min) {
				min = std::abs(notes[i] - real_freq);
				argmin = i;
			}
		}

		if (argmin == -1)
			return -1;

		return notes[argmin];
	}


};

