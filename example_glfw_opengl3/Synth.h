#pragma once
#include <vector>
#include "wavetable.h"
#include "portaudio.h"

constexpr auto SAMPLE_RATE = 48000;

class Synth
{
private:
    PaStream* stream{ 0 };
    char message[20];
    float a_amp;
    float b_amp;
    float c_amp;
    //static int callback_idx;
public:
    // GENERAL
    Wavetable_t m_oscA;
    Wavetable_t m_oscB;
    Wavetable_t m_oscC;
    LFO_t m_lfoA;
    LFO_t m_lfoB;
    LFO_t m_lfoC;
    std::vector<std::pair<Wavetable_t*, LFO_t*>> oscillators {{ &m_oscA, & m_lfoA}, { &m_oscB, &m_lfoB }, { &m_oscC, &m_lfoC }};
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