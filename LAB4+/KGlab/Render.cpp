#include "Render.h"
#include <cstdlib>
#include <string>
#include <windows.h>

#pragma comment(lib, "winmm.lib")

float full_time = 0.0f;
double delta_global = 0.0;

Texture texWall;
Texture texFloor;
Texture texBackWall;
Texture texDoorClosed;
Texture texDoorOpen;
Texture texFan;
Texture texAnimatronicHint;
Texture texStaticNoise;
Texture texCam1;
Texture texCam2;
Texture texCam3;
Texture texScreamer;
Texture texAnimatronicCorridor;
Texture texDoorLitEmpty;
Texture texDoorLitAnimatronic;

Shader phongTexShader;
Shader cameraNoiseShader;

bool rightDoorOpen = true;
bool leftDoorOpen = true;
float cameraYaw = 0.0f;
float animatronicTimer = 0.0f;

AnimatronicState animState = STATE_STAGE;
float stateTimer = 0.0f;
bool gameOver = false;
bool rightLightOn = false;
bool leftLightOn = false;
bool monitorOpen = false;
int activeCam = 1;
float camBlendFactor = 0.0f;
int prevCam = 1;
bool camSwitching = false;
float camSwitchTimer = 0.0f;

bool testMode = false;
float cameraPitch = 0.0f;
float testCamX = 0.0f;
float testCamY = 1.5f;
float testCamZ = -0.5f;
int prevMouseX = -1;
int prevMouseY = -1;
bool skipNextWarp = false;
float hoverMouseX = -1.0f;

int animatronicSide = 0;
float corridorTimer = 0.0f;
float doorCloseTimer = 0.0f;
float flashTimer = 0.0f;
int flashSide = 0;
float gameOverTimer = 0.0f;
int rightDoorOpenCount = 0;
int leftDoorOpenCount = 0;

float rightDoorClosedTimer = 0.0f;
float leftDoorClosedTimer = 0.0f;

bool musicPlaying = false;
static int sfxCounter = 0;

void playSoundAsync(const char* filename)
{
    char cmd[512];
    char alias[64];

    snprintf(alias, sizeof(alias), "sfx_%d", sfxCounter++);

    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", filename, alias);

    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        snprintf(cmd, sizeof(cmd), "play %s", alias);
        mciSendStringA(cmd, NULL, 0, NULL);
    }
}

void playSound(const char* filename)
{
    char cmd[512];
    char alias[64];

    snprintf(alias, sizeof(alias), "sfx_block_%d", sfxCounter++);
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias %s", filename, alias);

    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        snprintf(cmd, sizeof(cmd), "play %s wait", alias);
        mciSendStringA(cmd, NULL, 0, NULL);

        snprintf(cmd, sizeof(cmd), "close %s", alias);
        mciSendStringA(cmd, NULL, 0, NULL);
    }
}

void startMusic()
{
    if (musicPlaying) return;
    if (mciSendStringA("open \"music/music.mp3\" type mpegvideo alias bgm", NULL, 0, NULL) == 0) {
        mciSendStringA("play bgm repeat", NULL, 0, NULL);
        musicPlaying = true;
    }
}

void stopMusic()
{
    if (!musicPlaying) return;

    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
    musicPlaying = false;
}

void playDoorSound() { playSoundAsync("music/door.mp3"); }
void playLightSound() { playSoundAsync("music/pick.mp3"); }
void playComeInSound() { playSoundAsync("music/ComeIn.mp3"); }
void playGoAwaySound() { playSoundAsync("music/GoAway.mp3"); }
void playScreamerSound() {
    stopMusic();
    playSoundAsync("music/screamer.mp3");
}

void drawQuad(float x1, float y1, float z1, float x2, float y2, float z2,
    float x3, float y3, float z3, float x4, float y4, float z4,
    float nx, float ny, float nz)
{
    glBegin(GL_QUADS);
    glNormal3f(nx, ny, nz);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x1, y1, z1);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x2, y2, z2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x3, y3, z3);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x4, y4, z4);
    glEnd();
}

