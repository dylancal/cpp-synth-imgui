#include "Synth.h"

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
            a_amp * m_oscA.ps.amp * /*(half_f_add_one(2 * m_lfoA.lfo_amp * m_lfoA.interpolate_amp())) * */m_oscA.interpolate_left() +
            b_amp * m_oscB.ps.amp * /*half_f_add_one(2 * m_lfoB.lfo_amp * m_lfoB.interpolate_amp()) * */m_oscB.interpolate_left() +
            c_amp * m_oscC.ps.amp * /*half_f_add_one(2 * m_lfoC.lfo_amp * m_lfoC.interpolate_amp()) * */m_oscC.interpolate_left());
        *out++ = amplitude.load(std::memory_order_relaxed) * (
            a_amp * m_oscA.ps.amp */*(half_f_add_one(2 * m_lfoA.lfo_amp * m_lfoA.interpolate_amp())) * */m_oscA.interpolate_right() +
            b_amp * m_oscB.ps.amp */*half_f_add_one(2 * m_lfoB.lfo_amp * m_lfoB.interpolate_amp()) * */m_oscB.interpolate_right() +
            c_amp * m_oscC.ps.amp */*half_f_add_one(2 * m_lfoC.lfo_amp * m_lfoC.interpolate_amp()) * */m_oscC.interpolate_right());

        for (std::size_t j = 0; j < 3; ++j) {
            oscillators[j].first->ps.left_phase += oscillators[j].first->ps.left_phase_inc;
            if (oscillators[j].first->ps.left_phase >= TABLE_SIZE) oscillators[j].first->ps.left_phase -= TABLE_SIZE;
            oscillators[j].first->ps.right_phase += oscillators[j].first->ps.right_phase_inc;
            if (oscillators[j].first->ps.right_phase >= TABLE_SIZE) oscillators[j].first->ps.right_phase -= TABLE_SIZE;
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


