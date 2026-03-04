#ifndef CV1_H
#define CV1_H

// Struktura stavu modulu
// V C++ má float 4 byty, int 4 byty.
// Offsets pro ASM:
// posY        = 0
// velY        = 4
// fuel        = 8
// isThrusting = 12
// status      = 16
struct LanderState {
    float posY;         // Výška (0.0 je střed, -0.9 je zem)
    float velY;         // Rychlost (kladná = nahoru, záporná = dolů)
    float fuel;         // Palivo (např. 100.0)
    int isThrusting;    // 1 = zapnutý motor, 0 = vypnutý
    int status;         // 0 = letí, 1 = přistál, 2 = havárie
};

#endif