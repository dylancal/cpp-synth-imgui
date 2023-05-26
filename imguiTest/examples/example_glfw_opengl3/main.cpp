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
#include <vector>
#include <algorithm>

constexpr auto NUM_SECONDS = (5);
constexpr auto SAMPLE_RATE = (48000);

void SetupImGuiStyle()
{
    // Cherry style by r-lyeh from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(6.0f, 3.0f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(5.0f, 1.0f);
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(7.0f, 1.0f);
    style.ItemInnerSpacing = ImVec2(1.0f, 1.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 6.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 13.0f;
    style.ScrollbarRounding = 16.0f;
    style.GrabMinSize = 20.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.TabBorderSize = 1.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 0.8799999952316284f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 0.2800000011920929f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1294117718935013f, 0.1372549086809158f, 0.168627455830574f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 0.8999999761581421f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.5372549295425415f, 0.47843137383461f, 0.2549019753932953f, 0.1620000004768372f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.7799999713897705f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2313725501298904f, 0.2000000029802322f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 0.75f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 0.4699999988079071f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08627451211214066f, 0.1490196138620377f, 0.1568627506494522f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.7799999713897705f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7098039388656616f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4666666686534882f, 0.7686274647712708f, 0.8274509906768799f, 0.1400000005960464f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7098039388656616f, 0.2196078449487686f, 0.2666666805744171f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.4666666686534882f, 0.7686274647712708f, 0.8274509906768799f, 0.1400000005960464f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.8600000143051147f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.7599999904632568f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.8600000143051147f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176f, 0.07450980693101883f, 0.2549019753932953f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4666666686534882f, 0.7686274647712708f, 0.8274509906768799f, 0.03999999910593033f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.7799999713897705f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 0.6299999952316284f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8588235378265381f, 0.929411768913269f, 0.886274516582489f, 0.6299999952316284f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4549019634723663f, 0.196078434586525f, 0.2980392277240753f, 0.4300000071525574f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
    //style.Alpha = 0.5f;
    style.ScaleAllSizes(3);
}

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
    std::vector<Wavetable_t*> oscillators { & m_oscA, & m_oscB, & m_oscC };
    float amplitude{ 0.1f };

public:
    Synth() {
        gen_saw_wave(m_oscA);
        gen_saw_wave(m_oscB);
        gen_saw_wave(m_oscC);
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
        outputParameters.hostApiSpecificStreamInfo = NULL;

        PaError err = Pa_OpenStream(
            &stream,
            NULL, 
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

        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            //auto updatePhases = [&out](Wavetable_t* wt) {
            //    *out++ = wt->ps.amp * wt->interpolate_at(wt->ps.left_phase);
            //    *out++ = wt->ps.amp * wt->interpolate_at(wt->ps.right_phase);
            //    wt->ps.left_phase += wt->ps.left_phase_inc;
            //    if (wt->ps.left_phase >= TABLE_SIZE) wt->ps.left_phase -= TABLE_SIZE;
            //    wt->ps.right_phase += wt->ps.right_phase_inc;
            //    if (wt->ps.right_phase >= TABLE_SIZE) wt->ps.right_phase -= TABLE_SIZE;
            //};
            //std::for_each(oscillators.begin(), oscillators.end(), updatePhases);
            *out++ =    amplitude * (m_oscA.ps.amp * (m_oscA).interpolate_at(m_oscA.ps.left_phase) +
                                m_oscB.ps.amp * (m_oscB).interpolate_at(m_oscB.ps.left_phase) +
                                m_oscC.ps.amp * (m_oscC).interpolate_at(m_oscC.ps.left_phase));
            *out++ =    amplitude * (m_oscA.ps.amp * (m_oscA).interpolate_at(m_oscA.ps.right_phase) +
                                m_oscB.ps.amp * (m_oscB).interpolate_at(m_oscB.ps.right_phase) +
                                m_oscC.ps.amp * (m_oscC).interpolate_at(m_oscC.ps.right_phase));
            m_oscA.ps.left_phase += m_oscA.ps.left_phase_inc;
            m_oscB.ps.left_phase += m_oscB.ps.left_phase_inc;
            m_oscC.ps.left_phase += m_oscC.ps.left_phase_inc;
            if (m_oscA.ps.left_phase >= TABLE_SIZE) m_oscA.ps.left_phase -= TABLE_SIZE;
            if (m_oscB.ps.left_phase >= TABLE_SIZE) m_oscB.ps.left_phase -= TABLE_SIZE;
            if (m_oscC.ps.left_phase >= TABLE_SIZE) m_oscC.ps.left_phase -= TABLE_SIZE;
            m_oscA.ps.right_phase += m_oscA.ps.right_phase_inc;
            m_oscB.ps.right_phase += m_oscB.ps.right_phase_inc;
            m_oscC.ps.right_phase += m_oscC.ps.right_phase_inc;
            if (m_oscA.ps.right_phase >= TABLE_SIZE) m_oscA.ps.right_phase -= TABLE_SIZE;
            if (m_oscB.ps.right_phase >= TABLE_SIZE) m_oscB.ps.right_phase -= TABLE_SIZE;
            if (m_oscC.ps.right_phase >= TABLE_SIZE) m_oscC.ps.right_phase -= TABLE_SIZE;
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
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

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
    std::cout << std::pow(2.0f, 1 / 12.0);
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

            float freqs[72] { };
            for (size_t i = 0; i < 72; ++i) {
                freqs[i] = std::powf(2, (float) (i / 12.0));
            }

            int current_waveformA = 0;
            int current_waveformB = 0;
            int current_waveformC = 0;
            int saw_supersaw_amt = 1;

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
                        for (const auto& osc : synth_test.oscillators) {
                            tmp += osc->interpolate_at(i*osc->ps.left_phase_inc)*osc->ps.amp;
                        }
                        sum_table_L[i] = tmp;
                    }
                    float sum_table_R[TABLE_SIZE]{};
                    for (std::size_t i = 0; i < TABLE_SIZE; ++i) {
                        float tmp = 0;
                        for (const auto& osc : synth_test.oscillators) {
                            tmp += osc->interpolate_at(i * osc->ps.right_phase_inc) * osc->ps.amp;
                        }
                        sum_table_R[i] = tmp;
                    }
                    
                    ImGui::PlotLines("Wavetable Visualisation, L", sum_table_L, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::PlotLines("Wavetable Visualisation, R", sum_table_R, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

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
                        if (ImGui::Combo("L-Note", &synth_test.m_oscA.ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscA.ps.left_phase_inc = freqs[synth_test.m_oscA.ps.current_note_left];
                        }
                        if (ImGui::Combo("R-Note", &synth_test.m_oscA.ps.current_note_right, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscA.ps.right_phase_inc = freqs[synth_test.m_oscA.ps.current_note_right];
                        }
                        ImGui::DragFloat("Output Volume", &synth_test.m_oscA.ps.amp, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Left Phase Increment", &synth_test.m_oscA.ps.left_phase_inc, 0.005f, 1, 20, "%f");
                        ImGui::DragFloat("Right Phase Increment", &synth_test.m_oscA.ps.right_phase_inc, 0.005f, 1, 20, "%f");
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
                        if (ImGui::Combo("L-Note", &synth_test.m_oscB.ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscB.ps.left_phase_inc = freqs[synth_test.m_oscB.ps.current_note_left];
                        }
                        if (ImGui::Combo("R-Note", &synth_test.m_oscB.ps.current_note_right, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscB.ps.right_phase_inc = freqs[synth_test.m_oscB.ps.current_note_right];
                        }
                        ImGui::DragFloat("Output Volume", &synth_test.m_oscB.ps.amp, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Left Phase Increment", &synth_test.m_oscB.ps.left_phase_inc, 0.005f, 1, 20, "%f");
                        ImGui::DragFloat("Right Phase Increment", &synth_test.m_oscB.ps.right_phase_inc, 0.005f, 1, 20, "%f");
                    }
                    ImGui::PlotLines("Wavetable", synth_test.m_oscB.table, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

                if (show_oscC)
                {
                    ImGui::Begin("Oscillator C", &show_oscC, window_flags);
                    ImGui::SeparatorText("Waveform Selector");
                    ImGui::Combo("Waveform", &current_waveformC, waveforms, IM_ARRAYSIZE(waveforms));

                    switch (current_waveformC) {
                    case 0:
                        gen_saw_wave(synth_test.m_oscC);
                        if (ImGui::CollapsingHeader("Sawtooth Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05f, 0, 10);
                        }
                        break;
                    case 1:
                        gen_sin_wave(synth_test.m_oscC);
                        if (ImGui::CollapsingHeader("Sine Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    case 2:
                        gen_sqr_wave(synth_test.m_oscC, synth_test.m_oscC.ps.pulse_width);
                        if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::DragFloat("Pulse Width", &synth_test.m_oscC.ps.pulse_width, 0.0025f, 0.0f, 1.0f);
                        }
                        break;

                    case 3:
                        gen_sin_saw_wave(synth_test.m_oscC);
                        if (ImGui::CollapsingHeader("Supersaw Settings", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                        }
                        break;
                    }

                    if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (ImGui::Combo("L-Note", &synth_test.m_oscC.ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscC.ps.left_phase_inc = freqs[synth_test.m_oscC.ps.current_note_left];
                        }
                        if (ImGui::Combo("R-Note", &synth_test.m_oscC.ps.current_note_right, notes, IM_ARRAYSIZE(notes))) {
                            synth_test.m_oscC.ps.right_phase_inc = freqs[synth_test.m_oscC.ps.current_note_right];
                        }
                        ImGui::DragFloat("Output Volume", &synth_test.m_oscC.ps.amp, 0.0025f, 0.0f, 1.0f);
                        ImGui::DragFloat("Left Phase Increment", &synth_test.m_oscC.ps.left_phase_inc, 0.005f, 1, 20, "%f");
                        ImGui::DragFloat("Right Phase Increment", &synth_test.m_oscC.ps.right_phase_inc, 0.005f, 1, 20, "%f");
                    }
                    ImGui::PlotLines("Wavetable", synth_test.m_oscC.table, TABLE_SIZE, 0, NULL, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
                    ImGui::End();
                }

                if (show_osc_mixer) {
                    ImGui::Begin("Volume Mixer", &show_osc_mixer, window_flags);
                    const float spacing = 4;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
                    ImGui::VSliderFloat("A", ImVec2(20, 160), &synth_test.m_oscA.ps.amp, 0.0f, 0.5f, "");
                    ImGui::SameLine();
                    ImGui::VSliderFloat("B", ImVec2(20, 160), &synth_test.m_oscB.ps.amp, 0.0f, 0.5f, "");
                    ImGui::SameLine();
                    ImGui::VSliderFloat("C", ImVec2(20, 160), &synth_test.m_oscC.ps.amp, 0.0f, 0.5f, "");
                    ImGui::SameLine();
                    ImGui::SameLine();
                    ImGui::VSliderFloat("OUT", ImVec2(20, 160), &synth_test.amplitude, 0.0f, 0.5f, "");
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
