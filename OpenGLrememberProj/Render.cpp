#include "Render.h"

#include <windows.h>
#include <sstream>
#include <random>
#include <iomanip>

#define _USE_MATH_DEFINES
#include <cmath>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

#define TEXTURE_ON

GuiTextRectangle rec;
Shader s[10];  //массивчик для десяти шейдеров
std::mt19937 rng;

int tick_o = 0, tick_n = 0;
int enableTextureL, enableNormalMapL, enableLightL,  enableFogL;
int ma, md, ms;

bool enableLight, enableFog, debugInfo, pause;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
    //дистанция камеры
    double camDist;
    //углы поворота камеры
    angle eta, fi;

    CustomCamera()
    {
        camDist = 1;
        eta = 1;
        fi = 1;
    }

    //считает позицию камеры, исходя из углов поворота, вызывается движком
    virtual void SetUpCamera()
    {
        pos.setCoords(camDist * sin(eta) * cos(fi) + lookPoint.X(),
            camDist * sin(eta) * sin(fi) + lookPoint.Y(),
            camDist * cos(eta) + lookPoint.Z());

        if (sin(eta) <= 0)
            normal.setCoords(0, 0, -1);
        else
            normal.setCoords(0, 0, 1);

        LookAt();
    }

    void CustomCamera::LookAt()
    {
        gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
    }
};

struct Player : Object
{
    ObjFile model[13];
    Texture texture;
    CustomCamera cam;
    angle eta, fi;

    const float mvSpeed = 10;
    const float rtSensitivity = 0.01;

    float animTime;
    int animStart = 3;
    int animFrame;
    int movement[2];

    Player()
    {
        startPos();
    }

    void load() {
        texture.loadTextureFromFile("textures\\spiderTex.bmp");

        std::ostringstream ss;
        for (int i = 0; i < 13; ++i)
        {
            ss << "models\\spider_" << std::setfill('0') << std::setw(6) << i + 1 << ".obj";
            loadModel(ss.str().c_str(), &model[i]);
            ss.clear();
            ss.seekp(0);
        }
    }

    void startPos() {
        pos.setCoords(0, 0, 0);
        cam.lookPoint = pos;

        animFrame = animStart;
        eta = M_PI / 2;
        fi = M_PI / 2;

        cam.camDist = 6;
        cam.eta = M_PI * 1/4;
        cam.fi = fi;
    }

    void rotate(int dx, int dy)
    {
        fi = fi + (double) rtSensitivity * dx;
        cam.eta = cam.eta + (double) rtSensitivity * dy;

        if (cam.eta > 2*M_PI / 5)
            cam.eta = 2*M_PI / 5;
        else if (cam.eta < 0.001)
            cam.eta = 0.001;
    }

    void update(float deltaTime)
    {
        Vector3 mvDirection = Vector3(eta, fi, 1) * movement[0] +
                              Vector3(eta, fi + (double)M_PI/2, 1) * movement[1];
        mvDirection.normalize();

        pos = pos + mvDirection * mvSpeed * deltaTime;
        cam.lookPoint = pos;
        cam.fi = fi-M_PI;

        if (mvDirection.length() != 0 || (animFrame != 3 && animFrame != 9)) {
            animFrame = (animStart +(int)(animTime * 13.0 / 1.0))%13;
            animTime = fmod(animTime + deltaTime, 1.0);
        }
        else
        {
            animStart = animFrame;
            animTime = 0;
        }
    }

    inline void render()
    {
#ifdef TEXTURE_ON
        glUniform1iARB(enableTextureL, 1);
        glActiveTexture(GL_TEXTURE0);
        texture.bindTexture();
#endif

        glUniform1iARB(enableNormalMapL, 0);

        glPushMatrix();
        glTranslated(pos.X(), pos.Y(), pos.Z());
        glRotated(fi * 180 / M_PI, 0, 0, 1);
        model[animFrame].RenderModel(GL_TRIANGLES);
        glPopMatrix();
    }
} player;

struct Goal : Object
{
    angle fi;

    const int N = 10;
    const float H = 4, R = 3;
    const float rotateSpeed = 0.5, moveSpeed = 1.0;
    float **points, **pointsH;
    bool moveState;

