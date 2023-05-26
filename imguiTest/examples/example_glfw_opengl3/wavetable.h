#pragma once
#include <cmath>
#include <cstddef>
#include <iostream>
constexpr auto TABLE_SIZE = (872);
#ifndef M_PI
#define M_PI  (3.14159265)
#endif

struct OscSettings {
    // GENERAL
    float amp{ 0.05f };
    float left_phase{ 0 };
    float right_phase{ 0 };
    float left_phase_inc{ 1 };
    float right_phase_inc{ 1 };
    int current_note_left{ 1 };
    int current_note_right{ 1 };
    // SQR
    float pulse_width{ 0.5f };
};

struct Wavetable_t
{
    OscSettings ps;
    float table[TABLE_SIZE]{ 0 };
    float& operator[](int i) { return table[i]; }
    float interpolate_at(float idx) {
        float wl, fl;
        fl = std::modf(idx, &wl);
        return std::lerp(table[(int)wl], table[(int)wl + 1], fl);
    }
};

void gen_sin_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        table[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
    }
}

void gen_saw_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        table[i] = 2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f;
    }
}

void gen_sqr_wave(Wavetable_t& table, float pw) {
    for (int i = 0; i < (int)(TABLE_SIZE * pw); i++) { table[i] = 1.0f; }
    for (int i = (int)(TABLE_SIZE * pw); i < TABLE_SIZE; i++) { table[i] = -1.0f; }
}

void gen_ssaw_wave(Wavetable_t& table) {
    Wavetable_t tmp;
    gen_saw_wave(tmp);
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = tmp.interpolate_at((float)std::fmod(2*i, TABLE_SIZE));
    }
}

void gen_sin_saw_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        table[i] = ((float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.) +
            2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f)/2;
    }
}
