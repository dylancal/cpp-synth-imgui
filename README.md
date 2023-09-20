# cpp-synth-imgui
Principles of wavetable synthesis. This project uses Dear Imgui, along with OpenGL ES, and GLFW in order to render the GUI.
![Screenshot 2023-06-26 170906](https://github.com/dylancal/cpp-synth-imgui/assets/51345001/f4a57362-3ea7-4cde-9b97-674279d46d4b)

A lookup-table synthesizer with 3 oscillators (with independent LFO), a volume mixer, and a wave visualiser.

This project is in its early stages, and as such contains many bugs and inconsistencies which I hope to document and fix soon.

# Installation
A `CMakeLists.txt` file is provided, but configured to use my own `vcpkg` toolchain file. Install dependencies with `vcpkg` and update the file path to follow your own toolchain file.

# `vcpkg` Dependencies
- `egl-registry`
- `glfw3`
- `imgui`
- `opengl-registry`
- `opengl`
- `portaudio`
- 
# Oscillators
![Screenshot 2023-06-26 171939](https://github.com/dylancal/cpp-synth-imgui/assets/51345001/f0999da3-28e2-4687-9882-7448f610a9be)

Each of the three oscillators has various options to tweak the sound.
- Waveform \
  The waveform adjusts the basic timbre of the sound. There are currently 4 waveforms: Sine, Square, Saw, and Triangle. The ability to import drawn waveforms will come later

- Waveform-specific Options \
  Some waveforms, such as square, have additional parameters that can be controlled such as the pulse width. These will become visible once the wave is selected

- LR-Note \
  As each the left and right channels of an oscillator work independently, this dropdown allows us to select the played note for both channels at once. This can also be fine
  tuned using the increment slider which allows to use any pitch

- Per-Channel Pitching \
  These options are similar, but allow us to pitch the left and right channels independently

- LFO Waveform \
  This allows us to select a waveform for the low frequency oscillator which can currently only affect the amplitude of the oscillator, although more parameters will be added soon.

- LFO Depth \
  This changes the extent to which the amplitude is affected by the LFO

# Volume Mixer
![Screenshot 2023-06-26 173306](https://github.com/dylancal/cpp-synth-imgui/assets/51345001/be79fed9-be13-4bdc-b2bd-adcd918592a6)

The volume mixer is very simple with 3 sliders to adjust the balance of the oscillators, along with an output slider to control master volume. There is also an "LFO Sync" button to
force each LFO to return to the start of its wavetable. This is useful for tempo-syncing polyrhythmic LFO rates.

# Wavetable Viewer
![Screenshot 2023-06-26 174239](https://github.com/dylancal/cpp-synth-imgui/assets/51345001/fddbc4c5-1334-499b-9b44-820d8fdec14e)

The wavetable viewer is also very simple, showing the interaction between each oscillator's waveforms and pitches (3 table sizes long). This is appoximate since it does not span the entire range of what will be output by the program.