    Goal()
    {
        points = new float* [N];
        for (int i = 0; i < N; ++i)
            points[i] = new float[3];

        pointsH = new float* [N];
        for (int i = 0; i < N; ++i)
            pointsH[i] = new float[3];

        for (int i = 0; i < N; ++i) {
            float a = i * 2 * M_PI / N;
            points[i][0] = R * cos(a);
            points[i][1] = R * sin(a);
            points[i][2] = 0;

            pointsH[i][0] = points[i][0];
            pointsH[i][1] = points[i][1];
            pointsH[i][2] = H;
        }

        fi = 0;
        moveState = 0;
    }

    void update(float deltaTime)
    {
        fi =  deltaTime * rotateSpeed + fi;
        if (fi > 2 * M_PI)
            fi = 0;

        float newZ = pos.Z();
        if (moveState)
        {
            newZ += deltaTime * moveSpeed;
            if (newZ > 0.0)
            {
                newZ = pos.Z();
                moveState = 0;
            }
        }
        else
        {
            newZ -= deltaTime * moveSpeed;
            if (newZ < -2.0)
            {
                newZ = pos.Z();
                moveState = 1;
            }
        }
        pos.setCoords(pos.X(), pos.Y(), newZ);
    }

    void drawSide(int i, float texC)
    {
        int nxt = (i + 1) % N;

        glTexCoord2f(texC, 1);
        glVertex3fv(pointsH[i]);
        glTexCoord2f(texC + 0.5, 0);
        glVertex3fv(points[nxt]);
        glTexCoord2f(texC, 0);
        glVertex3fv(points[i]);

        glTexCoord2f(texC, 1);
        glVertex3fv(pointsH[i]);
        glTexCoord2f(texC + 0.5, 1);
        glVertex3fv(pointsH[nxt]);
        glTexCoord2f(texC + 0.5, 0);
        glVertex3fv(points[nxt]);
    }

    void render()
    {
        s[1].UseShader();
        int location = glGetUniformLocationARB(s[1].program, "gradColor");
        glUniform3fARB(location, 0.75, 0.75, 0.0);
        location = glGetUniformLocationARB(s[1].program, "sinGrad");
        glUniform1iARB(location, 1);

        glPushMatrix();
        glTranslated(pos.X(), pos.Y(), pos.Z());
        glRotated(fi * 180 / M_PI, 0, 0, 1);
        glBegin(GL_TRIANGLES);
        
        for (int i = 0; i < N; i += 2)
        {
            drawSide(i, 0);
            drawSide(i+1, 0.5);
        }

        glEnd();
        glPopMatrix();
    }

    bool doesContain(Vector3 coords, float size)
    {
        return (coords - Vector3(pos.X(), pos.Y(), 0)).length() <= R - size;
    }
};

struct Threat : Object
{
    angle fi;

    const int N = 80;
    const float H = 1, R = 4, A = 20, B = 10;
    float **triggerP, **triggerPH;

    Threat()
    {
        triggerP = new float* [N];
        for (int i = 0; i < N; ++i)
            triggerP[i] = new float[3];

        triggerPH = new float* [N];
        for (int i = 0; i < N; ++i)
            triggerPH[i] = new float[3];

        for (int i = 0; i < N/4; ++i)
        {
            angle etha = i * 2 * M_PI / (N/4);
            triggerP[i][0] = pow(A * cos(etha), 2 / R);
            triggerP[i][1] = pow(B * sin(etha), 2 / R);
            triggerP[i][2] = 0;

            triggerP[N/4 + i][0] = -triggerP[i][0];
            triggerP[N/4 + i][1] = triggerP[i][1];
            triggerP[N/4 + i][2] = 0;

            triggerP[N/2 + i][0] = -triggerP[i][0];
            triggerP[N/2 + i][1] = -triggerP[i][1];
            triggerP[N/2 + i][2] = 0;

            triggerP[3*N/4 + i][0] = triggerP[i][0];
            triggerP[3 * N / 4 + i][1] = -triggerP[i][1];
            triggerP[3 * N / 4 + i][2] = 0;
        }

        for (int i = 0; i < N; ++i)
        {
            triggerPH[i][0] = triggerP[i][0];
            triggerPH[i][1] = triggerP[i][1];
            triggerPH[i][2] = H;
        }
    }

