#include <vector>

class YIN_online
{

public:

	YIN_online(float threshold, float size, float sample_rate) {
		this->threshold = threshold;
		this->sample_rate = sample_rate;
		this->yin_buffer = std::vector<float>(size);
	}

	float get_pitch(std::vector<float>& samples) {
		return pitch(samples);
	}
private:

	float threshold;
	float sample_rate;
	std::vector<float> yin_buffer;


	float parabolic_interpolation(const std::vector<float> array, float x) {
		float x_adjusted;
		if (x < 1) {
			x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
		}
		else if (x > signed(array.size()) - 1) {
			x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
		}
		else {
			float den = array[x + 1] + array[x - 1] - 2 * array[x];
			float delta = array[x - 1] - array[x + 1];
			if (den == 0) {
				return x;
			}
			else {
				return x + delta / (2 * den);
			}
		}
		return x_adjusted;
	}



	std::vector<float> autocorrelation(std::vector<float> samples) {

		int N = samples.size();
		std::vector<float> r(N);

		for (int tau = 0; tau < N; tau++) {
			float entry = 0.0;
			for (int j = 1; j < N - tau; j++) {
				entry += samples[j] * samples[j + tau];
			}
			r[tau] = entry;
		}

		return r;
	}

	float absolute_threshold(const std::vector<float> yin_buffer) {
		int size = yin_buffer.size();
		int tau;
		for (tau = 2; tau < size; tau++) {
			if (yin_buffer[tau] < this->threshold) {
				while (tau + 1 < size && yin_buffer[tau + 1] < yin_buffer[tau]) {
					tau++;
				}
				break;
			}
		}

		return (tau == size || yin_buffer[tau] >= this->threshold) ? -1 : tau;
	}
	void difference(const std::vector<float> samples) {

		std::vector<float> autocorrelation_values(samples.size());
		autocorrelation_values = autocorrelation(samples);

		for (int tau = 0; tau < this->yin_buffer.size()/2; tau++) {
			yin_buffer[tau] = autocorrelation_values[0] + autocorrelation_values[1] - 2 * autocorrelation_values[tau];
		}
	}

	void cumulative_mean_normalized_difference(std::vector<float> yin_buffer) {
		double running_sum = 0.0f;
		yin_buffer[0] = 1;
		for (int tau = 1; tau < yin_buffer.size(); tau++) {
			running_sum += yin_buffer[tau];
			yin_buffer[tau] *= tau / running_sum;
		}
	}

	float pitch(std::vector<float> samples) {
		int tau_estimate;
		difference(samples);
		cumulative_mean_normalized_difference(this->yin_buffer);
		tau_estimate = absolute_threshold(this->yin_buffer);
		auto ret = (tau_estimate != -1) ? sample_rate / parabolic_interpolation(this->yin_buffer, tau_estimate) : -1;
		return ret;
	}
};


