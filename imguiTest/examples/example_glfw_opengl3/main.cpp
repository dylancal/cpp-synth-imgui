// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "wavetable.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <cmath>
#include "portaudio.h"

#include <thread>
#include <chrono>
#include <iostream>

constexpr auto NUM_SECONDS = (5);
constexpr auto SAMPLE_RATE = (48000);
constexpr auto FRAMES_PER_BUFFER = (64);

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

public:
    Synth() {
        gen_saw_wave(m_oscA);
        gen_saw_wave(m_oscB);
        gen_saw_wave(m_oscC);
        sprintf_s(message, "No Message");

    }

    void generate_new_sqr(Wavetable_t &wavetable) {
        gen_sqr_wave(wavetable, wavetable.ps.pulse_width);
    }

    bool open(PaDeviceIndex index) {
        PaStreamParameters outputParameters;

        outputParameters.device = index;
        if (outputParameters.device == paNoDevice) {
            return false;
        }

        const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
        if (pInfo != 0)
        {
            printf("Output device name: '%s'\r", pInfo->name);
        }

        outputParameters.channelCount = 2;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        PaError err = Pa_OpenStream(
            &stream,
            NULL, /* no input */
            &outputParameters,
            SAMPLE_RATE,
            paFramesPerBufferUnspecified,
            0,      /* we won't output out of range samples so don't bother clipping them */
            &Synth::paCallback,
            this            /* Using 'this' for userData so we can cast to Sine* in paCallback method */
        );

        if (err != paNoError)
        {
            /* Failed to open stream to device !!! */
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

        for (unsigned long i = 0; i < framesPerBuffer; i++)
        {
            *out++ =    m_oscA.ps.amp * (m_oscA).interpolate_at(m_oscA.ps.left_phase) +
                        m_oscB.ps.amp * (m_oscB).interpolate_at(m_oscB.ps.left_phase);
            *out++ =    m_oscA.ps.amp * (m_oscA).interpolate_at(m_oscA.ps.right_phase) +
                        m_oscB.ps.amp * (m_oscB).interpolate_at(m_oscB.ps.right_phase);

            m_oscA.ps.left_phase += m_oscA.ps.left_phase_inc;
            m_oscB.ps.left_phase += m_oscB.ps.left_phase_inc;
            if (m_oscA.ps.left_phase >= TABLE_SIZE) m_oscA.ps.left_phase -= TABLE_SIZE;
            if (m_oscB.ps.left_phase >= TABLE_SIZE) m_oscB.ps.left_phase -= TABLE_SIZE;
            m_oscA.ps.right_phase += m_oscA.ps.right_phase_inc;
            m_oscB.ps.right_phase += m_oscB.ps.right_phase_inc;
            if (m_oscA.ps.right_phase >= TABLE_SIZE) m_oscA.ps.right_phase -= TABLE_SIZE;
            if (m_oscB.ps.right_phase >= TABLE_SIZE) m_oscB.ps.right_phase -= TABLE_SIZE;
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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    Wavetable_t saw_wave;
    gen_saw_wave(saw_wave);

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    Synth synth_test;
    ScopedPaHandler paInit;

    if (paInit.result() != paNoError) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", paInit.result());
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
        return 1;
    }

    //std::thread t1(&Synth::start, &synth_test);

    if (synth_test.open(Pa_GetDefaultOutputDevice()))
    {
        if (synth_test.start())
        {
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
            ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

            const char* waveforms[] = { "Sawtooth", "Sine", "Square", "Supersaw"};
            float ADSR_envelope[4] = { 0.0f, 0.0f, 1.0f, 0.5f };
            float sample_length = 1.0f;
            int current_waveformA = 0;
            int current_waveformB = 0;
            int current_waveformC = 0;
            int saw_supersaw_amt = 1;

            // Main loop
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                if (show_wavetable_window) {
                    ImGui::Begin("Wavetable Viewer", &show_wavetable_window, window_flags);
                    ImGui::PlotLines("Wavetable Visualisation", synth_test.m_oscA.table, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

                if (show_intro_window)
                {
                    ImGui::Begin("Wave Synthesis Tester - Dylan Callaghan", &show_intro_window, window_flags);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                    ImGui::Text("This program provides some simple manipulation of primitive waveforms."
                        "\nThese waveforms are generated with simple math in a callback function\nand then added to an audio buffer");

                    if (ImGui::Button("Close Me"))
                        show_intro_window = false;
                    ImGui::End();
                }



                //if (show_synth_settings)
                //{
                //    ImGui::Begin("Wave Synthesis Tester - Dylan Callaghan", &show_synth_settings, window_flags);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                //    ImGui::Text("This program provides some simple manipulation of primitive waveforms."
                //        "\nThese waveforms are generated with simple math in a callback function\nand then added to an audio buffer");

                //    ImGui::SeparatorText("Waveform Selector");
                //    ImGui::Combo("Waveform", &current_waveform, waveforms, IM_ARRAYSIZE(waveforms));

                //    switch (current_waveform) {
                //    case 0:
                //        gen_saw_wave(synth_test.m_oscA);
                //        if (ImGui::CollapsingHeader("Sawtooth Settings", ImGuiTreeNodeFlags_DefaultOpen))
                //        {
                //            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05f, 0, 10);
                //            ImGui::Button("Test");
                //            ImGui::SameLine();
                //            ImGui::Button("Test2");
                //        }
                //        break;
                //    case 1:
                //        gen_sin_wave(synth_test.m_oscA);
                //        if (ImGui::CollapsingHeader("Sine Settings", ImGuiTreeNodeFlags_DefaultOpen))
                //        {
                //        }
                //        break;
                //    case 2:
                //        gen_sqr_wave(synth_test.m_oscA, synth_test.m_pulseWidth);
                //        if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                //        {
                //            ImGui::DragFloat("Pulse Width", &synth_test.m_pulseWidth, 0.0025f, 0.0f, 1.0f);
                //        }
                //        break;

                //    case 3:
                //        gen_sin_saw_wave(synth_test.m_oscA);
                //        if (ImGui::CollapsingHeader("Supersaw Settings", ImGuiTreeNodeFlags_DefaultOpen))
                //        {
                //        }
                //        break;
                //    }

                if (show_oscA)
                {
                    ImGui::Begin("Oscillator A", &show_oscA, window_flags); 
                    ImGui::SeparatorText("Waveform Selector");
                    ImGui::Combo("Waveform", &current_waveformA, waveforms, IM_ARRAYSIZE(waveforms));

                    switch (current_waveformA) {
                    case 0:
                        gen_saw_wave(synth_test.m_oscA);
                        if (ImGui::CollapsingHeader("Sawtooth Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05f, 0, 10);
                        }
                        break;
                    case 1:
                        gen_sin_wave(synth_test.m_oscA);
                        if (ImGui::CollapsingHeader("Sine Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    case 2:
                        gen_sqr_wave(synth_test.m_oscA, synth_test.m_oscA.ps.pulse_width);
                        if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragFloat("Pulse Width", &synth_test.m_oscA.ps.pulse_width, 0.0025f, 0.0f, 1.0f);
                        }
                        break;

                    case 3:
                        gen_sin_saw_wave(synth_test.m_oscA);
                        if (ImGui::CollapsingHeader("Supersaw Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    }

                    if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::DragFloat("Output Volume", &synth_test.m_oscA.ps.amp, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Left Phase Increment", &synth_test.m_oscA.ps.left_phase_inc, 0.005f, 1, 20);
                        ImGui::DragFloat("Right Phase Increment", &synth_test.m_oscA.ps.right_phase_inc, 0.005f, 1, 20);
                    }
                    ImGui::PlotLines("Wavetable", synth_test.m_oscA.table, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

                if (show_oscB)
                {
                    ImGui::Begin("Oscillator B", &show_oscB, window_flags);
                    ImGui::SeparatorText("Waveform Selector");
                    ImGui::Combo("Waveform", &current_waveformB, waveforms, IM_ARRAYSIZE(waveforms));

                    switch (current_waveformB) {
                    case 0:
                        gen_saw_wave(synth_test.m_oscB);
                        if (ImGui::CollapsingHeader("Sawtooth Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05f, 0, 10);
                        }
                        break;
                    case 1:
                        gen_sin_wave(synth_test.m_oscB);
                        if (ImGui::CollapsingHeader("Sine Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    case 2:
                        gen_sqr_wave(synth_test.m_oscB, synth_test.m_oscB.ps.pulse_width);
                        if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragFloat("Pulse Width", &synth_test.m_oscB.ps.pulse_width, 0.0025f, 0.0f, 1.0f);
                        }
                        break;

                    case 3:
                        gen_sin_saw_wave(synth_test.m_oscB);
                        if (ImGui::CollapsingHeader("Supersaw Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    }

                    if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::DragFloat("Output Volume", &synth_test.m_oscB.ps.amp, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Left Phase Increment", &synth_test.m_oscB.ps.left_phase_inc, 0.005f, 1, 20);
                        ImGui::DragFloat("Right Phase Increment", &synth_test.m_oscB.ps.right_phase_inc, 0.005f, 1, 20);
                    }
                    ImGui::PlotLines("Wavetable", synth_test.m_oscB.table, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
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
                        ImGui::EndMenu();
                    }

                    ImGui::EndMainMenuBar();
                }

                // Rendering
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                glfwSwapBuffers(window);
            }
        }
        synth_test.close();
    }

    printf("Test finished.\n");
    //t1.join();
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return paNoError;
}
