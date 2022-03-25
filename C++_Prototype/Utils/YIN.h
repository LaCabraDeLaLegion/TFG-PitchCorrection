#include <vector>
class YIN
{
private:

    float threshold;
    float sample_rate;
    float hop_size;
    
    float ACF(std::vector<float> samples, float window, int time, int lag) {
        int N = samples.size();
        float entry = 0.0;
        for (int j = time; j < N - lag; j++) {
            entry += samples[j] * samples[j + lag];
        }
        return entry;
    }

    float DIF(std::vector<float> samples, float window, int time, int lag) {
        return ACF(samples, window, time, 0) + ACF(samples, window, time + lag, 0) - (2 * ACF(samples, window, time, lag));
    }

    float CDIF(std::vector<float> samples, float window, int time, int lag) {
        if (lag == 0) {
            return 1;
        }
        else {
            float sum = 0;
            for (int j = 1; j < lag; j++) {
                sum += DIF(samples, window, time, j);
            }
            return (DIF(samples, window, time, lag) * lag) / sum;
        }
    }

public:

    YIN(float threshold, float sample_rate, float hop_size) {
        this->threshold = threshold;
        this->sample_rate = sample_rate;
        this->hop_size = hop_size;
    }

    float get_pitch_autotune_kindof(std::vector<float> samples, float window_size, int time, float min, float max) {
        int index = -1;
        int contador = 0;
        for (int i = min; i < max; i++) {
            if (0.2 * ACF(samples, window_size, time, 0) >= ACF(samples, window_size, time, 0) - 2 * ACF(samples, window_size, time, i)) {
                index = i;
                contador++;
            }
        }
        return sample_rate / (index + min);
    }

    float get_pitch_autocorrelation_only(std::vector<float> samples, float window_size, int time, float min, float max) {

        std::vector<float> ACF_values(max);
        float max_value = 0;
        int argmax = 0;
        for (int i = min; i < max; i++) {
            ACF_values[i] = ACF(samples, window_size, time, i);
            if (ACF_values[i] > max_value) {
                max_value = ACF_values[i];
                argmax = i;
            }
        }

        return sample_rate / (argmax + min);
    }

    float get_pitch_with_difference(std::vector<float> samples, float window_size, int time, float min, float max) {

        std::vector<float> DIF_values(max);
        float max_value = 0;
        int argmax = 0;
        for (int i = min; i < max; i++) {
            DIF_values[i] = DIF(samples, window_size, time, i);
            if (DIF_values[i] > max_value) {
                max_value = DIF_values[i];
                argmax = i;
            }
        }

        return sample_rate / (argmax + min);
    }

    float get_pitch_with_cumulative_difference(std::vector<float> samples, float window_size, int time, float min, float max) {

        std::vector<float> CDIF_values(max);
        float min_value = 9999;
        int argmin = 0;
        for (int i = min; i < max; i++) {
            CDIF_values[i] = CDIF(samples, window_size, time, i);
            if (CDIF_values[i] < min) {
                min = CDIF_values[i];
                argmin = i;
            }
        }

        return sample_rate / (argmin + min);
    }

    float get_pitch_threshold(std::vector<float> samples, float window_size, int time, float min, float max) {

        std::vector<float> CDIF_values(max);
        int arg = -1;
        int min_value = 9999;
        int argmin = 0;
        for (int i = min; i < max; i++) {
            CDIF_values[i] = CDIF(samples, window_size, time, i);
            if (CDIF_values[i] < min_value) {
                min_value = CDIF_values[i];
                argmin = i;
            }
            if (CDIF_values[i] < threshold) {
                arg = i;
                break;
            }
        }

        if (arg == -1) {
            return sample_rate / (argmin + min);
        }
        else {
            return sample_rate / (arg + min);
        }
    }


};

