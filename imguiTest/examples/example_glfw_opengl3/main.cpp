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
                    ImGui::DragInt("Supersaw amount", &saw_supersaw_amt, 0.05, 0, 10);
                }
                break;
            case 1:
                if (ImGui::CollapsingHeader("SIN Settings", ImGuiTreeNodeFlags_None))
                {
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
