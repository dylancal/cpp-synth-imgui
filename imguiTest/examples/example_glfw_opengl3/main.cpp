// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <cmath>
#include "portaudio.h"
#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (64)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (800)

struct Wavetable_t
{
    float table[TABLE_SIZE]{ 0 };
    float& operator[](int i) { return table[i]; }
};

struct SinWave_t : public Wavetable_t
{
    SinWave_t()
    {
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            table[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
        }
    }
};

struct SawWave_t : public Wavetable_t
{
    SawWave_t()
    {
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            table[i] = (float)i / TABLE_SIZE - 0.5f;
        }
    }
};

SinWave_t sin_wave;
SawWave_t saw_wave;

class Synth
{
public:

    Synth(Wavetable_t * def) : stream(0), left_phase(0), right_phase(0) { wavetable = def;  sprintf_s(message, "No Message"); }

    bool open(PaDeviceIndex index)
    {
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
        wavetable = def;
    }


private:
    /* The instance callback, where we have access to every method/variable in object of class Sine */
    int paCallbackMethod(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags)
    {
        float* out = (float*)outputBuffer;
        unsigned long i;
        float amplitude = 0.2f;
        (void)timeInfo; /* Prevent unused variable warnings. */
        (void)statusFlags;
        (void)inputBuffer;

        for (i = 0; i < framesPerBuffer; i++)
        {
            *out++ = amplitude * (*wavetable)[left_phase]; // l
            *out++ = amplitude * (*wavetable)[right_phase]; // r
            left_phase += 1;
            if (left_phase >= TABLE_SIZE) left_phase -= TABLE_SIZE;
            right_phase += 4; /* higher pitch so we can distinguish left and right. */
            if (right_phase >= TABLE_SIZE) right_phase -= TABLE_SIZE;
        }

        return paContinue;

    }

    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int paCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo * timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData)
    {
        /* Here we cast userData to Sine* type so we can call the instance method paCallbackMethod, we can do that since
           we called Pa_OpenStream with 'this' for userData */
        return ((Synth*)userData)->paCallbackMethod(inputBuffer, outputBuffer,
            framesPerBuffer,
            timeInfo,
            statusFlags);
    }


    void paStreamFinishedMethod()
    {
        printf("Stream Completed: %s\n", message);
    }
    static void paStreamFinished(void* userData)
    {
        return ((Synth*)userData)->paStreamFinishedMethod();
    }

    Wavetable_t* wavetable;
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

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    Synth synth_test(&saw_wave);
    ScopedPaHandler paInit;
    if (paInit.result() != paNoError) goto error;

    if (synth_test.open(Pa_GetDefaultOutputDevice()))
    {
        if (synth_test.start())
        {
            // Setup Dear ImGui context
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
            float amplitude = 1.0f;
            float sample_length = 1.0f;
            int current_waveform = 0;
            int sqr_pulse_width = 50;
            int saw_supersaw_amt = 1;

            // Main loop
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                ImGui::ShowDemoWindow();
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
                        if (ImGui::CollapsingHeader("SAW Settings", ImGuiTreeNodeFlags_None))
                        {
                            synth_test.set_wavetable(&saw_wave);
                            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05, 0, 10);
                        }
                        break;
                    case 1:
                        if (ImGui::CollapsingHeader("SIN Settings", ImGuiTreeNodeFlags_None))
                        {
                            synth_test.set_wavetable(&sin_wave);
                        }
                        break;
                    case 2:
                        if (ImGui::CollapsingHeader("SQR Settings", ImGuiTreeNodeFlags_None))
                        {
                            ImGui::DragInt("Pulse Width", &sqr_pulse_width, 0.05, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp);
                            float sq[100]{};
                            sq[0] = 0.0f;
                            sq[99] = 0.0f;
                            for (size_t t = 1; t <= 98; ++t) {
                                sq[t] = (t <= sqr_pulse_width) ? 1.0f : -1.0f;
                            }
                            ImGui::PlotLines("Duty Cycle Visualisation", sq, IM_ARRAYSIZE(sq), 0, NULL, -1.0f, 1.0f, ImVec2(100.0f, 100.0f));
                        }
                        break;
                    }


                    if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_None))
                    {
                        ImGui::DragFloat4("ADSR Envelope", ADSR_envelope, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("Output Volume", &amplitude, 0.0025f, 0.0f, 1.0f);
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return paNoError;

error:
    fprintf(stderr, "An error occurred while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", paInit.result());
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));

    return 1;
}