void drawRoom(float view_matrix[16])
{
    phongTexShader.UseShader();

    float lp[4] = { 0.0f, 3.5f, 0.0f, 1.0f };
    float lp_v[4];
    MatrixMultiply<float, 1, 4, 4, 4>(lp, view_matrix, lp_v);

    int loc = glGetUniformLocationARB(phongTexShader.program, "light_pos_v");
    glUniform3fvARB(loc, 1, lp_v);
    loc = glGetUniformLocationARB(phongTexShader.program, "Ia");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Id");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.7f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Is");
    glUniform3fARB(loc, 0.5f, 0.5f, 0.5f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ma");
    glUniform3fARB(loc, 0.2f, 0.2f, 0.2f);
    loc = glGetUniformLocationARB(phongTexShader.program, "md");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.8f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ms");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "sh");
    glUniform1fARB(loc, 32.0f);
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    texWall.Bind();
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(-2.0f, 0.0f, -2.0f, -2.0f, 0.0f, 2.0f, -2.0f, 4.0f, 2.0f, -2.0f, 4.0f, -2.0f, 1.0f, 0.0f, 0.0f);

    glActiveTexture(GL_TEXTURE0);
    texWall.Bind();
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(2.0f, 0.0f, 2.0f, 2.0f, 0.0f, -2.0f, 2.0f, 4.0f, -2.0f, 2.0f, 4.0f, 2.0f, -1.0f, 0.0f, 0.0f);

    glActiveTexture(GL_TEXTURE0);
    texFloor.Bind();
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(-2.0f, 0.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 0.0f, -2.0f, -2.0f, 0.0f, -2.0f, 0.0f, 1.0f, 0.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    drawQuad(-2.0f, 4.0f, -2.0f, 2.0f, 4.0f, -2.0f, 2.0f, 4.0f, 2.0f, -2.0f, 4.0f, 2.0f, 0.0f, -1.0f, 0.0f);

    glActiveTexture(GL_TEXTURE0);
    texBackWall.Bind();
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(-2.0f, 0.0f, 2.0f, 2.0f, 0.0f, 2.0f, 2.0f, 4.0f, 2.0f, -2.0f, 4.0f, 2.0f, 0.0f, 0.0f, -1.0f);

    glActiveTexture(GL_TEXTURE0);
    texWall.Bind();
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(2.0f, 0.0f, -2.0f, -2.0f, 0.0f, -2.0f, -2.0f, 4.0f, -2.0f, 2.0f, 4.0f, -2.0f, 0.0f, 0.0f, 1.0f);

    Shader::DontUseShaders();
}

void drawDoors(float view_matrix[16])
{
    phongTexShader.UseShader();

    float lp[4] = { 0.0f, 3.5f, 0.0f, 1.0f };
    float lp_v[4];
    MatrixMultiply<float, 1, 4, 4, 4>(lp, view_matrix, lp_v);

    int loc = glGetUniformLocationARB(phongTexShader.program, "light_pos_v");
    glUniform3fvARB(loc, 1, lp_v);
    loc = glGetUniformLocationARB(phongTexShader.program, "Ia");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Id");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.7f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Is");
    glUniform3fARB(loc, 0.5f, 0.5f, 0.5f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ma");
    glUniform3fARB(loc, 0.2f, 0.2f, 0.2f);
    loc = glGetUniformLocationARB(phongTexShader.program, "md");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.8f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ms");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "sh");
    glUniform1fARB(loc, 32.0f);
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    if (!rightDoorOpen)
    {
        texDoorClosed.Bind();
    }
    else
    {
        bool light = rightLightOn;
        if (flashTimer > 0.0f && flashSide == 1)
            light = ((int)(flashTimer * 20.0f) % 2) == 1;
        if (light)
        {
            if (animState == STATE_CORRIDOR && animatronicSide == 1)
                texDoorLitAnimatronic.Bind();
            else
                texDoorLitEmpty.Bind();
        }
        else
        {
            if (animState == STATE_CORRIDOR && animatronicSide == 1)
                texAnimatronicHint.Bind();
            else
                texDoorOpen.Bind();
        }
    }
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(1.98f, 0.0f, -0.75f, 1.98f, 0.0f, 0.75f, 1.98f, 3.0f, 0.75f, 1.98f, 3.0f, -0.75f, -1.0f, 0.0f, 0.0f);

    glActiveTexture(GL_TEXTURE0);
    if (!leftDoorOpen)
    {
        texDoorClosed.Bind();
    }
    else
    {
        bool light = leftLightOn;
        if (flashTimer > 0.0f && flashSide == -1)
            light = ((int)(flashTimer * 20.0f) % 2) == 1;
        if (light)
        {
            if (animState == STATE_CORRIDOR && animatronicSide == -1)
                texDoorLitAnimatronic.Bind();
            else
                texDoorLitEmpty.Bind();
        }
        else
        {
            if (animState == STATE_CORRIDOR && animatronicSide == -1)
                texAnimatronicHint.Bind();
            else
                texDoorOpen.Bind();
        }
    }
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);
    drawQuad(-1.98f, 0.0f, 0.75f, -1.98f, 0.0f, -0.75f, -1.98f, 3.0f, -0.75f, -1.98f, 3.0f, 0.75f, 1.0f, 0.0f, 0.0f);

    Shader::DontUseShaders();
}

