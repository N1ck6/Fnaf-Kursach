#pragma once

#include "MyOGL.h"
#include "Texture.h"
#include "ObjLoader.h"
#include "MyShaders.h"

template<typename T, int M, int N, int N2, int P>
void MatrixMultiply(const T* A, const T* B, T* C)
{
    for (int i = 0; i < M; i++)
        for (int j = 0; j < P; j++)
        {
            C[i * P + j] = (T)0;
            for (int k = 0; k < N; k++)
                C[i * P + j] += A[i * N + k] * B[k * P + j];
        }
}

void initRender();
void Render(double delta_time);
void onKeyPress(OpenGL* sender, KeyEventArg arg);
void onMouseMove(OpenGL* sender, MouseEventArg arg);

extern float full_time;
extern double delta_global;

extern OpenGL gl;

extern Texture texWall;
extern Texture texFloor;
extern Texture texBackWall;
extern Texture texDoorClosed;
extern Texture texDoorOpen;
extern Texture texFan;
extern Texture texAnimatronicHint;
extern Texture texStaticNoise;
extern Texture texCam1;
extern Texture texCam2;
extern Texture texCam3;
extern Texture texScreamer;
extern Texture texAnimatronicCorridor;
extern Texture texDoorLitEmpty;
extern Texture texDoorLitAnimatronic;

extern Shader phongTexShader;
extern Shader cameraNoiseShader;

extern bool rightDoorOpen;
extern bool leftDoorOpen;
extern float cameraYaw;
extern float animatronicTimer;

enum AnimatronicState
{
    STATE_STAGE,
    STATE_CORRIDOR,
    STATE_ATTACK
};

extern AnimatronicState animState;
extern float stateTimer;
extern bool gameOver;
extern bool rightLightOn;
extern bool leftLightOn;
extern bool monitorOpen;
extern int activeCam;
extern float camBlendFactor;
extern int prevCam;
extern bool camSwitching;
extern float camSwitchTimer;

extern bool testMode;
extern float cameraPitch;