    void render()
    {
        s[1].UseShader();
        int location = glGetUniformLocationARB(s[1].program, "gradColor");
        glUniform3fARB(location, 0.75, 0.0, 0.0);
        location = glGetUniformLocationARB(s[1].program, "sinGrad");
        glUniform1iARB(location, 0);

        glPushMatrix();
        glTranslated(pos.X(), pos.Y(), pos.Z());
        glRotated(fi * 180 / M_PI, 0, 0, 1);
        glBegin(GL_TRIANGLES);

        const float step = 0.5;
        for (int i = 0; i < N; ++i)
        {
            int nxt = (i + 1) % N;
            glTexCoord2f(0, 1);
            glVertex3fv(triggerPH[i]);
            glTexCoord2f(0, 0);
            glVertex3fv(triggerP[nxt]);
            glTexCoord2f(0, 0);
            glVertex3fv(triggerP[i]);

            glTexCoord2f(0, 1);
            glVertex3fv(triggerPH[i]);
            glTexCoord2f(0, 1);
            glVertex3fv(triggerPH[nxt]);
            glTexCoord2f(0, 0);
            glVertex3fv(triggerP[nxt]);
        }

        glEnd();
        glPopMatrix();
    }

    bool doesContain(Vector3 coords, float size)
    {
        return (coords.X() < pos.X() - size + A / 2 && coords.X() > pos.X() + size - A / 2 &&
            coords.Y() < pos.Y() - size + B / 2 && coords.Y() > pos.Y() + size - B / 2);
            
        return pow(abs(pos.X() - coords.X())/A, R) + pow(abs(pos.Y() - coords.Y()) / B, R) <= 1;
    }
};

struct Room : Object
{
    ObjFile floor, wall, baseBoards;
    Texture fTexture, fNormal;

    void load() {
#ifdef TEXTURE_ON
        fTexture.loadTextureFromFile("textures\\floor_baseColor.bmp");
        fNormal.loadTextureFromFile("textures\\floor_normal.bmp");
#endif

        loadModel("models\\floor.obj", &floor);
        loadModel("models\\wall.obj", &wall);
        loadModel("models\\baseboards.obj", &baseBoards);
    }

    void render()
    {
        glPushMatrix();
        glTranslated(pos.X(), pos.Y(), pos.Z());
        glScalef(1.0, 1.0, 1.0);

        ma = glGetUniformLocationARB(s[0].program, "ma");
        glUniform3fARB(ma, 0.5, 1.0, 0.5);
        md = glGetUniformLocationARB(s[0].program, "md");
        glUniform3fARB(md, 0.5, 1.0, 0.5);
        ms = glGetUniformLocationARB(s[0].program, "ms");
        glUniform4fARB(ms, 0.5, 1.0, 0.5, 16);
        wall.RenderModel(GL_TRIANGLES);

        ma = glGetUniformLocationARB(s[0].program, "ma");
        glUniform3fARB(ma, 1.0, 1.0, 1.0);
        md = glGetUniformLocationARB(s[0].program, "md");
        glUniform3fARB(md, 1.0, 1.0, 1.0);
        ms = glGetUniformLocationARB(s[0].program, "ms");
        glUniform4fARB(ms, 1.0, 1.0, 1.0, 25.6);
        baseBoards.RenderModel(GL_TRIANGLES);

#ifdef TEXTURE_ON
        glUniform1iARB(enableTextureL, 1);
        glActiveTexture(GL_TEXTURE0);
        fTexture.bindTexture();

        glUniform1iARB(enableNormalMapL, 1);
        glActiveTexture(GL_TEXTURE1);
        fNormal.bindTexture();
#endif

        floor.RenderModel(GL_TRIANGLES);
        glPopMatrix();
    }
} room;

struct GameManager : Object
{
    const int numberOfGoals = 4;
    const float xMin = -40, xMax = 40, yVariation = 2;
    const float xFinal = -43.8, yFinal = 175, yStart = 25;

