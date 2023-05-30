#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cmath>
#include "portaudio.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <utility>
#include <memory>

#include "wavetable.h"
#include "imgui_includes.h"

constexpr auto NUM_SECONDS = (5);
constexpr auto SAMPLE_RATE = (48000);

class Synth
{
private:
    PaStream* stream{ 0 };
    char message[20];
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
    Synth() {
        gen_saw_wave(m_oscA);
        gen_saw_wave(m_oscB);
        gen_saw_wave(m_oscC);
        gen_sin_wave(m_lfoA);
        gen_sin_wave(m_lfoB);
        gen_sin_wave(m_lfoC);
        sprintf_s(message, "Synth End ");
    }


    bool open(PaDeviceIndex index) {
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
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(
            &stream,
            nullptr, 
            &outputParameters,
            SAMPLE_RATE,
            paFramesPerBufferUnspecified,
            0,      
            &Synth::paCallback,
            this
        );

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
    bool close() {
        if (stream == 0)
            return false;

        PaError err = Pa_CloseStream(stream);
        stream = 0;

        return (err == paNoError);
    }
    bool start() {
        if (stream == 0)
            return false;

        PaError err = Pa_StartStream(stream);

        return (err == paNoError);
    }
    bool stop() {
        if (stream == 0)
            return false;

        PaError err = Pa_StopStream(stream);

        return (err == paNoError);
    }


private:
    int paCallbackMethod(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags) {
        float* out = (float*)outputBuffer;
        (void)timeInfo;
        (void)statusFlags;
        (void)inputBuffer;

        for (std::size_t i = 0; i < framesPerBuffer; i++) {
            *out++ = amplitude.load(std::memory_order_relaxed) * (m_oscA.ps.amp.load(std::memory_order_relaxed) * (half_f_add_one(2 * m_lfoA.lfo_depth * m_lfoA.interpolate())) * m_oscA.interpolate_left() +
                                m_oscB.ps.amp.load(std::memory_order_relaxed) * half_f_add_one(2 * m_lfoB.lfo_depth * m_lfoB.interpolate()) * m_oscB.interpolate_left() +
                                m_oscC.ps.amp.load(std::memory_order_relaxed) * half_f_add_one(2 * m_lfoC.lfo_depth * m_lfoC.interpolate()) * m_oscC.interpolate_left());
            *out++ = amplitude.load(std::memory_order_relaxed) * (m_oscA.ps.amp.load(std::memory_order_relaxed) * (half_f_add_one(2 * m_lfoA.lfo_depth * m_lfoA.interpolate())) * m_oscA.interpolate_right() +
                                m_oscB.ps.amp.load(std::memory_order_relaxed) * half_f_add_one(2 * m_lfoB.lfo_depth * m_lfoB.interpolate()) * m_oscB.interpolate_right() +
                                m_oscC.ps.amp.load(std::memory_order_relaxed) * half_f_add_one(2 * m_lfoC.lfo_depth * m_lfoC.interpolate()) * m_oscC.interpolate_right());

            for (std::size_t j = 0; j < 3; ++j) {
                oscillators[j].first->ps.left_phase += oscillators[j].first->ps.left_phase_inc;
                if (oscillators[j].first->ps.left_phase >= TABLE_SIZE) oscillators[j].first->ps.left_phase -= TABLE_SIZE;
                oscillators[j].first->ps.right_phase += oscillators[j].first->ps.right_phase_inc;
                if (oscillators[j].first->ps.right_phase >= TABLE_SIZE) oscillators[j].first->ps.right_phase -= TABLE_SIZE;
            }
        }
        return paContinue;
    }

    static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
        return ((Synth*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
            framesPerBuffer,
            timeInfo,
            statusFlags);
    }

    void paStreamFinishedMethod() {
        printf("Stream Completed: %s\n", message);
    }

    static void paStreamFinished(void* userData) {
        return ((Synth*)userData)->paStreamFinishedMethod();
    }
};

class ScopedPaHandler
{
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

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    // useful to create some of these here for use in functions later
    Wavetable_t saw_wave;
    Wavetable_t sin_wave;
    Wavetable_t sqr_wave;
    gen_saw_wave(saw_wave);
    gen_sin_wave(sin_wave);
    gen_sqr_wave(sqr_wave, 0.5);

    // start setting up glfw
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Lookup-table Synthesizer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // create our synth object and audio handler
    Synth st;
    ScopedPaHandler paInit;

    if (paInit.result() != paNoError) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", paInit.result());
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
        return 1;
    }

