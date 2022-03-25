
#include "CircularBuffer.h"

CircularBuffer::CircularBuffer()
{
    buffer = std::vector<double>();
    writeIndex = readIndex = delayLength = 0;
}

CircularBuffer::CircularBuffer(int bufferSize, int delayLength)
{
    buffer = std::vector<double>(bufferSize);
    buffer.clear();
    writeIndex = delayLength;
    readIndex = 0;
    this->delayLength = delayLength;
}

float CircularBuffer::getData()
{
    return buffer[readIndex];
}

void CircularBuffer::setData(float data)
{
    buffer[writeIndex] = data;
}

float CircularBuffer::getDataAtLag(int lag) {
    if (buffer.size() - 1 - lag >= 0) {
        return buffer[buffer.size() - 1 - lag];
    }
    return -1;
}

void CircularBuffer::nextSample()
{
    int bufferLength = buffer.size();
    readIndex = ((bufferLength + writeIndex) - delayLength) % bufferLength;
    writeIndex = (writeIndex + 1) % bufferLength;
}