/*
* AUTOR: [Jméno Studenta]
* LOGIN: [Login]
*/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <GL/freeglut.h>

#include "InstructionCounter.h"

#define M_PI 3.14159265358979323846
#define SPHERS_COUNT 512

double L = 320;                           // Velikost prostoru
double radius = 2.0;                      // Polomer
double minExtent[3], maxExtent[3];        // Hranice
int xWindowSize = 640, yWindowSize = 640; 
GLdouble aspectRatio;                     
GLdouble fovy, nearClip, farClip;         
GLdouble eye[3], center[3], up[3];        
GLuint sphereID, configID;                
int phi = 0, theta = 0;                   
int angle = 5;                            

// Pro jednoduchost alokujeme staticky a vyhneme se memory leaku
bool keyStates[256] = { false }; 

struct __attribute__((aligned(32))) t_sphere_array {
    float x[SPHERS_COUNT];
    float y[SPHERS_COUNT];
    float z[SPHERS_COUNT];
    float r[SPHERS_COUNT];
    float m[SPHERS_COUNT];
    float vectorX[SPHERS_COUNT];
    float vectorY[SPHERS_COUNT];
    float vectorZ[SPHERS_COUNT];
    uint32_t color[SPHERS_COUNT];
};

// Deklarace externi ASM funkce
extern "C" void nbody_simd(t_sphere_array * _spheres, int count, float dt);

enum Color { red=1, green, blue };

t_sphere_array spheres;

float dt = 0.08f;
unsigned long long counter = 2;
unsigned long long average = 0;
unsigned long long average_c = 0;

// --- DEKLARACE FUNKCI ---
void initView(double *minExtent, double *maxExtent);
void display();
void reshape(GLsizei width, GLsizei height);
void takeStep();
void createRenderList();
void makeSphere(GLuint sphereID, float radius);

void keyPressed(unsigned char key, int x, int y) { keyStates[key] = true; }
void keyUp(unsigned char key, int x, int y) { keyStates[key] = false; }

// Referencni (pomaly) C kod
void nbody()
{
    for (int i = 0; i < SPHERS_COUNT; i++)
    {
        for (int j = i + 1; j < SPHERS_COUNT; j++)
        {
            float dx = spheres.x[i] - spheres.x[j];
            float dy = spheres.y[i] - spheres.y[j];
            float dz = spheres.z[i] - spheres.z[j];

            float vector_length = sqrt(dx*dx + dy*dy + dz*dz);

            // Detekce kolize
            if (vector_length < (spheres.r[i] + spheres.r[j]))
            {
                float n_x_norm = dx / vector_length;
                float n_y_norm = dy / vector_length;
                float n_z_norm = dz / vector_length;

                float a1_dot = spheres.vectorX[i] * n_x_norm + spheres.vectorY[i] * n_y_norm + spheres.vectorZ[i] * n_z_norm;
                float a2_dot = spheres.vectorX[j] * n_x_norm + spheres.vectorY[j] * n_y_norm + spheres.vectorZ[j] * n_z_norm;

                float P = (2.0f * (a1_dot - a2_dot)) / (spheres.m[i] + spheres.m[j]);

                spheres.vectorX[i] -= P * spheres.m[j] * n_x_norm;  // spočítám a nahraju dle masky, všech 6 (7)
                spheres.vectorY[i] -= P * spheres.m[j] * n_y_norm;
                spheres.vectorZ[i] -= P * spheres.m[j] * n_z_norm;
                spheres.vectorX[j] += P * spheres.m[i] * n_x_norm;
                spheres.vectorY[j] += P * spheres.m[i] * n_y_norm;
                spheres.vectorZ[j] += P * spheres.m[i] * n_z_norm;

                spheres.color[i] = green;
            }
        }
        
        // Aktualizace pozic podle rychlosti
        spheres.x[i] += spheres.vectorX[i] * dt;
        spheres.y[i] += spheres.vectorY[i] * dt;
        spheres.z[i] += spheres.vectorZ[i] * dt;
    }
}

