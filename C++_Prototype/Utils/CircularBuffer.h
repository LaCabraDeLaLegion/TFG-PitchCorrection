#pragma once
#include <vector>

class CircularBuffer {

public:
    CircularBuffer();
    CircularBuffer(int bufferSize, int delayLength);
    float getData();
    void setData(float data);
    void nextSample();
    float getDataAtLag(int lag);

private:
    std::vector<double> buffer;
    int writeIndex;
    int readIndex;
    int delayLength;
};

