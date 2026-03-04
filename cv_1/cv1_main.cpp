#include <iostream>
#include <windows.h>
#include <GL/gl.h>
#include "GL/freeglut.h"
#include <string>
#include <cstdio>
#include "cv1.h"

// =============================================================
// NASTAVENÍ:
// Odkomentujte pro použití ASM (DLL).
// Zakomentujte pro použití C++ implementace.
// =============================================================
#define USE_ASM_IMPL
// =============================================================

// Definice pointeru na funkci
// RCX = state, XMM1 = dt (v C++ to kompilátor vyřeší sám)
typedef void(*UpdateLanderFunc)(LanderState* state, float dt);

LanderState lander;
UpdateLanderFunc _UpdateLander = nullptr;

// Konstanty okna
const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;

// -------------------------------------------------------------
// IMPLEMENTACE FYZIKY V C++
// -------------------------------------------------------------
#ifndef USE_ASM_IMPL
void updateLanderCPP(LanderState* state, float dt) {
    // Konstanty (musí odpovídat těm v ASM souboru)
    const float gravity = 0.3f;
    const float thrust = 0.8f;
    const float groundY = -0.9f;
    const float maxSafeVel = -0.3f; // Rychlost je záporná, takže limit je "větší" číslo (blíže k 0)
    const float fuelCons = 20.0f;

    // 1. Gravitace
    state->velY -= gravity * dt;

    // 2. Motor a Palivo
    if (state->isThrusting == 1 && state->fuel > 0.0f) {
        state->velY += thrust * dt;
        state->fuel -= fuelCons * dt;
    }

    // 3. Pozice
    state->posY += state->velY * dt;

    // 4. Kolize se zemí
    if (state->posY <= groundY) {
        // Zastavení na zemi
        state->posY = groundY;

        // Kontrola rychlosti
        // Padáme dolů -> rychlost je např. -0.5. Limit je -0.3.
        // Pokud (-0.5 < -0.3), je to náraz.
        if (state->velY < maxSafeVel) {
            state->status = 2; // Havárie
        } else {
            state->status = 1; // Úspěch
        }
    }
}
#endif
// -------------------------------------------------------------


// Vykreslení textu na obrazovku
void drawText(float x, float y, std::string text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // --- VYKRESLENÍ ZEMĚ ---
    glColor3f(0.0f, 1.0f, 0.0f); // Zelená
    glBegin(GL_LINES);
    glVertex2f(-1.0f, -0.9f);
    glVertex2f(1.0f, -0.9f);
    glEnd();

    // --- VYKRESLENÍ RAKETY ---
    glPushMatrix();
    glTranslatef(0.0f, lander.posY, 0.0f);

    // Tělo rakety
    if (lander.status == 2) glColor3f(1.0f, 0.0f, 0.0f); // Červená při výbuchu
    else if (lander.status == 1) glColor3f(0.0f, 1.0f, 0.0f); // Zelená při přistání
    else glColor3f(0.8f, 0.8f, 1.0f); // Modrá při letu

    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.0f);
    glVertex2f(0.05f, 0.0f);
    glVertex2f(0.05f, 0.1f);
    glVertex2f(-0.05f, 0.1f);
    glEnd();

    // Plamen motoru (jen pokud letí, má palivo a drží se klávesa)
    if (lander.status == 0 && lander.isThrusting && lander.fuel > 0) {
        glColor3f(1.0f, 0.5f, 0.0f); // Oranžová
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.03f, 0.0f);
        glVertex2f(0.03f, 0.0f);
        glVertex2f(0.0f, -0.05f - (rand() % 10) / 200.0f); // Blikání
        glEnd();
    }
    glPopMatrix();

    // --- VYKRESLENÍ GUI ---
    glColor3f(1.0f, 1.0f, 1.0f);
    char buffer[100];
    sprintf(buffer, "Palivo: %.1f", lander.fuel);
    drawText(-0.95f, 0.9f, buffer);

    sprintf(buffer, "Rychlost: %.3f", lander.velY);
    drawText(-0.95f, 0.8f, buffer);

    // Zobrazení módu (ASM vs C++)
    #ifdef USE_ASM_IMPL
        drawText(0.7f, 0.9f, "[Mode: ASM DLL]", GLUT_BITMAP_HELVETICA_12);
    #else
        drawText(0.7f, 0.9f, "[Mode: Internal C++]", GLUT_BITMAP_HELVETICA_12);
    #endif

    if (lander.status == 1) {
        glColor3f(0.0f, 1.0f, 0.0f);
        drawText(-0.2f, 0.0f, "USPESNE PRISTANI!", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(-0.25f, -0.15f, "Stiskni 'R' pro restart");
    }
    else if (lander.status == 2) {
        glColor3f(1.0f, 0.0f, 0.0f);
        drawText(-0.1f, 0.0f, "HAVARIE!", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(-0.25f, -0.15f, "Stiskni 'R' pro restart");
    }

    glutSwapBuffers();
}

void timer(int value) {
    if (_UpdateLander && lander.status == 0) {
        // Volání aktualizační funkce (ASM nebo C++)
        _UpdateLander(&lander, 0.016f);
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Reset hry
void resetGame() {
    lander.posY = 0.8f;
    lander.velY = 0.0f;
    lander.fuel = 100.0f;
    lander.isThrusting = 0;
    lander.status = 0;
}

void keyboard(unsigned char key, int x, int y) {
    if (key == ' ') lander.isThrusting = 1;
    if (key == 'r' || key == 'R') resetGame();
    if (key == 27) exit(0); // ESC
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == ' ') lander.isThrusting = 0;
}

int main(int argc, char** argv) {
    HINSTANCE hInstLibrary = NULL;

    // 1. Rozhodnutí zda načíst DLL nebo použít C++
    #ifdef USE_ASM_IMPL
        std::cout << "Loading ASM implementation from cv1.dll..." << std::endl;
        hInstLibrary = LoadLibrary("cv1.dll");
        if (!hInstLibrary) {
            std::cerr << "Chyba: Nelze nacist cv1.dll!" << std::endl;
            return 1;
        }
        _UpdateLander = (UpdateLanderFunc)GetProcAddress(hInstLibrary, "updateLander");
        if (!_UpdateLander) {
            std::cerr << "Chyba: Funkce updateLander nenalezena v DLL!" << std::endl;
            return 1;
        }
    #else
        std::cout << "Using internal C++ implementation." << std::endl;
        _UpdateLander = updateLanderCPP;
    #endif

    // 2. Inicializace hry
    resetGame();

    // 3. GLUT Init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
    glutCreateWindow("ASM Lunar Lander");
    
    // Nastavení callbacků
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);

    // Spuštění smyčky
    glutMainLoop();

    // Úklid DLL (pokud byla načtena)
    #ifdef USE_ASM_IMPL
        if (hInstLibrary) FreeLibrary(hInstLibrary);
    #endif

    return 0;
}