    const float curveVariation = 10, curveSpeed = 0.3;

    Goal goal;
    Threat threat;
    int clearedGoals;

    int direction = 1;
    float curveP1[2], curveP2[2], curveP3[2], curveP4[3], curveT;

    GameManager()
    {
        init();
    }

    void init()
    {
        clearedGoals = 0;
        generateGoal();
        player.startPos();
    }

    inline double beziers(double p0, double p1, double p2, double p3, double t) {
        return p0 * pow(1 - t, 3) + 3 * t * p1 * pow(1 - t, 2) + 3 * t * t * p2 * (1 - t) + t * t * t * p3;
    }

    void generateGoal()
    {
        if (clearedGoals + 1 != numberOfGoals)
        {
            std::uniform_real_distribution<double> xDist(xMin, xMax);
            std::uniform_real_distribution<double> yDist(-yVariation, yVariation);
            float x = xDist(rng);
            float y = yStart + clearedGoals * (yFinal - yStart) / (numberOfGoals - 1) + yDist(rng);
            goal.pos.setCoords(x, y, goal.pos.Z());
        }
        else
        {
            goal.pos.setCoords(xFinal, yFinal, goal.pos.Z());
        }

        generateCurve();
    }

    void generateCurve()
    {
        std::uniform_real_distribution<double> curveDist(-curveVariation, curveVariation);

        curveP1[0] = goal.pos.X() + curveDist(rng);
        curveP1[1] = goal.pos.Y() + curveDist(rng);

        curveP2[0] = goal.pos.X() + curveDist(rng);
        curveP2[1] = goal.pos.Y() + curveDist(rng);

        curveP3[0] = player.pos.X() + curveDist(rng);
        curveP3[1] = player.pos.Y() + curveDist(rng);

        curveP4[0] = player.pos.X() + curveDist(rng);
        curveP4[1] = player.pos.Y() + curveDist(rng);

        curveT = 0;
        direction = 1;
    }

    void update(float deltaTime)
    {
        curveT = curveT + direction * deltaTime * curveSpeed;
        if (curveT > 1)
            direction = -1;
        else if (curveT < 0)
            generateCurve();
    
            
        float x = beziers(curveP1[0], curveP2[0], curveP3[0], curveP4[0], curveT);
        float y = beziers(curveP1[1], curveP2[1], curveP3[1], curveP4[1], curveT);
        threat.pos.setCoords(x, y, 0);

        if (goal.doesContain(player.pos, 1))
        {
            ++clearedGoals;
            if (clearedGoals != numberOfGoals)
                generateGoal();
            else
                init();
        }

        if (threat.doesContain(player.pos, 3))
            init();
    }
} manager;

//Класс для настройки света
class CustomLight : public Light
{
public:
    CustomLight()
    {
        //начальная позиция света
        pos = Vector3(0, 50, 50);
    }

    //рисует сферу и линии под источником света, вызывается движком
    void DrawLightGhismo()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        Shader::ToggleShaders();
        bool f1 = glIsEnabled(GL_LIGHTING);
        glDisable(GL_LIGHTING);
        bool f2 = glIsEnabled(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_2D);
        bool f3 = glIsEnabled(GL_DEPTH_TEST);

        glDisable(GL_DEPTH_TEST);
        glColor3d(0.9, 0.8, 0);
        Sphere s;
        s.pos = pos;
        s.scale = s.scale * 0.08;
        s.Show();

        if (OpenGL::isKeyPressed('G'))
        {
            glColor3d(0, 0, 0);
            //линия от источника света до окружности
            glBegin(GL_LINES);
            glVertex3d(pos.X(), pos.Y(), pos.Z());
            glVertex3d(pos.X(), pos.Y(), 0);
            glEnd();

            //рисуем окруность
            Circle c;
            c.pos.setCoords(pos.X(), pos.Y(), 0);
            c.scale = c.scale * 1.5;
            c.Show();
        }
    }

    void SetUpLight()
    {
        GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
        GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
        GLfloat spec[] = { .7, .7, .7, 0 };
        GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

        // параметры источника света
        glLightfv(GL_LIGHT0, GL_POSITION, position);
        // характеристики излучаемого света
        // фоновое освещение (рассеянный свет)
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        // диффузная составляющая света
        glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
        // зеркально отражаемая составляющая света
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

        glEnable(GL_LIGHT0);
    }
} light;  //создаем источник света

