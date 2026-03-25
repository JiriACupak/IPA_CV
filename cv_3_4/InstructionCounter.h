#pragma once

#include <x86intrin.h> // Pro GCC/MinGW (obsahuje __rdtsc a intrinsics)
#include <stdio.h>
#include <stdint.h>

class InstructionCounter
{
    unsigned long long start_count;
    unsigned long long end_count;

public:
    void start()
    {
        // LFENCE funguje jako bariéra, aby se všechny předchozí instrukce
        // dokončily předtím, než začneme měřit čas.
        _mm_lfence();
        start_count = __rdtsc();
        _mm_lfence();
    }

    void end()
    {
        _mm_lfence();
        end_count = __rdtsc();
        _mm_lfence();
    }

    unsigned long long getCyclesCount()
    {
        end();
        return end_count - start_count;
    }

    void print()
    {
        printf("%llu\n", getCyclesCount());
    }
};