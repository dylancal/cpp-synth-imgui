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

#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)

SinWave_t sin_wave;
SawWave_t saw_wave;
SqrWave_t sqr_wave(0.2f);

class Synth
{
public:
    //
    // general
    Wavetable_t* m_wavetable;
    float m_amplitude{ 0.1f };
    //
    // SAW
    // //
    // SIN
    // //
    // SQR
    //
    float m_pulseWidth{ 0.5f };
public:
    Synth(Wavetable_t * def) : stream(0), left_phase(0), right_phase(0) { m_wavetable = def;  sprintf_s(message, "No Message"); }

    void generate_new_sqr() {
        m_wavetable -> generate(m_pulseWidth);
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
    bool close()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_CloseStream(stream);
        stream = 0;

        return (err == paNoError);
    }
    bool start()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StartStream(stream);

        return (err == paNoError);
    }
    bool stop()
    {
        if (stream == 0)
            return false;

        PaError err = Pa_StopStream(stream);

        return (err == paNoError);
    }
    void set_wavetable(Wavetable_t * def)
    {
        m_wavetable = def;
    }


private:
    int paCallbackMethod(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags)
    {
        float* out = (float*)outputBuffer;
        (void)timeInfo;
        (void)statusFlags;
        (void)inputBuffer;

        for (unsigned long i = 0; i < framesPerBuffer; i++)
        {
            *out++ = m_amplitude * (*m_wavetable)[left_phase]; // l
            *out++ = m_amplitude * (*m_wavetable)[right_phase]; // r
            left_phase += 4;
            if (left_phase >= TABLE_SIZE) left_phase -= TABLE_SIZE;
            right_phase += 4; /* higher pitch so we can distinguish left and right. */
            if (right_phase >= TABLE_SIZE) right_phase -= TABLE_SIZE;
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


    PaStream* stream;
    int left_phase;
    int right_phase;
    char message[20];
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

    Synth synth_test(&saw_wave);
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

            const char* waveforms[] = { "SAW", "SIN", "SQR" };
            float ADSR_envelope[4] = { 0.0f, 0.0f, 1.0f, 0.5f };
            float sample_length = 1.0f;
            int current_waveform = 0;
            int saw_supersaw_amt = 1;

            // Main loop
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                if (show_wavetable_window) {
                    ImGui::Begin("Wavetable Viewer", &show_intro_window);
                    ImGui::PlotLines("Duty Cycle Visualisation", synth_test.m_wavetable->table, 2048, 0, NULL, -1.0f, 1.0f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

                if (show_intro_window)
                {
                    ImGui::Begin("Wave Synthesis Tester - Dylan Callaghan", &show_intro_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                    ImGui::Text("This program provides some simple manipulation of primitive waveforms."
                        "\nThese waveforms are generated with simple math in a callback function\nand then added to an audio buffer");

                    if (ImGui::Button("Close Me"))
                        show_intro_window = false;
                    ImGui::End();
                }

                if (show_synth_settings)
                {
                    ImGui::Begin("Wave Synthesis Tester2 - Dylan Callaghan", &show_synth_settings, window_flags);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                    ImGui::Text("This program provides some simple manipulation of primitive waveforms."
                        "\nThese waveforms are generated with simple math in a callback function\nand then added to an audio buffer");

                    ImGui::SeparatorText("Waveform Selector");
                    ImGui::Combo("combo", &current_waveform, waveforms, IM_ARRAYSIZE(waveforms));

                    switch (current_waveform) {
                    case 0:
                        synth_test.set_wavetable(&saw_wave);
                        if (ImGui::CollapsingHeader("SAW Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05f, 0, 10);
                        }
                        break;
                    case 1:
                        synth_test.set_wavetable(&sin_wave);
                        if (ImGui::CollapsingHeader("SIN Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    case 2:
                        synth_test.set_wavetable(&sqr_wave);
                        if (ImGui::CollapsingHeader("SQR Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragFloat("Pulse Width", &synth_test.m_pulseWidth, 0.0025f, 0.0f, 1.0f);
                            /*float sq[100]{};
                            sq[0] = 0.0f;
                            sq[99] = 0.0f;
                            for (size_t t = 1; t <= 98; ++t) {
                                sq[t] = (t <= (int)100 * synth_test.m_pulseWidth) ? 1.0f : -1.0f;
                            }
                            ImGui::PlotLines("Duty Cycle Visualisation", sq, IM_ARRAYSIZE(sq), 0, NULL, -1.0f, 1.0f, ImVec2(100.0f, 100.0f));*/
                            if (ImGui::Button("Generate New Wavetable")) {
                                synth_test.generate_new_sqr();
                            }
                        }
                        break;
                    }

                    if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_None))
                    {
                        ImGui::DragFloat4("ADSR Envelope", ADSR_envelope, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("Output Volume", &synth_test.m_amplitude, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Sample Length (s)", &sample_length, 0.05f, 0.0f, 30.0f);
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
                        ImGui::EndMainMenuBar();
                    }

                    ImGui::End();
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
