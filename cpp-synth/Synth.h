#pragma once
#include <vector>
#include "wavetable.h"
#include "portaudio.h"

constexpr auto SAMPLE_RATE = 48000;
struct Oscillator_t {
  Wavetable_t wtbl;
  LFO_t lfo;
  std::atomic<float> amp;
};

class Synth
{
private:
    PaStream* stream{ 0 };
    char message[20];
public:
    Oscillator_t oscA;
    Oscillator_t oscB;
    Oscillator_t oscC;
    std::vector<Oscillator_t*> oscs { &oscA, &oscB, &oscC };
    std::atomic<float> amplitude{ 0.1f };

public:
    Synth();
    bool open(PaDeviceIndex index);
    bool close();
    bool start();
    bool stop();
private:
    int paCallbackMethod(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags);

    static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
    void paStreamFinishedMethod();
    static void paStreamFinished(void* userData);
};

class ScopedPaHandler {
public:
    ScopedPaHandler()
        : _result(Pa_Initialize())
    {
    }
    ~ScopedPaHandler()
    {
        if (_result == paNoError)
        {
            Pa_Terminate();
        }
    }

    PaError result() const { return _result; }

private:
    PaError _result;
};
