#include "wavetable.h"

float Wavetable_t::interpolate_at(float idx) {
    float wl, fl;
    fl = std::modf(idx, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

float Wavetable_t::interpolate_left() {
    float wl, fl;
    fl = std::modf(ps.left_phase, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

float Wavetable_t::interpolate_right() {
    float wl, fl;
    fl = std::modf(ps.right_phase, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

float LFO_t::interpolate_amp() {
    float wl, fl;
    fl = std::modf(ps.left_phase, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

void gen_sin_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
    }
}

void gen_sin_wave(Wavetable_t* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        (*table)[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
    }
}

void gen_saw_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = 2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f;
    }
}

void gen_saw_wave(Wavetable_t* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        (*table)[i] = 2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f;
    }
}

void gen_sqr_wave(Wavetable_t& table, float pw) {
    for (int i = 0; i < (int)(TABLE_SIZE * pw); i++) { table[i] = 1.0f; }
    for (int i = (int)(TABLE_SIZE * pw); i < TABLE_SIZE; i++) { table[i] = -1.0f; }
}

void gen_sqr_wave(Wavetable_t* table) {
    for (int i = 0; i < (int)(TABLE_SIZE * table->ps.pulse_width); i++) { (*table)[i] = 1.0f; }
    for (int i = (int)(TABLE_SIZE * table->ps.pulse_width); i < TABLE_SIZE; i++) { (*table)[i] = -1.0f; }
}

void gen_ssaw_wave(Wavetable_t& table) {
    Wavetable_t tmp;
    gen_saw_wave(tmp);
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = tmp.interpolate_at((float)std::fmod(2 * i, TABLE_SIZE));
    }
}

void gen_sin_saw_wave(Wavetable_t& table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        table[i] = ((float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.) +
            2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f) / 2;
    }
}

float clip(float amp) {
    if (amp < 0) return 0;
    else if (amp > 1) return 1;
    else return amp;
}

float half_f_add_one(float amp) {
    return 0.5f * amp + 1;
}