void drawTable(float view_matrix[16])
{
    phongTexShader.UseShader();

    float lp[4] = { 0.0f, 3.5f, 0.0f, 1.0f };
    float lp_v[4];
    MatrixMultiply<float, 1, 4, 4, 4>(lp, view_matrix, lp_v);

    int loc = glGetUniformLocationARB(phongTexShader.program, "light_pos_v");
    glUniform3fvARB(loc, 1, lp_v);
    loc = glGetUniformLocationARB(phongTexShader.program, "Ia");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Id");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.7f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Is");
    glUniform3fARB(loc, 0.5f, 0.5f, 0.5f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ma");
    glUniform3fARB(loc, 0.05f, 0.05f, 0.05f);
    loc = glGetUniformLocationARB(phongTexShader.program, "md");
    glUniform3fARB(loc, 0.1f, 0.1f, 0.1f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ms");
    glUniform3fARB(loc, 0.2f, 0.2f, 0.2f);
    loc = glGetUniformLocationARB(phongTexShader.program, "sh");
    glUniform1fARB(loc, 64.0f);
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    float w = 1.2f, h = 0.05f, d = 0.6f;
    float x = 0.0f, y = 0.8f, z = 1.2f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - w, y + h, z - d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + w, y + h, z - d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + w, y + h, z + d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - w, y + h, z + d);

    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - w, y - h, z + d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + w, y - h, z + d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + w, y - h, z - d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - w, y - h, z - d);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - w, y + h, z + d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + w, y + h, z + d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + w, y - h, z + d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - w, y - h, z + d);

    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - w, y - h, z - d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + w, y - h, z - d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + w, y + h, z - d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - w, y + h, z - d);

    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x + w, y + h, z + d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + w, y + h, z - d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + w, y - h, z - d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x + w, y - h, z + d);

    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - w, y + h, z - d);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x - w, y + h, z + d);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x - w, y - h, z + d);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - w, y - h, z - d);
    glEnd();

    Shader::DontUseShaders();
}

ObjModel fanBase;
ObjModel fanBlade;

