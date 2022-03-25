
#include <iostream>
#include <vector>
#include "AudioFile.h"
#include "YIN.h"
#include <complex>
#include <algorithm>
#include <functional>
#include "YIN_online.h"
#include "utils.h"
#include "Autotune.h"


#define PI 3.14159265359


float pitch_YIN_original(std::vector<float> samples, float sample_rate, float window_size, int min, int max) {

    YIN yin = YIN(0.15, sample_rate, window_size);

    return yin.get_pitch_with_cumulative_difference(samples, window_size, 1, 20, max);
}

float pitch_YIN_online(std::vector<float> samples, float sample_rate, float window_size, int min, int max) {
    YIN_online yin = YIN_online(0.15, samples.size(), sample_rate);
    return yin.get_pitch(samples);
}

int main()
{
    std::ofstream myfile;
    myfile.open("salida.txt");

    AudioFile<double> input;
    input.load("C:/Users/Tania/Desktop/samples/lifeafterlove.wav");

    AudioFile<double> output;
    output.load("C:/Users/Tania/Desktop/samples/lifeafterlove.wav");


    float sample_rate = 500;

    int start = 0;
    int end = 5;
    int num_samples = int(sample_rate * (end - start) + 1);
    int window_size = 200;
    int min = 20;
    int max = std::floor(num_samples / 2);

    std::vector<float> x = linspace((float)start, (float)end, num_samples);

    std::vector<float> f(num_samples, 0);

    for (int i = 0; i < x.size(); i++) {
        f[i] = sin_waveform(x[i], 5);
    }

    YIN yin = YIN(0.15, sample_rate, window_size);
    

    //std::cout << "Autotune hacendado: " << yin.get_pitch_autotune_kindof(f, window_size, 1, min, max) << std::endl;
    //std::cout << "ACF: " << yin.get_pitch_autocorrelation_only(f, window_size, 1, min, max) << std::endl;
    //std::cout << "DIF: " << yin.get_pitch_with_difference(f, window_size, 1, min, max) << std::endl;
    //std::cout << "CMDF: " << yin.get_pitch_with_cumulative_difference(f, window_size, 1, min, max) << std::endl;
    //std::cout << "Thres: " << yin.get_pitch_threshold(f, window_size, 1, min, max) << std::endl;

    //std::cout << "Original YIN : " << pitch_YIN_original(f, sample_rate, window_size, min, max) << std::endl;

    Autotune autotune = Autotune(0.2, 80, 220, input.getSampleRate());


    max = std::floor(input.getNumSamplesPerChannel() / 2);

    double freq = 0;
    double Lmin = 0;

    for (int i = 0; i < input.getNumSamplesPerChannel(); i++) {

        autotune.add_sample(input.samples[0][i]);
        
        if (i % 8 == 0) {
            autotune.add_decimated_sample();
            freq = autotune.get_fundamental_frequency_python();
            Lmin = input.getSampleRate() / freq;
            if (i % 10000 == 0) {
                std::cout << "fundamental at index " << i << " : " << input.getSampleRate() / freq << std::endl;
            }
        }

        if (freq != 0) {
            autotune.calculate_resample_rate(freq);
        }

        double output_sample = 0;

        autotune.udpate_output_addr(Lmin);

        output_sample = autotune.get_output_sample(Lmin);
    
        output.samples[0][i] = output_sample;

        if (i % 10000 == 0)
            std::cout << "output sample at index " << i << " : " << output_sample << std::endl;
        
        if (i % 8 != 0) {
            autotune.reset_resample_rate();
        }
        /*if (i % 10000 == 0) {
            std::cout << "ACF: " << yin.get_pitch_autocorrelation_only(input.samples[0], window_size, i, min, max) << std::endl;
        }*/
    }


    
    output.save("C:/Users/Tania/Desktop/tests_pitch_detection.wav");
}
