#pragma once
#include <vector>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <functional>

#define PI 3.14159265359

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

float pitch_to_freq(float sample_rate, float pitch) {
    return sample_rate / pitch;
}

float freq_to_pitch(float sample_rate, float freq) {
    return sample_rate / freq;
}

float sin_waveform(float x, float freq) {
    return std::sin(x * 2 * PI * freq);
}