void drawFan(float view_matrix[16])
{
    phongTexShader.UseShader();

    float lp[4] = { 0.0f, 3.5f, 0.0f, 1.0f };
    float lp_v[4];
    MatrixMultiply<float, 1, 4, 4, 4>(lp, view_matrix, lp_v);

    int loc = glGetUniformLocationARB(phongTexShader.program, "light_pos_v");
    glUniform3fvARB(loc, 1, lp_v);
    loc = glGetUniformLocationARB(phongTexShader.program, "Ia");
    glUniform3fARB(loc, 0.3f, 0.3f, 0.3f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Id");
    glUniform3fARB(loc, 0.8f, 0.8f, 0.7f);
    loc = glGetUniformLocationARB(phongTexShader.program, "Is");
    glUniform3fARB(loc, 0.5f, 0.5f, 0.5f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ma");
    glUniform3fARB(loc, 0.15f, 0.15f, 0.15f);
    loc = glGetUniformLocationARB(phongTexShader.program, "md");
    glUniform3fARB(loc, 0.6f, 0.6f, 0.6f);
    loc = glGetUniformLocationARB(phongTexShader.program, "ms");
    glUniform3fARB(loc, 0.4f, 0.4f, 0.4f);
    loc = glGetUniformLocationARB(phongTexShader.program, "sh");
    glUniform1fARB(loc, 48.0f);
    loc = glGetUniformLocationARB(phongTexShader.program, "tex");
    glUniform1iARB(loc, 0);

    glActiveTexture(GL_TEXTURE0);
    texFan.Bind();

    glPushMatrix();
    glTranslated(0.0f, 0.85f, 1.0f);
    glScaled(0.5f, 0.5f, 0.5f);
    glPushMatrix();
    glScaled(0.4f, 1.1f, 0.8f);
    fanBase.Draw();
    glPopMatrix();

    glPushMatrix();
    glTranslated(0.0f, 0.6f, 0.0f);
    glRotated(-90.0, 1.0, 0.0, 0.0);
    glRotated(full_time * 180.0, 0.0, 1.0, 0.0);
    fanBlade.Draw();
    glPopMatrix();

    glPopMatrix();

    Shader::DontUseShaders();
}

void drawMonitor()
{
    if (!monitorOpen) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    cameraNoiseShader.UseShader();

    glActiveTexture(GL_TEXTURE0);
    if (activeCam == 1) texCam1.Bind();
    else if (activeCam == 2)
    {
        if (animState == STATE_CORRIDOR) texAnimatronicCorridor.Bind();
        else texCam2.Bind();
    }
    else texCam3.Bind();

    glActiveTexture(GL_TEXTURE1);
    texStaticNoise.Bind();

    int loc = glGetUniformLocationARB(cameraNoiseShader.program, "texStatic");
    glUniform1iARB(loc, 0);
    loc = glGetUniformLocationARB(cameraNoiseShader.program, "texCamera");
    glUniform1iARB(loc, 1);
    loc = glGetUniformLocationARB(cameraNoiseShader.program, "u_time");
    glUniform1fARB(loc, full_time);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.1f, 0.1f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(0.9f, 0.1f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(0.9f, 0.9f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.1f, 0.9f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    Shader::DontUseShaders();

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawScreamer()
{
    if (animState != STATE_ATTACK || stateTimer >= 3.0f) return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    texScreamer.Bind();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void resetGame()
{
    animState = STATE_STAGE;
    stateTimer = 0.0f;
    rightDoorOpen = true;
    leftDoorOpen = true;
    gameOver = false;
    rightLightOn = false;
    leftLightOn = false;
    monitorOpen = false;
    activeCam = 1;
    camBlendFactor = 0.0f;
    prevCam = 1;
    camSwitching = false;
    camSwitchTimer = 0.0f;
    cameraYaw = 0.0f;
    cameraPitch = 0.0f;
    animatronicSide = 0;
    corridorTimer = 0.0f;
    doorCloseTimer = 0.0f;
    flashTimer = 0.0f;
    flashSide = 0;
    gameOverTimer = 0.0f;
    rightDoorClosedTimer = 0.0f;
    leftDoorClosedTimer = 0.0f;
    rightDoorOpenCount = 0;
    leftDoorOpenCount = 0;
}

void updateAnimatronic(float dt)
{
    if (gameOver)
    {
        gameOverTimer += dt;
        if (gameOverTimer >= 3.0f)
        {
            PostMessage(gl.getHwnd(), WM_CLOSE, 0, 0);
        }
        if (animState == STATE_ATTACK)
            stateTimer += dt;
        return;
    }

    if (!rightDoorOpen)
    {
        rightDoorClosedTimer += dt;
        if (rightDoorClosedTimer >= 8.0f)
        {
            rightDoorOpen = true;
            rightLightOn = false;
            rightDoorClosedTimer = 0.0f;
            playDoorSound();
            if (animState == STATE_CORRIDOR && animatronicSide == 1)
                corridorTimer = 0.0f;
        }
    }
    else
    {
        rightDoorClosedTimer = 0.0f;
    }

    if (!leftDoorOpen)
    {
        leftDoorClosedTimer += dt;
        if (leftDoorClosedTimer >= 8.0f)
        {
            leftDoorOpen = true;
            leftLightOn = false;
            leftDoorClosedTimer = 0.0f;
            playDoorSound();
            if (animState == STATE_CORRIDOR && animatronicSide == -1)
                corridorTimer = 0.0f;
        }
    }
    else
    {
        leftDoorClosedTimer = 0.0f;
    }

    stateTimer += dt;

    switch (animState)
    {
    case STATE_STAGE:
        if (stateTimer >= 4.5f)
        {
            animState = STATE_CORRIDOR;
            stateTimer = 0.0f;
            corridorTimer = 0.0f;
            doorCloseTimer = 0.0f;
            if (rightDoorOpen && leftDoorOpen)
                animatronicSide = (rand() % 2 == 0) ? 1 : -1;
            else if (rightDoorOpen)
                animatronicSide = 1;
            else if (leftDoorOpen)
                animatronicSide = -1;
            else
                animatronicSide = (rand() % 2 == 0) ? 1 : -1;
            flashTimer = 0.6f;
            flashSide = animatronicSide;
            playComeInSound();
        }
        break;

    case STATE_CORRIDOR:
    {
        bool doorOpen = (animatronicSide == 1) ? rightDoorOpen : leftDoorOpen;
        int openCount = (animatronicSide == 1) ? rightDoorOpenCount : leftDoorOpenCount;
        if (openCount >= 6)
        {
            animState = STATE_ATTACK;
            stateTimer = 0.0f;
            corridorTimer = 0.0f;
            doorCloseTimer = 0.0f;
            gameOver = true;
            gameOverTimer = 0.0f;
            playScreamerSound();
        }
        else if (doorOpen)
        {
            doorCloseTimer = 0.0f;
            corridorTimer += dt;
            if (corridorTimer >= 6.0f)
            {
                animState = STATE_ATTACK;
                stateTimer = 0.0f;
                corridorTimer = 0.0f;
                doorCloseTimer = 0.0f;
                gameOver = true;
                gameOverTimer = 0.0f;
                playScreamerSound();
            }
        }
        else
        {
            corridorTimer = 0.0f;
            doorCloseTimer += dt;
            if (doorCloseTimer >= 1.5f)
            {
                animState = STATE_STAGE;
                stateTimer = 0.0f;
                corridorTimer = 0.0f;
                doorCloseTimer = 0.0f;
                rightDoorOpenCount = 0;
                leftDoorOpenCount = 0;
                flashTimer = 0.6f;
                flashSide = animatronicSide;
                int oldSide = animatronicSide;
                animatronicSide = 0;
                playGoAwaySound();
            }
        }
    }
    break;

    case STATE_ATTACK:
        if (stateTimer >= 2.0f)
        {
            resetGame();
        }
        break;
    }
}

void onKeyPress(OpenGL* sender, KeyEventArg arg)
{
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
    switch (key)
    {
    case 'T':
    case 't':
        testMode = !testMode;
        if (testMode)
        {
            testCamX = 0.0f;
            testCamY = 1.5f;
            testCamZ = -0.5f;
            cameraPitch = 0.0f;
            cameraYaw = 0.0f;
            prevMouseX = -1;
            prevMouseY = -1;
            skipNextWarp = false;
            monitorOpen = false;
            ShowCursor(FALSE);
        }
        else
        {
            cameraYaw = 0.0f;
            cameraPitch = 0.0f;
            ShowCursor(TRUE);
        }
        break;
    case 'E':
    case 'e':
        if (!testMode && !gameOver && !monitorOpen)
        {
            if (cameraYaw > 30.0f)
            {
                if (!rightDoorOpen)
                {
                    rightDoorOpen = true;
                    rightLightOn = false;
                    rightDoorClosedTimer = 0.0f;
                    playDoorSound();
                    if (animState == STATE_CORRIDOR && animatronicSide == 1)
                    {
                        corridorTimer = 0.0f;
                        rightDoorOpenCount++;
                    }
                }
                else
                {
                    rightDoorOpen = false;
                    rightLightOn = false;
                    rightDoorClosedTimer = 0.0f;
                    playDoorSound();
                }
            }
            else if (cameraYaw < -30.0f)
            {
                if (!leftDoorOpen)
                {
                    leftDoorOpen = true;
                    leftLightOn = false;
                    leftDoorClosedTimer = 0.0f;
                    playDoorSound();
                    if (animState == STATE_CORRIDOR && animatronicSide == -1)
                    {
                        corridorTimer = 0.0f;
                        leftDoorOpenCount++;
                    }
                }
                else
                {
                    leftDoorOpen = false;
                    leftLightOn = false;
                    leftDoorClosedTimer = 0.0f;
                    playDoorSound();
                }
            }
        }
        break;
    case '1':
        if (!testMode && monitorOpen) { prevCam = activeCam; activeCam = 1; camSwitching = true; camSwitchTimer = 0.0f; }
        break;
    case '2':
        if (!testMode && monitorOpen) { prevCam = activeCam; activeCam = 2; camSwitching = true; camSwitchTimer = 0.0f; }
        break;
    case '3':
        if (!testMode && monitorOpen) { prevCam = activeCam; activeCam = 3; camSwitching = true; camSwitchTimer = 0.0f; }
        break;
    }

    if (!testMode && (arg.key == VK_SPACE || arg.key == VK_TAB))
    {
        if (fabs(cameraYaw) < 15.0f && !gameOver)
        {
            monitorOpen = !monitorOpen;
        }
    }
}

void onMouseMove(OpenGL* sender, MouseEventArg arg)
{
    if (testMode)
    {
        if (skipNextWarp)
        {
            skipNextWarp = false;
            prevMouseX = arg.x;
            prevMouseY = arg.y;
            return;
        }

        if (prevMouseX >= 0 && prevMouseY >= 0)
        {
            float dx = static_cast<float>(arg.x - prevMouseX);
            float dy = static_cast<float>(arg.y - prevMouseY);
            cameraYaw -= dx * 0.15f;
            cameraPitch -= dy * 0.15f;
            if (cameraPitch > 89.0f) cameraPitch = 89.0f;
            if (cameraPitch < -89.0f) cameraPitch = -89.0f;
        }

        prevMouseX = arg.x;
        prevMouseY = arg.y;

        HWND hwnd = gl.getHwnd();
        if (hwnd)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            if (rc.right > 0 && rc.bottom > 0)
            {
                POINT pt = { rc.right / 2, rc.bottom / 2 };
                ClientToScreen(hwnd, &pt);
                SetCursorPos(pt.x, pt.y);
                skipNextWarp = true;
            }
        }
        return;
    }

    hoverMouseX = static_cast<float>(arg.x);
}

void initRender()
{
    texWall.LoadTexture("textures/wall.png");
    texFloor.LoadTexture("textures/floor.png");
    texBackWall.LoadTexture("textures/backwall.png");
    texDoorClosed.LoadTexture("textures/door_closed.png");
    texDoorOpen.LoadTexture("textures/door_open.png");
    texFan.LoadTexture("textures/fan.png");
    texAnimatronicHint.LoadTexture("textures/animatronic_hint.png");
    texStaticNoise.LoadTexture("textures/static_noise.png");
    texCam1.LoadTexture("textures/cam1.png");
    texCam2.LoadTexture("textures/cam2.png");
    texCam3.LoadTexture("textures/cam3.png");
    texScreamer.LoadTexture("textures/screamer.png");
    texAnimatronicCorridor.LoadTexture("textures/animatronic_corridor.png");
    texDoorLitEmpty.LoadTexture("textures/door_lit_empty.png");
    texDoorLitAnimatronic.LoadTexture("textures/door_lit_animatronic.png");

    fanBase.LoadModel("models/fan_base.obj_m");
    fanBlade.LoadModel("models/fan_blade.obj_m");

    phongTexShader.VshaderFileName = "shaders/v.vert";
    phongTexShader.FshaderFileName = "shaders/phong_tex.frag";
    phongTexShader.LoadShaderFromFile();
    phongTexShader.Compile();

    cameraNoiseShader.VshaderFileName = "shaders/v.vert";
    cameraNoiseShader.FshaderFileName = "shaders/camera_noise.frag";
    cameraNoiseShader.LoadShaderFromFile();
    cameraNoiseShader.Compile();

    gl.KeyDownEvent.reaction(onKeyPress);
    gl.MouseMovieEvent.reaction(onMouseMove);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    float light_pos[] = { 0.0f, 3.5f, 0.0f, 1.0f };
    float light_amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    float light_dif[] = { 0.8f, 0.8f, 0.7f, 1.0f };
    float light_spec[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_spec);

    glEnable(GL_TEXTURE_2D);

    startMusic();
}

void Render(double delta_time)
{
    delta_global = delta_time;
    full_time += static_cast<float>(delta_time);

    float dt = static_cast<float>(delta_time);
    updateAnimatronic(dt);

    if (camSwitching)
    {
        camSwitchTimer += dt;
        camBlendFactor = camSwitchTimer / 0.4f;
        if (camBlendFactor >= 1.0f)
        {
            camBlendFactor = 1.0f;
            camSwitching = false;
        }
    }

    if (flashTimer > 0.0f)
    {
        flashTimer -= dt;
        if (flashTimer <= 0.0f)
        {
            flashTimer = 0.0f;
            flashSide = 0;
        }
    }

    if (!testMode && !gameOver && !monitorOpen)
    {
        bool fPressed = OpenGL::isKeyPressed('F');
        bool oldRightLight = rightLightOn;
        bool oldLeftLight = leftLightOn;
        if (cameraYaw < -30.0f)
            leftLightOn = (leftDoorOpen && fPressed);
        else if (cameraYaw > 30.0f)
            rightLightOn = (rightDoorOpen && fPressed);
        else
        {
            rightLightOn = false;
            leftLightOn = false;
        }
        if (rightLightOn != oldRightLight || leftLightOn != oldLeftLight)
            playLightSound();
        int currentWidth = gl.getWidth();
        if (currentWidth <= 0) currentWidth = 800;
        float leftZone = currentWidth * 0.20f;
        float rightZone = currentWidth * 0.80f;

        if (hoverMouseX >= 0.0f)
        {
            if (hoverMouseX < leftZone)
            {
                cameraYaw += 1.0f;
                if (cameraYaw > 65.0f) cameraYaw = 65.0f;
            }
            else if (hoverMouseX > rightZone)
            {
                cameraYaw -= 1.0f;
                if (cameraYaw < -65.0f) cameraYaw = -65.0f;
            }
        }
    }

    glMatrixMode(GL_MODELVIEW);

    if (testMode)
    {
        float moveSpeed = 3.0f * dt;
        float yawRad = cameraYaw * 3.14159265f / 180.0f;
        float pitchRad = cameraPitch * 3.14159265f / 180.0f;

        float fwdX = sinf(yawRad);
        float fwdZ = cosf(yawRad);
        float rightX = cosf(yawRad);
        float rightZ = -sinf(yawRad);

        if (OpenGL::isKeyPressed('W')) { testCamX += fwdX * moveSpeed; testCamZ += fwdZ * moveSpeed; }
        if (OpenGL::isKeyPressed('S')) { testCamX -= fwdX * moveSpeed; testCamZ -= fwdZ * moveSpeed; }
        if (OpenGL::isKeyPressed('A')) { testCamX -= rightX * moveSpeed; testCamZ -= rightZ * moveSpeed; }
        if (OpenGL::isKeyPressed('D')) { testCamX += rightX * moveSpeed; testCamZ += rightZ * moveSpeed; }
        if (OpenGL::isKeyPressed(VK_SPACE)) { testCamY += moveSpeed; }
        if (OpenGL::isKeyPressed(VK_SHIFT)) { testCamY -= moveSpeed; }

        float lookX = testCamX + sinf(yawRad) * cosf(pitchRad);
        float lookY = testCamY + sinf(pitchRad);
        float lookZ = testCamZ + cosf(yawRad) * cosf(pitchRad);

        glLoadIdentity();
        gluLookAt(testCamX, testCamY, testCamZ,
            lookX, lookY, lookZ,
            0.0, 1.0, 0.0);
    }
    else
    {
        glLoadIdentity();
        gluLookAt(0.0, 1.5, -0.5,
            0.0, 1.5, 1.0,
            0.0, 1.0, 0.0);

        glRotated(-cameraYaw, 0.0, 1.0, 0.0);
    }

    float view_matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

    drawRoom(view_matrix);
    drawDoors(view_matrix);
    drawTable(view_matrix);
    drawFan(view_matrix);

    drawMonitor();
    drawScreamer();
}