//старые координаты мыши
int mouseX = 0, mouseY = 0;
float zoom = 1;

//обработчик движения мыши
void mouseEvent(OpenGL* ogl, int mX, int mY)
{
    int dx = mouseX - mX;
    int dy = mouseY - mY;
    mouseX = mX;
    mouseY = mY;

    if (!OpenGL::isKeyPressed('G'))
    {
        player.rotate(dx, dy);
    }
    else
    {
        if (OpenGL::isKeyPressed(VK_LBUTTON))
        {
            light.pos = light.pos + Vector3(0, 0, 0.02 * dy);
        }
        else  //двигаем свет по плоскости, в точку где мышь
        {
            LPPOINT POINT = new tagPOINT();
            GetCursorPos(POINT);
            ScreenToClient(ogl->getHwnd(), POINT);
            POINT->y = ogl->getHeight() - POINT->y;

            Ray r = player.cam.getLookRay(POINT->x, POINT->y, 60, ogl->aspect);

            double z = light.pos.Z();

            double k = 0, x = 0, y = 0;
            if (r.direction.Z() == 0)
                k = 0;
            else
                k = (z - r.origin.Z()) / r.direction.Z();

            x = k * r.direction.X() + r.origin.X();
            y = k * r.direction.Y() + r.origin.Y();

            light.pos = Vector3(x, y, z);
        }
    }
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL* ogl, int delta)
{
    float _tmpZ = delta * 0.003;
    if (ogl->isKeyPressed('Z'))
        _tmpZ *= 10;
    zoom += 0.2 * zoom * _tmpZ;

    if (delta < 0 && player.cam.camDist <= 3)
        return;
    if (delta > 0 && player.cam.camDist >= 10)
        return;

    player.cam.camDist += 0.01 * delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL* ogl, int key)
{
    switch (key)
    {
    case 'W':    
        player.movement[0] = 1;
        break;
    case 'A':
        player.movement[1] = 1;
        break;
    case 'S':
        player.movement[0] = -1;
        break;
    case 'D':
        player.movement[1] = -1;
        break;
    case 'F':
        enableFog = !enableFog;
        break;
    case 'L':
        enableLight = !enableLight;
        break;
    case 'P':
        pause = !pause;
        break;
    case 'C':
        s[0].LoadShaderFromFile();
        s[0].Compile();
        s[1].LoadShaderFromFile();
        s[1].Compile();
        break;
    case 'I':
        debugInfo = !debugInfo;
        break;
    }
}

void keyUpEvent(OpenGL* ogl, int key)
{
    switch (key)
    {
    case 'W':
        player.movement[0] = 0;
        break;
    case 'A':
        player.movement[1] = 0;
        break;
    case 'S':
        player.movement[0] = 0;
        break;
    case 'D':
        player.movement[1] = 0;
        break;
    }
}

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
    glClearColor(0.05,0.05,0.05,1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //настройка текстур
    //4 байта на хранение пикселя
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    //настройка режима наложения текстур
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    //включаем текстуры
    glEnable(GL_TEXTURE_2D);
    
    room.load();
    player.load();

    //камеру и свет привязываем к "движку"
    ogl->mainCamera = &(player.cam);
    ogl->mainLight = &light;

    // нормализация нормалей : их длины будет равна 1
    glEnable(GL_NORMALIZE);

    // устранение ступенчатости для линий
    glEnable(GL_LINE_SMOOTH);

    s[0].VshaderFileName = "shaders\\light.vert"; //имя файла вершинного шейдер
    s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
    s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
    s[0].Compile(); //компилируем

    s[1].VshaderFileName = "shaders\\grad.vert"; //имя файла вершинного шейдер
    s[1].FshaderFileName = "shaders\\grad.frag"; //имя файла фрагментного шейдера
    s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
    s[1].Compile(); //компилируем

    tick_n = GetTickCount64();
    tick_o = tick_n;

    rec.setSize(300, 100);
    rec.setPosition(10, ogl->getHeight() - 100 - 10);
    rec.setText("G - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали", 0, 0, 0);

    std::random_device rd;
    rng.seed(rd());
    enableLight = true;
    enableFog = true;
    debugInfo = false;
    pause = false;
}

