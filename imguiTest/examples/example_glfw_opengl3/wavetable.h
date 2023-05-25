#pragma once
#include <cmath>
#define TABLE_SIZE   (2048)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
struct Wavetable_t
{
    float table[TABLE_SIZE]{ 0 };
    float& operator[](int i) { return table[i]; }
    virtual void generate(float pw) {};
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

    void generate(float pw) override {}
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

    void generate(float pw) override {}
};

struct SqrWave_t : public Wavetable_t
{
    SqrWave_t(float pw) { generate(pw); }

    void generate(float pw) override {
        for (int i = 0; i < (int)(TABLE_SIZE * pw); i++) { table[i] = 1.0f; }
        for (int i = (int)(TABLE_SIZE * pw); i < TABLE_SIZE; i++) { table[i] = -1.0f; }
    }
};
