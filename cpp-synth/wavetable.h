#pragma once
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iostream>
constexpr auto TABLE_SIZE = (872);
#ifndef M_PI
#define M_PI  (3.14159265)
#endif

struct OscSettings {
    std::atomic<float> amp { 0.33f };
    std::atomic<float> left_phase { 0 };
    std::atomic<float> right_phase { 0 };
    std::atomic<float> left_phase_inc { 1 };
    std::atomic<float> right_phase_inc { 1 };
    std::atomic<int> current_note_left { 1 };
    std::atomic<int> current_note_right { 1 };
    std::atomic<int> current_waveform { 2 };
    std::atomic<float> pulse_width { 0.5f };
};

struct Wavetable_t {
    OscSettings ps;
    std::atomic<float> table[TABLE_SIZE]{ 0 };
    std::atomic<float>& operator[](int i) { return table[i]; }
    float interpolate_at(float idx);
    float interpolate_left();
    float interpolate_right();
};

struct LFO_t : public Wavetable_t {
    float amps[90] { 0 };
    int amp_offset { 0 };
    double refresh_time { 0.0 };
    float lfo_amp { 0 };
    bool lfo_enable { false };
    float interpolate_amp();
};


void gen_sin_wave(Wavetable_t& table);
void gen_sin_wave(Wavetable_t* table);
void gen_saw_wave(Wavetable_t& table);
void gen_saw_wave(Wavetable_t* table);
void gen_sqr_wave(Wavetable_t& table, float pw);
void gen_sqr_wave(Wavetable_t* table);
void gen_tri_wave(Wavetable_t& table, float pw);
void gen_tri_wave(Wavetable_t* table, float pw);


float clip(float amp);
float half_f_add_one(float amp);