void Render(OpenGL* ogl)
{
    tick_o = tick_n;
    tick_n = GetTickCount();
    float deltaTime = (tick_n - tick_o) / 1000.0;

    glEnable(GL_DEPTH_TEST);

    //альфаналожение
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    s[0].UseShader();

    int location = glGetUniformLocationARB(s[0].program, "texture");
    glUniform1iARB(location, 0);

    location = glGetUniformLocationARB(s[0].program, "normalMap");
    glUniform1iARB(location, 1);

    enableTextureL = glGetUniformLocationARB(s[0].program, "enableTexture");
    glUniform1iARB(enableTextureL, 0);
    enableNormalMapL = glGetUniformLocationARB(s[0].program, "enableNormalMap");
    glUniform1iARB(enableNormalMapL, 0);
    enableLightL = glGetUniformLocationARB(s[0].program, "enableLight");
    glUniform1iARB(enableLightL, enableLight);
    enableFogL = glGetUniformLocationARB(s[0].program, "enableFog");
    glUniform1iARB(enableFogL, enableFog);

    location = glGetUniformLocationARB(s[0].program, "light_pos");
    glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

    location = glGetUniformLocationARB(s[0].program, "camera_pos");
    glUniform3fARB(location, player.cam.pos.X(), player.cam.pos.Y(), player.cam.pos.Z());

    location = glGetUniformLocationARB(s[0].program, "player_pos");
    glUniform3fARB(location, player.pos.X(), player.pos.Y(), player.pos.Z());

    location = glGetUniformLocationARB(s[0].program, "iViewMatrix");
    float iv_matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, iv_matrix);
    glUniformMatrix4fv(location, 1, false, iv_matrix);

    location = glGetUniformLocationARB(s[0].program, "Ia");
    glUniform1fARB(location, 0.2);

    location = glGetUniformLocationARB(s[0].program, "Id");
    glUniform1fARB(location, 0.5);

    location = glGetUniformLocationARB(s[0].program, "Is");
    glUniform1fARB(location, 0.9);

    ma = glGetUniformLocationARB(s[0].program, "ma");
    glUniform3fARB(ma, 1.0, 1.0, 1.0);
    md = glGetUniformLocationARB(s[0].program, "md");
    glUniform3fARB(md, 1.0, 1.0, 1.0);
    ms = glGetUniformLocationARB(s[0].program, "ms");
    glUniform4fARB(ms, 1.0, 1.0, 1.0, 16);

    room.render();
    player.render();
    manager.goal.render();
    manager.threat.render();

    player.update(deltaTime);
    if (!pause)
    {
        manager.goal.update(deltaTime);
        manager.update(deltaTime);
    }
}

bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL* ogl)
{
    Shader::ToggleShaders();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);


    std::ostringstream ss;
    ss << manager.clearedGoals << "/" << manager.numberOfGoals << std::endl;
    if (debugInfo)
    {
        ss << "Player: pos - (" << player.pos.X() << ", " << player.pos.Y() << "), fr - " << player.animFrame << std::endl
            << "Goal: pos - (" << manager.goal.pos.X() << ", " << manager.goal.pos.Y() << ") " << (manager.goal.doesContain(player.pos, 1) ? "T" : "F") << std::endl
            << "Threat: pos - (" << manager.threat.pos.X() << ", " << manager.threat.pos.Y() << ") " << manager.curveT << " " << (manager.threat.doesContain(player.pos, 3) ? "T" : "F");
    }
    rec.setText(ss.str().c_str(), 254, 254, 254);

    glActiveTexture(GL_TEXTURE0);
    rec.Draw();

    Shader::ToggleShaders();
}

void resizeEvent(OpenGL* ogl, int newW, int newH)
{
    rec.setPosition(10, newH - 100 - 10);
}