// DOPLNIT: Funkce volana v kazdem snimku
void takeStep() {
    InstructionCounter ticks;

 //    Stridame C a ASM pro overeni funkcnosti (pokud se kulicky tresou, ASM je spatne)
    if(counter % 2 == 0)
    {
        ticks.start();
        nbody_simd(&spheres, SPHERS_COUNT, dt);
        average += ticks.getCyclesCount();
    }
    else
    {
        ticks.start();
        nbody();
        average_c += ticks.getCyclesCount();
    }
    
    counter++;

    // Vypis statistik kazdych 100 iteraci, aby to nespamovalo konzoli
    if (counter % 100 == 0) {
        double asm_frac = (double)average / (counter / 2.0);
        double c_frac = (double)average_c / (counter / 2.0);
        double speedup = c_frac / asm_frac;

        printf("Cykly ASM: %.0f | Cykly C: %.0f | Zrychleni: %.2fx\n", asm_frac, c_frac, speedup);
    }

    createRenderList();
    glutPostRedisplay();
}

int main(int argc, char *argv[])
{
    // Inicializace pozic (zjednoduseno)
    for (int i = 0; i < SPHERS_COUNT; i++)
    {
        spheres.x[i] = (float)(rand() % 320) - 160;
        spheres.y[i] = (float)(rand() % 320) - 160;
        spheres.z[i] = (float)(rand() % 320) - 160;
    
        spheres.vectorX[i] = ((float)rand() / ((RAND_MAX) / 2)) - 1.0f;
        spheres.vectorY[i] = ((float)rand() / ((RAND_MAX) / 2)) - 1.0f;
        spheres.vectorZ[i] = ((float)rand() / ((RAND_MAX) / 2)) - 1.0f;

        spheres.r[i] = (float)((rand() % 5) + 1);
        spheres.m[i] = spheres.r[i] * ((rand() % 5) + 1);
        spheres.color[i] = blue; // Defaultni barva
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(xWindowSize, yWindowSize);
    glutCreateWindow("IPA - N-Body Simulation (SIMD)");

    for (int i = 0; i < 3; i++) {
        minExtent[i] = -L / 2;
        maxExtent[i] = L / 2;
    }

    initView(minExtent, maxExtent);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(takeStep);
    glutKeyboardFunc(keyPressed); 
    glutKeyboardUpFunc(keyUp);

    sphereID = glGenLists(1);
    makeSphere(sphereID, 1.0);
    configID = glGenLists(1);

    glutMainLoop();
    return 0;
}

// --- OPENGL FUNKCE (zde neni potreba pro studenty nic menit) ---

void initView(double *minExtent, double *maxExtent) {
    GLfloat lightDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat lightPosition[] = { 0.5, 0.5, 1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);

    double difExtent[3];
    for (int i = 0; i < 3; i++) difExtent[i] = maxExtent[i] - minExtent[i];
    
    double dist = sqrt(difExtent[0]*difExtent[0] + difExtent[1]*difExtent[1] + difExtent[2]*difExtent[2]);

    for (int i = 0; i < 3; i++) center[i] = minExtent[i] + difExtent[i] / 2;

    eye[0] = center[0];
    eye[1] = center[1];
    eye[2] = center[2] + dist; 
    up[0] = 0; up[1] = 1; up[2] = 0;

    nearClip = (dist - difExtent[2] / 2.0) / 2.0;
    farClip = 2.0 * (dist + difExtent[2] / 2.0);

    fovy = difExtent[1] / (dist - difExtent[2] / 2.0) / 2.0;
    fovy = 2.0 * atan(fovy) / M_PI * 180.0;
    fovy *= 1.2;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
    glCallList(configID);
    glutSwapBuffers();
}

void reshape(GLsizei width, GLsizei height) {
    if (height == 0) height = 1;
    aspectRatio = width / double(height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, aspectRatio, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void createRenderList() {
    glNewList(configID, GL_COMPILE);
    
    for (int i = 0; i < SPHERS_COUNT; i++) {
        glPushMatrix();
        if (spheres.color[i] == red) glColor3f(1.0, 0.0, 0.0);
        else if (spheres.color[i] == green) glColor3f(0.0, 1.0, 0.0);
        else glColor3f(0.0, 0.0, 1.0); // Default modra

        glTranslated(spheres.x[i], spheres.y[i], spheres.z[i]);
        glScaled(spheres.r[i], spheres.r[i], spheres.r[i]);
        glCallList(sphereID);
        glPopMatrix();
    }

    glColor3ub(255, 255, 255);
    glutWireCube(L);
    glEndList();
}

void makeSphere(GLuint sphereID, float radius) {
    glNewList(sphereID, GL_COMPILE);
    glutSolidSphere(radius, 18, 9);
    glEndList();
}