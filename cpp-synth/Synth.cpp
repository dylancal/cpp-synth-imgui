#include "Synth.h"
#include "wavetable.h"

Synth::Synth() {
     sprintf(message, "Synth End ");
     oscA.amp = 0.2f;
     oscB.amp = 0.2f;
     oscC.amp = 0.2f;
}

bool Synth::open(PaDeviceIndex index) {
    PaStreamParameters outputParameters{ };

    outputParameters.device = index;
    if (outputParameters.device == paNoDevice) {
        return false;
    }

    const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
    if (pInfo != 0)
    {
        printf("Output device name: %s\r", pInfo->name);
    }

    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE, 512, 0, &Synth::paCallback, this);

    if (err != paNoError)
    {
        return false;
    }

    err = Pa_SetStreamFinishedCallback(stream, &Synth::paStreamFinished);

    if (err != paNoError)
    {
        Pa_CloseStream(stream);
        stream = 0;

        return false;
    }

    return true;
}

bool Synth::close() {
    if (stream == 0)
        return false;
    PaError err = Pa_CloseStream(stream);
    stream = 0;
    return (err == paNoError);
}

bool Synth::start() {
    if (stream == 0)
        return false;
    PaError err = Pa_StartStream(stream);
    return (err == paNoError);
}

bool Synth::stop() {
    if (stream == 0)
        return false;
    PaError err = Pa_StopStream(stream);
    return (err == paNoError);
}

int Synth::paCallbackMethod(const void* inputBuffer, 
                            void* outputBuffer, 
                            unsigned long framesPerBuffer, 
                            const PaStreamCallbackTimeInfo* timeInfo, 
                            PaStreamCallbackFlags statusFlags) {

    float* out = (float*)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)inputBuffer;

    for (std::size_t i = 0; i < framesPerBuffer; i++) {
        *out++ = amplitude.load(std::memory_order_relaxed) * (
            oscA.amp * oscA.wtbl.interpolate_left() +
            oscB.amp * oscB.wtbl.interpolate_left() +
            oscC.amp * oscC.wtbl.interpolate_left());
        *out++ = amplitude.load(std::memory_order_relaxed) * (
            oscA.amp * oscA.wtbl.interpolate_right() +
            oscB.amp * oscB.wtbl.interpolate_right() +
            oscC.amp * oscC.wtbl.interpolate_right());
        for (std::size_t j = 0; j < 3; ++j) {
            oscs[j]->wtbl.ps.left_phase += oscs[j]->wtbl.ps.left_phase_inc;
            if (oscs[j]->wtbl.ps.left_phase >= TABLE_SIZE) oscs[j]->wtbl.ps.left_phase -= TABLE_SIZE;
            oscs[j]->wtbl.ps.right_phase += oscs[j]->wtbl.ps.right_phase_inc;
            if (oscs[j]->wtbl.ps.right_phase >= TABLE_SIZE) oscs[j]->wtbl.ps.right_phase -= TABLE_SIZE; 
        }
    }
    return paContinue;
}

int Synth::paCallback(const void* inputBuffer, 
                      void* outputBuffer, 
                      unsigned long framesPerBuffer, 
                      const PaStreamCallbackTimeInfo* timeInfo, 
                      PaStreamCallbackFlags statusFlags, 
                      void* userData) {

    return ((Synth*)userData)->paCallbackMethod(inputBuffer, 
                                                outputBuffer,
                                                framesPerBuffer,
                                                timeInfo,
                                                statusFlags);
}

void Synth::paStreamFinishedMethod() {
    printf("Stream Completed: %s\n", message);
}

void Synth::paStreamFinished(void* userData) {
    return ((Synth*)userData)->paStreamFinishedMethod();
}


