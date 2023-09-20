#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <utility>
#include <thread>
#include "wavetable.h"
#include "imgui_includes.h"
#include "Synth.h"

// add pwm to lfo section
// move synth into its own header file
// move globals somewhere more useful
// add more useful comments
// change to more useful and consistent variable names
// add cent detuning

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**) {
    // window size stuff
    int display_w, display_h;

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
    
    // check that port audio streams are opened correctly with no errors
    if (paInit.result()) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", paInit.result());
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
        return 1;
    }

    if (!st.open(Pa_GetDefaultOutputDevice())) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        return 1;
    }

    if (!st.start()) {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        return 1;
    }

    // program can continue with starting imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // enable keyboard 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // enable gamepad 

   /* ImGui::StyleColorsDark();*/ // not needed since we are using a custom theme

    // set up for glfw + openGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // window states
    bool show_intro_window      = true;
    bool show_wavetable_window  = true;
    bool show_synth_settings    = true;
    bool show_oscA              = true;
    bool show_oscB              = true;
    bool show_oscC              = true;
    bool show_osc_mixer         = true;
    bool show_osc_scope         = true;

    // default window flags for use on all windows
    const bool no_titlebar            = false;
    const bool no_scrollbar           = false;
    const bool no_menu                = true;
    const bool no_move                = false;
    const bool no_resize              = false;
    const bool no_collapse            = false;
    const bool no_nav                 = false;
    const bool no_background          = false;
    const bool no_bring_to_front      = false;
    const bool unsaved_document       = false;

    // transparent background color
    ImVec4 clear_color = ImVec4(0.20f, 0.09f, 0.14f, 0.95f); 

    // apply window flags
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

    // wwaveform names for dropdown lists
    const char* waveforms[] = { "Sawtooth", "Sine", "Square", "Triangle"};

    // notes for dropdown list, index used for freq manipulation
    const char* notes[] = { "A0", "A#0", "B0",
        "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
        "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
        "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
        "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
        "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5" };

    // create array of frequencies
    float freqs[72]{ };
    for (std::size_t i = 0; i < 72; ++i) {
        freqs[i] = std::pow(2, (float)(i / 12.0));
    }

    // non atomic variables used for the gui
    // along with vectors containing pointers to them
    // for easy iteration later on
    float gui_global_amp{ 0.1f };
    float gui_oscA_amp{ 0.33f };
    float gui_oscB_amp{ 0.33f };
    float gui_oscC_amp{ 0.33f };
    float gui_oscA_lpi{ 1.0f };
    float gui_oscA_rpi{ 1.0f };
    float gui_oscB_lpi{ 1.0f };
    float gui_oscB_rpi{ 1.0f };
    float gui_oscC_lpi{ 1.0f };
    float gui_oscC_rpi{ 1.0f };
    float gui_oscA_pw{ 0.5f };
    float gui_oscB_pw{ 0.5f };
    float gui_oscC_pw{ 0.5f };
    std::vector<float*> gui_amplitudes {&gui_oscA_amp, & gui_oscB_amp, & gui_oscC_amp};
    std::vector<float*> gui_left_phase_incs {&gui_oscA_lpi, & gui_oscB_lpi, & gui_oscC_lpi};
    std::vector<float*> gui_right_phase_incs {&gui_oscA_rpi, & gui_oscB_rpi, & gui_oscC_rpi};
    std::vector<float*> pws {&gui_oscA_pw, & gui_oscB_pw, & gui_oscC_pw};

    // so we aren't updating variables every single frame
    std::atomic<bool> gui_updated { false };
    std::atomic<bool> pw_updated { false };

    float osc_scopes[300];
    int osc_scopes_offset = 0;
    double osc_refresh_time = 0;

    SetupImGuiStyle();
    while (!glfwWindowShouldClose(window))
    {
        // get ready for drawing GUI
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // this window shows the combined waveform from the 3 oscillators
        // with the correct amplitudes and pitches per channel
        if (show_wavetable_window) {
            const int viewer_width = 3;
            ImGui::Begin("Wavetable Viewer", &show_wavetable_window, window_flags);
            float sum_table_L[TABLE_SIZE * viewer_width]{};
            for (std::size_t i = 0; i < TABLE_SIZE * viewer_width; ++i) {
                float tmp = 0;
                for (const auto& osc : st.oscillators) {
                    tmp += osc.first->interpolate_at(i * osc.first->ps.left_phase_inc) * osc.first->ps.amp;
                }
                sum_table_L[i] = tmp;
            }
            float sum_table_R[TABLE_SIZE*4]{};
            for (std::size_t i = 0; i < TABLE_SIZE * viewer_width; ++i) {
                float tmp = 0;
                for (const auto& osc : st.oscillators) {
                    tmp += osc.first->interpolate_at(i * osc.first->ps.right_phase_inc) * osc.first->ps.amp;
                }
                sum_table_R[i] = tmp;
            }

            ImGui::PlotLines("L", sum_table_L, TABLE_SIZE * viewer_width, 0, NULL, -1.1f, 1.1f, ImVec2(viewer_width * 100.0f, 100.0f));
            ImGui::PlotLines("R", sum_table_R, TABLE_SIZE * viewer_width, 0, NULL, -1.1f, 1.1f, ImVec2(viewer_width * 100.0f, 100.0f));
            ImGui::End();
        }

        // idk why i have std::pair and shit and then also use an array and counter
        // just ignore the stupid shit
        // it would maybe make sense if the things were constructed _in_ the pair
        const char* oscs[3] = { "A", "B", "C" };
        std::size_t osc_idx = 0;

        // iteratively create oscillator windows, since they all function the same this is done
        // in a loop, saves writing it out 3 times
        for (auto& oscpair : st.oscillators) {
            Wavetable_t* osc = oscpair.first;
            LFO_t* lfo = oscpair.second;

            ImGui::Begin((std::string("Oscillator ") + std::string(oscs[osc_idx])).c_str(), &show_oscA, window_flags);
            ImGui::PlotLines("Waveform", (float*)osc->table, TABLE_SIZE, 0, nullptr, -1.1f, 1.1f, ImVec2(100.0f, 100.0f));
            ImGui::SeparatorText("Waveform");
            if (ImGui::Combo("Waveform", (int*)&osc->ps.current_waveform, waveforms, IM_ARRAYSIZE(waveforms)))
                gui_updated = true;

            switch (osc->ps.current_waveform) {
            case 0: // saw not special 
                gen_saw_wave(osc);
                break;
            case 1: // sin not special
                gen_sin_wave(osc);
                break;
            case 2: // square has a pulse width
                gen_sqr_wave(osc);
                if (ImGui::CollapsingHeader("Square Settings", ImGuiTreeNodeFlags_DefaultOpen))
                    if (ImGui::DragFloat("Pulse Width", pws[osc_idx], 0.0025f, 0.0f, 1.0f))
                        gui_updated = true;
                break;
            case 3:
                gen_tri_wave(osc, *pws[osc_idx]);
                if (ImGui::CollapsingHeader("Triangle Settings", ImGuiTreeNodeFlags_DefaultOpen))
                    if (ImGui::DragFloat("Duty Cycle", pws[osc_idx], 0.0025f, 0.0f, 1.0f))
                        gui_updated = true;
                break;
            }

            // settings such as per channel pitch
            ImGui::SeparatorText("General");
            //if (ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
            //{
                // change both pitches at once
                if (ImGui::Combo("LR-Note", (int*)&osc->ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                    osc->ps.current_note_right.store(osc->ps.current_note_left);
                    *gui_right_phase_incs[osc_idx] = freqs[osc->ps.current_note_right];
                    *gui_left_phase_incs[osc_idx] = freqs[osc->ps.current_note_left];
                    gui_updated = true;
                }
                if (ImGui::DragFloat("LR Increment", gui_left_phase_incs[osc_idx], 0.005f, 1, 20, "%f")) {
                    *gui_right_phase_incs[osc_idx] = *gui_left_phase_incs[osc_idx];

                    gui_updated = true;
                }
                if (ImGui::CollapsingHeader("Per-Channel Pitching")) {
                    if (ImGui::Combo("L-Note", (int*)&osc->ps.current_note_left, notes, IM_ARRAYSIZE(notes))) {
                        *gui_left_phase_incs[osc_idx] = freqs[osc->ps.current_note_left];
                        gui_updated = true;
                    }
                    // pitch fine tune
                    if (ImGui::DragFloat("Left Phase Increment", gui_left_phase_incs[osc_idx], 0.005f, 0.01f, 20.0f, "%f"))
                        gui_updated = true;
                    if (ImGui::Combo("R-Note", (int*)&osc->ps.current_note_right, notes, IM_ARRAYSIZE(notes))) {
                        *gui_right_phase_incs[osc_idx] = freqs[osc->ps.current_note_right];
                        gui_updated = true;
                    }
                    if (ImGui::DragFloat("Right Phase Increment", gui_right_phase_incs[osc_idx], 0.005f, 0.01f, 20.0f, "%f"))
                        gui_updated = true;
                }
                
            //}
            ImGui::SeparatorText("LFO");
            // low frequency oscillator, one per osc with its own waveform
            if (ImGui::CollapsingHeader("LFO Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Combo("LFO Waveform", (int*)&lfo->ps.current_waveform, waveforms, IM_ARRAYSIZE(waveforms)))
                    gui_updated = true;

                // switch similarly to the osc waveforms
                switch (lfo->ps.current_waveform) {
                case 0:
                    gen_saw_wave(*lfo);
                    break;
                case 1:
                    gen_sin_wave(*lfo);
                    break;
                case 2:
                    gen_sqr_wave(*lfo, lfo->ps.pulse_width);
                    if (ImGui::CollapsingHeader("Square Settings "))
                    {
                        if (ImGui::DragFloat("Pulse Width ", (float*)&lfo->ps.pulse_width, 0.0025f, 0.0f, 1.0f))
                            gui_updated = true;
                    }
                    break;
                case 3:
                    gen_tri_wave(*lfo, lfo->ps.pulse_width);
                    if (ImGui::CollapsingHeader("Triangle Settings "))
                    {
                        if (ImGui::DragFloat("Midpoint ", (float*)&lfo->ps.pulse_width, 0.0025f, 0.0f, 1.0f))
                            gui_updated = true;
                    }
                    break;
                }

                if (ImGui::Checkbox("Enable LFO?", &lfo->lfo_enable))
                    gui_updated = true;
                if (ImGui::DragFloat("LFO Rate", (float*)&lfo->ps.left_phase_inc, 0.005f, 0.0f, 15.0f, "%f"))
                    gui_updated = true;
                if (ImGui::DragFloat("LFO Amp Depth", &lfo->lfo_amp, 0.005f, -1.0f, 1.0f, "%f"))
                    gui_updated = true;
                    
                // whilst it is a terrible idea, currently the LFO is tied
                // to the GUI frame rate, and window actions such as dragging a window
                // will pause the LFO. this is along with other unintented side-effects
                // such as inconsistent pacing
                // the lfo needs to be untied from the GUI asap
                if (lfo->refresh_time == 0.0)
                    lfo->refresh_time = ImGui::GetTime();

                // animate and progress the LFO, looks cool but is terrible for pacing!
                // depending on how i feel i might keep this
                while (lfo->refresh_time < ImGui::GetTime())
                {
                    lfo->amps[lfo->amp_offset] = lfo->lfo_amp * lfo->interpolate_left();
                    lfo->amp_offset = (lfo->amp_offset + 1) % IM_ARRAYSIZE(lfo->amps);
                    lfo->ps.left_phase += lfo->ps.left_phase_inc;
                    if (lfo->ps.left_phase >= TABLE_SIZE) lfo->ps.left_phase -= TABLE_SIZE;
                    if (!lfo->lfo_enable) lfo->ps.left_phase = 0;
                    lfo->refresh_time += 0.1f / 60.0f;
                }
                ImGui::PlotLines("LFO", lfo->amps, IM_ARRAYSIZE(lfo->amps), lfo->amp_offset, "", -1.0f, 1.0f, ImVec2(200.0f, 100.0f));
            }
            ImGui::End();
            ++osc_idx;
        }

        if (show_osc_scope) {
            ImGui::Begin("Oscilloscope", &show_osc_scope, window_flags);
            if (osc_refresh_time == 0.0)
                osc_refresh_time = ImGui::GetTime();

            while (osc_refresh_time < ImGui::GetTime())
            {
                osc_scopes[osc_scopes_offset] = st.m_oscA.interpolate_left();
                osc_scopes_offset = (osc_scopes_offset + 1) % IM_ARRAYSIZE(osc_scopes);
                osc_refresh_time += 0.01f / 60.0f;
            }
            ImGui::PlotLines("Wave", osc_scopes, IM_ARRAYSIZE(osc_scopes), osc_scopes_offset, "", -1.0f, 1.0f, ImVec2(800.0f, 100.0f));
            ImGui::End();
        }

        // mixer window, for adjusting the mix of the oscilators
        // along with the global amplitude
        if (show_osc_mixer) {
            ImGui::Begin("Volume Mixer", &show_osc_mixer, window_flags);
            const float spacing = 4;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
            if (ImGui::VSliderFloat("A", ImVec2(20, 160), &gui_oscA_amp, 0.0f, 0.5f, ""))
                gui_updated = true;
            ImGui::SameLine();
            if (ImGui::VSliderFloat("B", ImVec2(20, 160), &gui_oscB_amp, 0.0f, 0.5f, ""))
                gui_updated = true;
            ImGui::SameLine();
            if (ImGui::VSliderFloat("C", ImVec2(20, 160), &gui_oscC_amp, 0.0f, 0.5f, ""))
                gui_updated = true;
            ImGui::SameLine();
            ImGui::SameLine();
            if (ImGui::VSliderFloat("OUT", ImVec2(20, 160), &gui_global_amp, 0.0f, 0.5f, ""))
                gui_updated = true;
            ImGui::PopStyleVar();
            if (ImGui::Button("LFO Sync", ImVec2(120, 20))) {
                st.m_lfoA.ps.left_phase.store(0);
                st.m_lfoB.ps.left_phase.store(0);
                st.m_lfoC.ps.left_phase.store(0);
            }
            if (ImGui::Button("Phase reset", ImVec2(120, 20))) {
                st.m_oscA.ps.left_phase.store(0);
                st.m_oscA.ps.right_phase.store(0);
                st.m_oscB.ps.left_phase.store(0);
                st.m_oscB.ps.right_phase.store(0);
                st.m_oscC.ps.left_phase.store(0);
                st.m_oscC.ps.right_phase.store(0);
            }

            ImGui::End();
        }

        // the menu bar, currently not really used at all apart from quitting
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit", "Alt+F4"))
                    glfwSetWindowShouldClose(window, 1);
                if (ImGui::MenuItem("Save Config", "CTRL+S"))
                {}
                if (ImGui::MenuItem("Open Config", "CTRL+O"))
                {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows")) {
                if (ImGui::MenuItem("Welcome"))
                    show_intro_window = true;
                if (ImGui::MenuItem("Wavetable Viewer", "CTRL+W"))
                    show_wavetable_window = true;
                if (ImGui::MenuItem("Program Editor", "CTRL+E"))
                    show_synth_settings = true;
                if (ImGui::MenuItem("Oscillator A"))
                    show_oscA = true;
                if (ImGui::MenuItem("Oscillator B"))
                    show_oscB = true;
                if (ImGui::MenuItem("Oscillator C"))
                    show_oscC = true;
                if (ImGui::MenuItem("Volume Mixer"))
                    show_osc_mixer = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // only when the GUI has actually been updated
        // then update the atomic variables in the oscillators
        if (gui_updated) {
            st.m_oscA.ps.amp.store(gui_oscA_amp);
            st.m_oscB.ps.amp.store(gui_oscB_amp);
            st.m_oscC.ps.amp.store(gui_oscC_amp);
            st.m_oscA.ps.left_phase_inc.store(gui_oscA_lpi);
            st.m_oscA.ps.right_phase_inc.store(gui_oscA_rpi);
            st.m_oscB.ps.left_phase_inc.store(gui_oscB_lpi);
            st.m_oscB.ps.right_phase_inc.store(gui_oscB_rpi);
            st.m_oscC.ps.left_phase_inc.store(gui_oscC_lpi);
            st.m_oscC.ps.right_phase_inc.store(gui_oscC_rpi);
            st.m_oscA.ps.pulse_width.store(gui_oscA_pw);
            st.m_oscB.ps.pulse_width.store(gui_oscB_pw);
            st.m_oscC.ps.pulse_width.store(gui_oscC_pw);
            st.amplitude.store(gui_global_amp);
        }

        // render all our shit 
        ImGui::Render();

        // set the window up for drawing
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // reset the gui check
        gui_updated.store(false);
    }

    st.close();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return paNoError;
}