    if (!st.open(Pa_GetDefaultOutputDevice()))
    {
        
    }
    if (!st.start())
    {

    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_intro_window = true;
    bool show_wavetable_window = true;
    bool show_synth_settings = true;
    bool show_oscA = true;
    bool show_oscB = true;
    bool show_oscC = true;
    bool show_osc_mixer = true;
    ImVec4 clear_color = ImVec4(0.20f, 0.09f, 0.14f, 0.95f);

    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = true;
    static bool no_move = false;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool unsaved_document = false;

    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;

    const char* waveforms[] = { "Sawtooth", "Sine", "Square", "Supersaw" };
    const char* notes[] = { "A0", "A#0", "B0",
        "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
        "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
        "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
        "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
        "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5" };

    float freqs[72]{ };
    for (size_t i = 0; i < 72; ++i) {
        freqs[i] = std::powf(2, (float)(i / 12.0));
    }

    float gui_global_amp{ 0.1f };

    float gui_oscA_amp{ 0.33f };
    float gui_oscB_amp{ 0.33f };
    float gui_oscC_amp{ 0.33f };
    std::vector<float*> amps {&gui_oscA_amp, & gui_oscB_amp, & gui_oscC_amp};

    float gui_oscA_lpi{ 1.0f };
    float gui_oscA_rpi{ 1.0f };
    float gui_oscB_lpi{ 1.0f };
    float gui_oscB_rpi{ 1.0f };
    float gui_oscC_lpi{ 1.0f };
    float gui_oscC_rpi{ 1.0f };
    std::vector<float*> lpis {&gui_oscA_lpi, & gui_oscB_lpi, & gui_oscC_lpi};
    std::vector<float*> rpis {&gui_oscA_rpi, & gui_oscB_rpi, & gui_oscC_rpi};

    float gui_oscA_pw{ 0.5f };
    float gui_oscB_pw{ 0.5f };
    float gui_oscC_pw{ 0.5f };
    std::vector<float*> pws {&gui_oscA_pw, & gui_oscB_pw, & gui_oscC_pw};

    std::atomic<bool> gui_updated { false };

    SetupImGuiStyle();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_wavetable_window) {
            ImGui::Begin("Wavetable Viewer", &show_wavetable_window, window_flags);
            float sum_table_L[TABLE_SIZE]{};
            for (std::size_t i = 0; i < TABLE_SIZE; ++i) {
                float tmp = 0;
                for (const auto& osc : st.oscillators) {
                    tmp += osc.first->interpolate_at(i * osc.first->ps.left_phase_inc) * osc.first->ps.amp;
                }
                sum_table_L[i] = tmp;
            }
            float sum_table_R[TABLE_SIZE]{};
            for (std::size_t i = 0; i < TABLE_SIZE; ++i) {
                float tmp = 0;
                for (const auto& osc : st.oscillators) {
                    tmp += osc.first->interpolate_at(i * osc.first->ps.right_phase_inc) * osc.first->ps.amp;
                }
                sum_table_R[i] = tmp;
            }

            ImGui::PlotLines("Wavetable Visualisation, L", sum_table_L, TABLE_SIZE, 0, nullptr, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
            ImGui::PlotLines("Wavetable Visualisation, R", sum_table_R, TABLE_SIZE, 0, nullptr, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
            ImGui::End();
        }

        const char* oscs[3] = { "A", "B", "C" };
        std::size_t osc_idx = 0;
        /////////////////////////////////////////////////////////////////
        for (auto& oscpair : st.oscillators) {
            Wavetable_t* osc = oscpair.first;
            LFO_t* lfo = oscpair.second;
            ImGui::Begin((std::string("Oscillator ") + std::string(oscs[osc_idx])).c_str(), &show_oscA, window_flags);
            ImGui::PlotLines("Waveform", osc->table, TABLE_SIZE, 0, nullptr, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
            ImGui::SeparatorText("Waveform Selector");
            if (ImGui::Combo("Waveform", (int*)&osc->ps.current_waveform, waveforms, IM_ARRAYSIZE(waveforms))) gui_updated = true;

            switch (osc->ps.current_waveform) {
            case 0:
                gen_saw_wave(*osc);
                break;
            case 1:
                gen_sin_wave(*osc);
                break;
            case 2:
                gen_sqr_wave(*osc, osc->ps.pulse_width);
                if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::DragFloat("Pulse Width", pws[osc_idx], 0.0025f, 0.0f, 1.0f)) gui_updated = true;
                }
                break;

            case 3:
                gen_sin_saw_wave(*osc);
                break;
            }
            if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Combo("L-Note", (int*)&osc->ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                    *lpis[osc_idx] = freqs[osc->ps.current_note_left];
                    gui_updated = true;
                }
                if (ImGui::Combo("R-Note", (int*)&osc->ps.current_note_right, notes, IM_ARRAYSIZE(notes))) {
                    *rpis[osc_idx] = freqs[osc->ps.current_note_right];
                    gui_updated = true;
                }
                if (ImGui::DragFloat("Left Phase Increment", lpis[osc_idx], 0.005f, 1, 20, "%f")) gui_updated = true;
                if (ImGui::DragFloat("Right Phase Increment", rpis[osc_idx], 0.005f, 1, 20, "%f")) gui_updated = true;
            }
            if (ImGui::CollapsingHeader("LFO Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Combo("LFO Waveform", (int*)&lfo->ps.current_waveform, waveforms, IM_ARRAYSIZE(waveforms))) gui_updated = true;
                switch (lfo->ps.current_waveform) {
                case 0:
                    gen_saw_wave(*lfo);
                    break;
                case 1:
                    gen_sin_wave(*lfo);
                    break;
                case 2:
                    gen_sqr_wave(*lfo, lfo->ps.pulse_width);
                    if (ImGui::CollapsingHeader("Square Settings"))
                    {
                        if (ImGui::DragFloat("Pulse Width", (float*)&lfo->ps.pulse_width, 0.0025f, 0.0f, 1.0f)) gui_updated = true;
                    }
                    break;

                case 3:
                    gen_sin_saw_wave(*lfo);
                    break;
                }
                if (ImGui::Checkbox("Enable LFO?", &lfo->lfo_enable)) gui_updated = true;
                if (ImGui::DragFloat("LFO Rate", (float*)&lfo->ps.left_phase_inc, 0.005f, 0.0f, 15.0f, "%f")) gui_updated = true;
                if (ImGui::DragFloat("LFO Amp Depth", &lfo->lfo_depth, 0.005f, -1.0f, 1.0f, "%f")) gui_updated = true;
                if (/*!lfo->lfo_enable ||*/ lfo->refresh_time == 0.0)
                    lfo->refresh_time = ImGui::GetTime();
                while (lfo->refresh_time < ImGui::GetTime())
                {
                    lfo->values[lfo->values_offset] = lfo->lfo_depth * lfo->interpolate_left();
                    lfo->values_offset = (lfo->values_offset + 1) % IM_ARRAYSIZE(lfo->values);
                    lfo->ps.left_phase += lfo->ps.left_phase_inc;
                    if (lfo->ps.left_phase >= TABLE_SIZE) lfo->ps.left_phase -= TABLE_SIZE;
                    if (!lfo->lfo_enable) lfo->ps.left_phase = 0;
                    lfo->refresh_time += 0.1f / 60.0f;
                }
                ImGui::PlotLines("Lines", lfo->values, IM_ARRAYSIZE(lfo->values), lfo->values_offset, "", -1.0f, 1.0f, ImVec2(200.0f, 100.0f));
            }
            ImGui::End();
            ++osc_idx;
        }

        if (show_osc_mixer) {
            ImGui::Begin("Volume Mixer", &show_osc_mixer, window_flags);
            const float spacing = 4;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
            if (ImGui::VSliderFloat("A", ImVec2(20, 160), &gui_oscA_amp, 0.0f, 0.5f, "")) gui_updated = true;
            ImGui::SameLine();
            if (ImGui::VSliderFloat("B", ImVec2(20, 160), &gui_oscB_amp, 0.0f, 0.5f, "")) gui_updated = true;
            ImGui::SameLine();
            if (ImGui::VSliderFloat("C", ImVec2(20, 160), &gui_oscC_amp, 0.0f, 0.5f, "")) gui_updated = true;
            ImGui::SameLine();
            ImGui::SameLine();
            if (ImGui::VSliderFloat("OUT", ImVec2(20, 160), &gui_global_amp, 0.0f, 0.5f, "")) gui_updated = true;
            ImGui::PopStyleVar();
            ImGui::End();
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Quit", "Alt+F4")) { glfwSetWindowShouldClose(window, 1); }
                if (ImGui::MenuItem("Save Config", "CTRL+S")) {}
                if (ImGui::MenuItem("Open Config", "CTRL+O")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows"))
            {
                if (ImGui::MenuItem("Welcome")) { show_intro_window = true; }
                if (ImGui::MenuItem("Wavetable Viewer", "CTRL+W")) { show_wavetable_window = true; }
                if (ImGui::MenuItem("Program Editor", "CTRL+E")) { show_synth_settings = true; }
                if (ImGui::MenuItem("Oscillator A")) { show_oscA = true; }
                if (ImGui::MenuItem("Oscillator B")) { show_oscB = true; }
                if (ImGui::MenuItem("Oscillator C")) { show_oscC = true; }
                if (ImGui::MenuItem("Volume Mixer")) { show_osc_mixer = true; }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (gui_updated) {
            st.m_oscA.ps.amp = gui_oscA_amp;
            st.m_oscB.ps.amp = gui_oscB_amp;
            st.m_oscC.ps.amp = gui_oscC_amp;
            st.m_oscA.ps.left_phase_inc = gui_oscA_lpi;
            st.m_oscA.ps.right_phase_inc = gui_oscA_rpi;
            st.m_oscB.ps.left_phase_inc = gui_oscB_lpi;
            st.m_oscB.ps.right_phase_inc = gui_oscB_rpi;
            st.m_oscC.ps.left_phase_inc = gui_oscC_lpi;
            st.m_oscC.ps.right_phase_inc = gui_oscC_rpi;
            st.m_oscA.ps.pulse_width = gui_oscA_pw;
            st.m_oscB.ps.pulse_width = gui_oscB_pw;
            st.m_oscC.ps.pulse_width = gui_oscC_pw;
            st.amplitude = gui_global_amp;
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        gui_updated = false;
    }

    st.close();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return paNoError;
}
