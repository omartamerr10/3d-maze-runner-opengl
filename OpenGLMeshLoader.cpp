#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <glut.h>
#include <vector>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <windows.h>     // For Windows API and PlaySound
#include <mmsystem.h>    // For PlaySound (winmm.lib needed)
#include <string>        // Include the string header
#pragma comment(lib, "winmm.lib")

int WIDTH = 1280;
int HEIGHT = 720;

GLuint tex;
// Maze dimensions
const int MAZE_ROWS = 21; // Must be odd
const int MAZE_COLS = 21; // Must be odd
const float CELL_SIZE = 4.0f;

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 500.0; // Increased far clipping plane

// Maze Layout (1 = wall, 0 = path)
int maze[MAZE_ROWS][MAZE_COLS];

void playStep(const char* filePath) {
    PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC);
}

// Function to stop any currently playing music
void stopStep() {
    PlaySound(NULL, NULL, SND_ASYNC);
}

void playJump(const char* filePath) {
    PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC);
}

// Function to stop any currently playing music
void stopJump() {
    PlaySound(NULL, NULL, SND_ASYNC);
}

void playDoor(const char* filePath) {
    PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC);
}

// Function to stop any currently playing music
void stopDoor() {
    PlaySound(NULL, NULL, SND_ASYNC);
}

void playKey(const char* filePath) {
    PlaySoundA(filePath, NULL, SND_FILENAME | SND_ASYNC);
}

// Function to stop any currently playing music
void stopKey() {
    PlaySound(NULL, NULL, SND_ASYNC);
}

class Vector
{
public:
    GLdouble x, y, z;
    Vector() {}
    Vector(GLdouble _x, GLdouble _y, GLdouble _z) : x(_x), y(_y), z(_z) {}
    void operator +=(const Vector& value)
    {
        x += value.x;
        y += value.y;
        z += value.z;
    }
    Vector operator *(float scalar) const
    {
        return Vector(x * scalar, y * scalar, z * scalar);
    }
    Vector operator +(const Vector& value) const
    {
        return Vector(x + value.x, y + value.y, z + value.z);
    }
    Vector operator -(const Vector& value) const
    {
        return Vector(x - value.x, y - value.y, z - value.z);
    }
};

Vector Eye(0, 25, -20);
Vector At(0, 0, 0);
Vector Up(0, 1, 0);

Vector playerPos(10, 0, 2); // Player position (two steps forward)
Vector playerDirection(0, 0, 1); // Player direction

Vector keyPos(5, 0, 5); // Key position
bool keyCollected = false;
Vector doorPos(15, 0, 15); // Door position
bool doorOpen = false;

Vector door2Pos(15, 0, 15); // Door2 position
bool door2Open = false;

int cameraZoom = 0;
bool firstPersonView = true; // Enable first-person view
bool globalView = false;

// Model Variables
Model_3DS model_house;
Model_3DS model_tree;
Model_3DS model_player;
Model_3DS model_barrier;
Model_3DS model_concreteBarrier;
Model_3DS model_bonyWall;
Model_3DS model_RoadBlockade1;
Model_3DS model_RoadBlockade2;
Model_3DS model_Cone;
Model_3DS model_caveWall;
Model_3DS model_stoneBridge;
Model_3DS model_rockWalkway;
Model_3DS model_caveWall1;
Model_3DS model_goldKey;
Model_3DS model_door;
Model_3DS model_key2;
Model_3DS model_door2;
Model_3DS model_barrier2;
Model_3DS model_walltorch;
Model_3DS model_sphere;


// Textures
GLTexture tex_ground;
GLTexture tex_stones;
GLTexture tex_skybox; // Add this line
GLTexture tex_darkStone;


std::vector<Vector> conePositions;
bool isJumping = false;
float jumpHeight = 0.0f;
float jumpSpeed = 0.2f;
float gravity = -0.01f;
Vector jumpDirection;

// Light source variables
GLfloat lightIntensity[] = { 0.7, 0.7, 0.7, 1.0f };
GLfloat lightPosition[] = { 0.0f, 100.0f, 0.0f, 1.0f }; // Ensure the last value is 1.0f for positional light
float lightAngle = 0.0f; // Declare lightAngle

// Light source variables for the torch
GLfloat torchLightIntensity[] = { 5.0f, 2.5f, 1.0f, 1.0f }; // Brighter light
GLfloat torchLightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Position will be updated dynamically

// Function to update the torch light source
void UpdateTorchLightSource()
{
    // Update the torch light position to follow the player, to the rightmost
    torchLightPosition[0] = playerPos.x + 10.0f; // Move further to the right
    torchLightPosition[1] = playerPos.y + jumpHeight + 1.8; // Adjust height for torch position
    torchLightPosition[2] = playerPos.z;

    // Simulate flickering intensity
    torchLightIntensity[0] = 5.0f + 0.5f * ((rand() % 100) / 100.0f - 0.5f);
    torchLightIntensity[1] = 2.5f + 0.2f * ((rand() % 100) / 100.0f - 0.5f);
    torchLightIntensity[2] = 1.0f;

    glLightfv(GL_LIGHT1, GL_POSITION, torchLightPosition);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, torchLightIntensity);
    glLightfv(GL_LIGHT1, GL_SPECULAR, torchLightIntensity); // Ensure specular light is updated
}

//=======================================================================
// Check Key Collision Function
//=======================================================================
bool level1Completed = false; // Track if level 1 is completed

std::vector<Vector> keyPositionsLevel2;
bool keysCollectedLevel2[5] = { false, false, false, false, false }; // Track collected keys in level 2
// Place scattered keys in level 2

std::vector<Vector> torchPositionsLevel2;

std::vector<Vector> barrierPositionsLevel2 = conePositions; // Use the same positions as the cones
float barrierSpeed = 0.009f; // Make the animation smoother and slower

//=======================================================================
// Lighting Configuration Function
//=======================================================================
void InitLightSource()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambient[] = { 0.1f, 0.1f, 0.1, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // Global ambient light
    GLfloat globalAmbient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    // Torch light properties
    glEnable(GL_LIGHT1); // Enable second light source
    GLfloat torch_ambient[] = { 0.2f, 0.2f, 0.1f, 1.0f };
    GLfloat torch_diffuse[] = { 5.0f, 2.5f, 1.0f, 1.0f }; // Brighter light
    GLfloat torch_specular[] = { 5.0f, 2.5f, 1.0f, 1.0f }; // Brighter light

    glLightfv(GL_LIGHT1, GL_AMBIENT, torch_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, torch_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, torch_specular);

    // Configure attenuation for smaller span
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.1f); // Smaller span
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.1f); // Smaller span
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.1f); // Smaller span
}

//=======================================================================
// Update Light Source Function
//=======================================================================
void UpdateLightSource()
{
    lightAngle += 0.01f; // Adjust the speed of rotation as needed
    if (lightAngle > 360.0f) {
        lightAngle -= 360.0f;
    }

    lightPosition[0] = 100.0f * cos(lightAngle);
    lightPosition[2] = 100.0f * sin(lightAngle);

    // Slightly change the intensity to simulate daytime transitions
    lightIntensity[0] = 0.7f + 0.3f * sin(lightAngle); // Increased amplitude for testing
    lightIntensity[1] = 0.7f + 0.3f * sin(lightAngle); // Increased amplitude for testing
    lightIntensity[2] = 0.7f + 0.3f * sin(lightAngle); // Increased amplitude for testing

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightIntensity); // Ensure diffuse light is updated

    // Update the torch light source if in the second environment
    if (level1Completed) {
        UpdateTorchLightSource();
    }
}

//=======================================================================
// Material Configuration Function
//======================================================================
void InitMaterial()
{
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

    GLfloat shininess[] = { 96.0f };
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

//=======================================================================
// OpenGL Configuration Function
//=======================================================================
void myInit(void)
{
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, aspectRatio, zNear, zFar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);

    InitLightSource();
    InitMaterial();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    // Initialize the torch light source
    glEnable(GL_LIGHT1);
    GLfloat ambient[] = { 0.2f, 0.1f, 0.0f, 1.0f }; // Slightly brighter ambient light
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, torchLightIntensity);
    glLightfv(GL_LIGHT1, GL_SPECULAR, torchLightIntensity);
    glLightfv(GL_LIGHT1, GL_POSITION, torchLightPosition);
}

//=======================================================================
// Render Ground Function
//=======================================================================
void RenderGround()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.6, 0.6, 0.6);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]);

    glPushMatrix();
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);
    glVertex3f(-100, 0, -100); // Increased floor size
    glTexCoord2f(10, 0);
    glVertex3f(100, 0, -100); // Increased floor size
    glTexCoord2f(10, 10);
    glVertex3f(100, 0, 100); // Increased floor size
    glTexCoord2f(0, 10);
    glVertex3f(-100, 0, 100); // Increased floor size
    glEnd();
    glPopMatrix();

    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);
}

//=======================================================================
// Maze Generation Function using Depth-First Search
//=======================================================================
void GenerateMaze()
{
    if (!level1Completed) {
        std::srand(0); // Set a fixed seed for reproducibility
        std::stack<std::pair<int, int>> stack;
        int directions[4][2] = { {0, 2}, {2, 0}, {0, -2}, {-2, 0} };

        // Initialize maze with walls
        for (int i = 0; i < MAZE_ROWS; ++i)
            for (int j = 0; j < MAZE_COLS; ++j)
                maze[i][j] = 1;

        // Start at the top-left corner
        int startX = 1;
        int startY = 1;
        maze[startX][startY] = 0;
        stack.push({ startX, startY });

        while (!stack.empty())
        {
            auto current = stack.top();
            stack.pop();

            std::vector<std::pair<int, int>> neighbors;
            for (auto& dir : directions)
            {
                int nx = current.first + dir[0], ny = current.second + dir[1];
                if (nx > 0 && nx < MAZE_ROWS - 1 && ny > 0 && ny < MAZE_COLS - 1 && maze[nx][ny] == 1)
                    neighbors.push_back({ nx, ny });
            }

            if (!neighbors.empty())
            {
                stack.push(current);
                auto next = neighbors[std::rand() % neighbors.size()];
                maze[(current.first + next.first) / 2][(current.second + next.second) / 2] = 0;
                maze[next.first][next.second] = 0;
                stack.push(next);
            }
        }

        // Place the key in a reachable location
        keyPos.x = 3 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2);
        keyPos.z = 3 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2);
        keyPos.y = CELL_SIZE / 2;

        // Create an exit in the maze
        for (int j = 0; j < MAZE_COLS; ++j) {
            maze[0][j] = 1; // Close all topmost cells
        }
        maze[0][MAZE_COLS / 2] = 0; // Open a cell in the middle of the topmost row
        doorPos.x = (MAZE_COLS / 2) * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2);
        doorPos.z = 0 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2);
        doorPos.y = CELL_SIZE / 2;

        // Place cones in scattered positions throughout the maze
        conePositions.clear();
        std::vector<Vector> potentialConePositions = {
            Vector(-38.0f, 2.0f, -38.0f),
            Vector(-30.0f, 2.0f, -20.0f),
            Vector(-25.0f, 2.0f, -10.0f),
            Vector(-15.0f, 2.0f, -5.0f),
            Vector(-5.0f, 2.0f, 15.0f),
            Vector(10.0f, 2.0f, -30.0f),
            Vector(20.0f, 2.0f, -25.0f),
            Vector(30.0f, 2.0f, -15.0f),
            Vector(5.0f, 2.0f, 25.0f),
            Vector(15.0f, 2.0f, 35.0f),
            Vector(-10.0f, 2.0f, 10.0f),  // New position 11
            Vector(0.0f, 2.0f, 20.0f),    // New position 12
            Vector(10.0f, 2.0f, 30.0f),   // New position 13
            Vector(20.0f, 2.0f, 40.0f),   // New position 14
            Vector(30.0f, 2.0f, 50.0f),   // New position 15
            Vector(40.0f, 2.0f, 60.0f)    // New position 16
        };

        for (const auto& pos : potentialConePositions) {
            int row = (pos.z + (MAZE_ROWS * CELL_SIZE / 2)) / CELL_SIZE;
            int col = (pos.x + (MAZE_COLS * CELL_SIZE / 2)) / CELL_SIZE;
            if (maze[row][col] == 0) {
                conePositions.push_back(pos);
            }
        }

        barrierPositionsLevel2 = conePositions; // Update barrier positions to match cone positions

        // Randomize torch positions
        torchPositionsLevel2.clear();
        for (int i = 0; i < 10; ++i) {
            int row, col;
            do {
                row = std::rand() % MAZE_ROWS;
                col = std::rand() % MAZE_COLS;
            } while (maze[row][col] != 1); // Ensure torches are placed on walls
            torchPositionsLevel2.push_back(Vector(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)));
        }

        // Randomize key positions
        keyPositionsLevel2.clear();
        for (int i = 0; i < 5; ++i) {
            int row, col;
            do {
                row = std::rand() % MAZE_ROWS;
                col = std::rand() % MAZE_COLS;
            } while (maze[row][col] != 0); // Ensure keys are placed on paths
            keyPositionsLevel2.push_back(Vector(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)));
        }
    }
}

//=======================================================================
// Render Maze Function
//=======================================================================
void RenderMaze()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.6, 0.6, 0.6);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_stones.texture[0]);

    for (int row = 0; row < MAZE_ROWS; ++row)
    {
        for (int col = 0; col < MAZE_COLS; ++col)
        {
            if (maze[row][col] == 1)
            {
                glPushMatrix();
                glTranslatef(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2));
                glScalef(CELL_SIZE, CELL_SIZE, CELL_SIZE);
                glColor3f(1, 1, 1); // Ensure the color is white to see the texture
                glBegin(GL_QUADS);
                // Front face
                glNormal3f(0, 0, 1);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, 0.5);
                // Back face
                glNormal3f(0, 0, -1);
                glTexCoord2f(0, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(-0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(0.5, 0.5, -0.5);
                // Left face
                glNormal3f(-1, 0, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(-0.5, -0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(-0.5, 0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, -0.5);
                // Right face
                glNormal3f(1, 0, 0);
                glTexCoord2f(0, 0); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(0.5, 0.5, 0.5);
                // Top face
                glNormal3f(0, 1, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, 0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, 0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, -0.5);
                // Bottom face
                glNormal3f(0, -1, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, -0.5, 0.5);
                glEnd();
                glPopMatrix();
            }
        }
    }

    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    //glDisable(GL_TEXTURE_2D);
}

//=======================================================================
// Render Torches Function for Level 2
//=======================================================================
void RenderTorchesLevel2()
{
    for (const auto& torchPos : torchPositionsLevel2) {
        glPushMatrix();
        glTranslatef(torchPos.x, torchPos.y, torchPos.z);
        glScalef(2.0, 2.0, 2.0); // Adjust the scale if needed
        model_walltorch.Draw(); // Draw the torch model
        glPopMatrix();
    }
}

//=======================================================================
// Render Barriers Function for Level 2
//=======================================================================
void RenderBarriersLevel2()
{
    for (auto& barrierPos : barrierPositionsLevel2) {
        glPushMatrix();
        barrierPos.y += barrierSpeed;
        if (barrierPos.y > 5.0f || barrierPos.y < 0.0f) {
            barrierSpeed = -barrierSpeed; // Reverse direction when reaching limits
        }
        glTranslatef(barrierPos.x, barrierPos.y, barrierPos.z);
        glScalef(1.5, 1.5, 1.5); // Adjust the scale if needed
        model_sphere.Draw(); // Draw the barrier model
        glPopMatrix();
    }
}


void RenderMazeLevel2(); // Forward declaration of RenderMazeLevel2

//=======================================================================
// Render Maze Function for Level 2
//=======================================================================
void RenderMazeLevel2()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.6, 0.6, 0.6);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_darkStone.texture[0]);

    for (int row = 0; row < MAZE_ROWS; ++row)
    {
        for (int col = 0; col < MAZE_COLS; ++col)
        {
            if (maze[row][col] == 1)
            {
                glPushMatrix();
                glTranslatef(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2));
                glScalef(CELL_SIZE, CELL_SIZE, CELL_SIZE);
                glColor3f(1, 1, 1); // Ensure the color is white to see the texture
                glBegin(GL_QUADS);
                // Front face
                glNormal3f(0, 0, 1);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, 0.5);
                // Back face
                glNormal3f(0, 0, -1);
                glTexCoord2f(0, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(-0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(0.5, 0.5, -0.5);
                // Left face
                glNormal3f(-1, 0, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(-0.5, -0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(-0.5, 0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, -0.5);
                // Right face
                glNormal3f(1, 0, 0);
                glTexCoord2f(0, 0); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(0.5, 0.5, 0.5);
                // Top face
                glNormal3f(0, 1, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, 0.5, 0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, 0.5, 0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, 0.5, -0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, 0.5, -0.5);
                // Bottom face
                glNormal3f(0, -1, 0);
                glTexCoord2f(0, 0); glVertex3f(-0.5, -0.5, -0.5);
                glTexCoord2f(1, 0); glVertex3f(0.5, -0.5, -0.5);
                glTexCoord2f(1, 1); glVertex3f(0.5, -0.5, 0.5);
                glTexCoord2f(0, 1); glVertex3f(-0.5, -0.5, 0.5);
                glEnd();
                glPopMatrix();
            }
        }
    }

    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    RenderTorchesLevel2(); // Render torches in level 2
    RenderBarriersLevel2(); // Render barriers in level 2

    //glDisable(GL_TEXTURE_2D);
}

//=======================================================================
// Render SkyBox Function
//=======================================================================
void RenderSkyBox2()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_skybox.texture[0]); // Use tex_skybox instead of tex

    float size = 100.0f;
    glBegin(GL_QUADS);
    // Front face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -size, -size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -size, -size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(size, size, -size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, size, -size);
    // Back face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(size, -size, size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-size, -size, size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-size, size, size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(size, size, size);
    // Left face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -size, size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-size, -size, -size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-size, size, -size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, size, size);
    // Right face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(size, -size, -size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -size, size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(size, size, size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(size, size, -size);
    // Top face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, size, -size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(size, size, -size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(size, size, size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, size, size);
    // Bottom face
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-size, -size, size);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(size, -size, size);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(size, -size, -size);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-size, -size, -size);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

//=======================================================================
// Check Collision Function
//=======================================================================
bool CheckCollision(Vector newPos)
{
    int row = (newPos.z + (MAZE_ROWS * CELL_SIZE / 2)) / CELL_SIZE;
    int col = (newPos.x + (MAZE_COLS * CELL_SIZE / 2)) / CELL_SIZE;

    // Debugging information
    if (row < 0 || row >= MAZE_ROWS || col < 0 || col >= MAZE_COLS) {
        printf("CheckCollision: Invalid row or col value. row: %d, col: %d\n", row, col);
        return true; // Treat as collision to prevent out-of-bounds access
    }

    // Check precise collision with maze walls
    float buffer = 10.01f; // Small buffer to prevent entering walls
    float playerSize = 0.005f; // Adjust player size as needed

    if (maze[row][col] == 1) {
        float left = col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2) - CELL_SIZE / 2 + buffer;
        float right = col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2) + CELL_SIZE / 2 - buffer;
        float top = row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2) - CELL_SIZE / 2 + buffer;
        float bottom = row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2) + CELL_SIZE / 2 - buffer;

        if (newPos.x - playerSize < left || newPos.x + playerSize > right || newPos.z - playerSize < top || newPos.z + playerSize > bottom) {
            return true;
        }
    }

    // Check collision with cones
    for (const auto& conePos : conePositions) {
        if (abs(newPos.x - conePos.x) < CELL_SIZE / 2.5 && abs(newPos.z - conePos.z) < CELL_SIZE / 2.5) {
            return true;
        }
    }

    // Check collision with door if not open
    if (!doorOpen) {
        if (abs(newPos.x - doorPos.x) < CELL_SIZE / 2.5 && abs(newPos.z - doorPos.z) < CELL_SIZE / 2.5) {
            return true;
        }
    }

    return false;
}

//=======================================================================
// Check collision with barrier function
//=======================================================================
bool CheckBarrierCollision(Vector newPos)
{
    int row = (newPos.z + (MAZE_ROWS * CELL_SIZE / 2)) / CELL_SIZE;
    int col = (newPos.x + (MAZE_COLS * CELL_SIZE / 2)) / CELL_SIZE;

    // Debugging information
    if (row < 0 || row >= MAZE_ROWS || col < 0 || col >= MAZE_COLS) {
        printf("CheckCollision: Invalid row or col value. row: %d, col: %d\n", row, col);
        return true; // Treat as collision to prevent out-of-bounds access
    }

    for (const auto& barrierPos : barrierPositionsLevel2) {
        if (abs(newPos.x - barrierPos.x) < CELL_SIZE / 2.5 && abs(newPos.z - barrierPos.z) < CELL_SIZE / 2.5) {
            return true;
        }
    }

    return false;
}


//=======================================================================
// Render Key Function
//=======================================================================
void RenderKey()
{
    if (!keyCollected)
    {
        glPushMatrix();
        glTranslatef(keyPos.x, keyPos.y, keyPos.z);
        glScalef(0.03, 0.03, 0.03); // Scale the key model appropriately
        model_goldKey.Draw(); // Draw the gold key model
        glPopMatrix();
    }
}

//=======================================================================
// Render Door Function
//=======================================================================
void RenderDoor()
{
    if (!level1Completed && !doorOpen) { // Only render door if level 1 is not completed and door is not open
        glPushMatrix();
        glTranslatef(doorPos.x, doorPos.y, doorPos.z);
        glRotatef(180, 0, 1, 0); // Rotate the door by 180 degrees on the Y-axis
        glScalef(0.8, 0.8, 0.8); // Scale the door model appropriately
        model_door.Draw(); // Draw the door model
        glPopMatrix();
    }
}

//=======================================================================
// Render Cones Function
//=======================================================================
void RenderCones()
{
    if (!level1Completed) { // Only render cones if level 1 is not completed
        glPushMatrix();
        for (const auto& conePos : conePositions) {
            glPushMatrix();
            glTranslatef(conePos.x, conePos.y, conePos.z);
            glScalef(0.05, 0.05, 0.05); // Make the cones even smaller
            model_Cone.Draw(); // Draw the cone model
            glPopMatrix();
        }
        glPopMatrix();
    }
}

//=======================================================================
// Render Keys Function for Level 2
//=======================================================================

void RenderKeysLevel2()
{
    for (int i = 0; i < keyPositionsLevel2.size(); ++i) {
        if (!keysCollectedLevel2[i]) {
            glPushMatrix();
            glTranslatef(keyPositionsLevel2[i].x, keyPositionsLevel2[i].y, keyPositionsLevel2[i].z);
            glScalef(5.3, 5.3, 5.3); // Scale the key model appropriately
            model_key2.Draw(); // Draw the key2 model
            glPopMatrix();
        }
    }
}


//=======================================================================
// Generate Maze Function for Level 2
//=======================================================================
void GenerateMazeLevel2()
{
    std::srand(0); // Set a fixed seed for reproducibility
    std::stack<std::pair<int, int>> stack;
    int directions[4][2] = { {0, 2}, {2, 0}, {0, -2}, {-2, 0} };

    // Initialize maze with walls
    for (int i = 0; i < MAZE_ROWS; ++i)
        for (int j = 0; j < MAZE_COLS; ++j)
            maze[i][j] = 1;

    // Start at the top-left corner
    int startX = 1;
    int startY = 1;
    maze[startX][startY] = 0;
    stack.push({ startX, startY });

    while (!stack.empty())
    {
        auto current = stack.top();
        stack.pop();

        std::vector<std::pair<int, int>> neighbors;
        for (auto& dir : directions)
        {
            int nx = current.first + dir[0], ny = current.second + dir[1];
            if (nx > 0 && nx < MAZE_ROWS - 1 && ny > 0 && ny < MAZE_COLS - 1 && maze[nx][ny] == 1)
                neighbors.push_back({ nx, ny });
        }

        if (!neighbors.empty())
        {
            stack.push(current);
            auto next = neighbors[std::rand() % neighbors.size()];
            maze[(current.first + next.first) / 2][(current.second + next.second) / 2] = 0;
            maze[next.first][next.second] = 0;
            stack.push(next);
        }
    }

    // Place the key in a reachable location
    keyPos.x = 5 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2);
    keyPos.z = 5 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2);
    keyPos.y = CELL_SIZE / 2;

    // Create an exit in the maze
    for (int j = 0; j < MAZE_COLS; ++j) {
        maze[0][j] = 1; // Close all topmost cells
    }
    maze[0][MAZE_COLS / 2] = 0; // Open a cell in the middle of the topmost row
    door2Pos.x = (MAZE_COLS / 2) * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2);
    door2Pos.z = 0 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2);
    door2Pos.y = CELL_SIZE / 2;

    // Ensure keys are placed in valid positions within the maze
    keyPositionsLevel2 = {
        Vector(3 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, 3 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)),
        Vector(5 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, 5 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)),
        Vector(7 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, 7 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)),
        Vector(9 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, 9 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)),
        Vector(11 * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, 11 * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2))
    };

    // Randomize torch positions
    torchPositionsLevel2.clear();
    for (int i = 0; i < 10; ++i) {
        int row, col;
        do {
            row = std::rand() % MAZE_ROWS;
            col = std::rand() % MAZE_COLS;
        } while (maze[row][col] != 1); // Ensure torches are placed on walls
        torchPositionsLevel2.push_back(Vector(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)));
    }

    // Randomize key positions
    keyPositionsLevel2.clear();
    for (int i = 0; i < 5; ++i) {
        int row, col;
        do {
            row = std::rand() % MAZE_ROWS;
            col = std::rand() % MAZE_COLS;
        } while (maze[row][col] != 0); // Ensure keys are placed on paths
        keyPositionsLevel2.push_back(Vector(col * CELL_SIZE - (MAZE_COLS * CELL_SIZE / 2), CELL_SIZE / 2, row * CELL_SIZE - (MAZE_ROWS * CELL_SIZE / 2)));
    }

    RenderKeysLevel2(); // Render scattered keys in level 2
}

int playerScore = 0; // Player's score

//=======================================================================
// Check Key Collision Function for Level 2
//=======================================================================
void CheckKeyCollisionLevel2()
{
    for (int i = 0; i < keyPositionsLevel2.size(); ++i) {
        if (!keysCollectedLevel2[i] && abs(playerPos.x - keyPositionsLevel2[i].x) < 1.0 && abs(playerPos.z - keyPositionsLevel2[i].z) < 1.0) {
            keysCollectedLevel2[i] = true;
            playKey("C:\\Users\\hp\\Downloads\\key.wav");
            stopKey();
            playerScore += 1; // Increment score for each key collected

            // Check if all keys are collected
            bool allKeysCollected = true;
            for (bool collected : keysCollectedLevel2) {
                if (!collected) {
                    allKeysCollected = false;
                    break;
                }
            }

            if (allKeysCollected) {
                door2Open = true;
                playDoor("C:\\Users\\hp\\Downloads\\door.wav");
            }
        }
    }
}

//=======================================================================
// Display Score Function
//=======================================================================
void DisplayScore()
{
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(10, HEIGHT - 20); // Position at the top-left corner

    std::string scoreText = "Score: " + std::to_string(playerScore);
    for (char c : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

//=======================================================================
// Check Key Collision Function
//=======================================================================
void CheckKeyCollision()
{
    if (!keyCollected && abs(playerPos.x - keyPos.x) < 1.0 && abs(playerPos.z - keyPos.z) < 1.0)
    {
        keyCollected = true;
        playKey("C:\\Users\\hp\\Downloads\\key.wav");
        stopKey();
        doorOpen = true;
        playDoor("C:\\Users\\hp\\Downloads\\door.wav");

        int doorRow = doorPos.z / CELL_SIZE;
        int doorCol = doorPos.x / CELL_SIZE;

        // Debugging information
        if (doorRow < 0 || doorRow >= MAZE_ROWS || doorCol < 0 || doorCol >= MAZE_COLS) {
            printf("CheckKeyCollision: Invalid doorRow or doorCol value. doorRow: %d, doorCol: %d\n", doorRow, doorCol);
            return;
        }

        maze[doorRow][doorCol] = 0; // Open the door in the maze

        playerScore = 1; // Set score to 1 when key is collected
    }
}

//=======================================================================
// Check Door Collision Function
//=======================================================================
void CheckDoorCollision()
{
    if (doorOpen && abs(playerPos.x - doorPos.x) < 1.0 && abs(playerPos.z - doorPos.z) < 1.0)
    {
        // Check if level 1 is completed
        if (!level1Completed) {
            level1Completed = true;
            GenerateMazeLevel2(); // Generate maze for level 2
            playerPos = Vector(10, 0, 0); // Reset player position
            keyCollected = false;
            doorOpen = false;
            playerScore = 0; // Reset score when entering the second environment
            conePositions.clear(); // Clear cones
        }
    }
}

//=======================================================================
// Render Door2 Function
//=======================================================================
void RenderDoor2()
{
    if (!door2Open) {
        glPushMatrix();
        glTranslatef(door2Pos.x, door2Pos.y, door2Pos.z);
        glRotatef(-90, 0, 1, 0); // Rotate the door 2 by -90 degrees on the Y-axis
        glScalef(0.8, 0.8, 0.8); // Scale the door model appropriately
        model_door2.Draw(); // Draw the door2 model
        glPopMatrix();
    }
}

//=======================================================================
// Check Door2 Collision Function
//=======================================================================
void CheckDoor2Collision()
{
    if (door2Open && abs(playerPos.x - door2Pos.x) < 1.0 && abs(playerPos.z - door2Pos.z) < 1.0)
    {
        // Handle the event when the player reaches the door2
        // For example, you can end the game or move to the next level
    }
}

//=======================================================================
// Display Function
//=======================================================================
void myDisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw Ground
    RenderGround();

    // Draw the maze
    if (level1Completed) {
        RenderMazeLevel2();
        RenderKeysLevel2(); // Ensure this line is present to render keys in level 2
        RenderDoor2(); // Render door2 in level 2
        // Clear cones and door
        conePositions.clear();
        doorOpen = false;
    }
    else {
        UpdateLightSource(); // Update the light source

        GLfloat lightIntensity[] = { 0.7, 0.7, 0.7, 1.0f };
        GLfloat lightPosition[] = { 0.0f, 100.0f, 0.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

        RenderMaze();
        // Draw the key
        RenderKey();
        // Draw the door
        RenderDoor();
        // Draw the cones
        RenderCones();
    }

    // Draw Player Model
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_stones.texture[0]);
    glPushMatrix();
    glTranslatef(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
    glRotatef(atan2(playerDirection.x, playerDirection.z) * 180 / 3.14159, 0, 1, 0);
    glScalef(2.5, 2.5, 2.5);
    model_player.Draw();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);

    // Apply torch light in level 2
    if (level1Completed) {
        GLfloat torchPosition[] = { playerPos.x + 1, playerPos.y + jumpHeight + 2.2, playerPos.z + 1, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, torchPosition);
    }

    // Sky box
    RenderSkyBox2();

    // Display the score
    DisplayScore();

    // Update the torch light source if in the second environment
    if (level1Completed) {
        UpdateTorchLightSource();
    }

    glutSwapBuffers();
}

//=======================================================================
// Special Keyboard Function
//=======================================================================
void mySpecialKeyboard(int key, int x, int y)
{
    Vector newPos = playerPos;
    switch (key)
    {
    case GLUT_KEY_UP:
        playStep("C:\\Users\\hp\\Downloads\\steps.wav");
        newPos.x += playerDirection.x;
        newPos.z += playerDirection.z;
        break;
    case GLUT_KEY_DOWN:
        playStep("C:\\Users\\hp\\Downloads\\steps.wav");
        newPos.x -= playerDirection.x;
        newPos.z -= playerDirection.z;
        break;
    case GLUT_KEY_LEFT:
        playStep("C:\\Users\\hp\\Downloads\\steps.wav");
        playerDirection = Vector(
            playerDirection.z,
            playerDirection.y,
            -playerDirection.x
        );
        break;
    case GLUT_KEY_RIGHT:
        playStep("C:\\Users\\hp\\Downloads\\steps.wav");
        playerDirection = Vector(
            -playerDirection.z,
            playerDirection.y,
            playerDirection.x
        );
        break;
    case GLUT_KEY_PAGE_UP: // Move camera up
        Eye.y += 1.0;
        break;
    case GLUT_KEY_PAGE_DOWN: // Move camera down
        Eye.y -= 1.0;
        break;
    case GLUT_KEY_HOME: // Zoom in
        Eye.z -= 1.0;
        break;
    case GLUT_KEY_END: // Zoom out
        Eye.z += 1.0;
        break;
    default:
        break;
    }

    if (!CheckCollision(newPos)) {
        if (CheckBarrierCollision(newPos)) {
            Vector knockbackPos = playerPos - playerDirection * 2.0f; // Calculate knockback position
            if (!CheckCollision(knockbackPos)) { // Ensure no collision with walls
                playerPos = knockbackPos;
            }
        }
        else {
            playerPos = newPos;
        }
    }

    CheckKeyCollision(); // Check if the player collected the keyglobalView
    CheckDoorCollision(); // Check if the player exited through the door
    CheckKeyCollisionLevel2(); // Check if the player collected any keys in level 2
    CheckDoor2Collision(); // Check if the player exited through the door2

    glLoadIdentity();
    if (firstPersonView) {
        // First-person view: Camera at the player's eye level
        Eye = Vector(playerPos.x, playerPos.y + jumpHeight + 1.8, playerPos.z); // Adjust height for eye level
        At = Eye + playerDirection;
        Up = Vector(0, 1, 0);

    }
    else if (globalView) {
        // Set the camera position above the center of the maze
        Eye = Vector(MAZE_COLS * CELL_SIZE / 300, 95, MAZE_ROWS * CELL_SIZE / 40); // Increase height as needed
        // Set the camera to look directly down at the center of the maze
        At = Vector(MAZE_COLS * CELL_SIZE / 300, 0, MAZE_ROWS * CELL_SIZE / 40);
        Up = Vector(0, 0, -1); // Adjust the up vector to ensure the correct orientation

        // Adjust the projection matrix to ensure the entire maze is visible
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, aspectRatio, zNear, zFar); // Adjust FOV if necessary
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
    }



    else {
        // Third-person view: Camera behind and above the player
        float offsetBehind = 20.0f; // Distance behind the player
        float offsetAbove = 20.0f;   // Height above the player

        Eye = Vector(
            playerPos.x - playerDirection.x * offsetBehind,
            playerPos.y + offsetAbove,
            playerPos.z - playerDirection.z * offsetBehind
        );
        At = Vector(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
        Up = Vector(0, 1, 0);

    }

    // Rotate the camera 180 degrees and move it a bit to the front
    Eye.x += playerDirection.x * 2.0;
    Eye.z += playerDirection.z * 2.0;
    At.x += playerDirection.x * 2.0;
    At.z += playerDirection.z * 2.0;

    // Update the view matrix
    glLoadIdentity();
    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);

    glutPostRedisplay();
}

//=======================================================================
// Keyboard Function
//=======================================================================
void myKeyboard(unsigned char button, int x, int y)
{
    switch (button)
    {
    case '1': // First-person view
        firstPersonView = true;
        globalView = false;
        break;

    case '2': // Third-person view
        firstPersonView = false;
        globalView = false;
        break;
    case '3': //Global view
        firstPersonView = false;
        globalView = true;
        break;

    case 32: // Space bar for jumping
        playJump("C:\\Users\\hp\\Downloads\\jump.wav");
        if (!isJumping) {
            isJumping = true;
            jumpSpeed = 0.2f; // Initial jump speed
            jumpDirection = playerDirection; // Set jump direction to current player direction
        }

        break;
    case 27:
        exit(0);
        break;
    default:
        break;
    }

    CheckKeyCollision(); // Check if the player collected the key
    CheckDoorCollision(); // Check if the player exited through the door
    CheckKeyCollisionLevel2(); // Check if the player collected any keys in level 2
    CheckDoor2Collision(); // Check if the player exited through the door2

    glLoadIdentity();
    if (firstPersonView) {
        // First-person view: Camera at the player's eye level
        Eye = Vector(playerPos.x, playerPos.y + jumpHeight + 1.8, playerPos.z); // Adjust height for eye level
        At = Eye + playerDirection;
        Up = Vector(0, 1, 0);

    }
    else if (globalView) {
        // Set the camera position above the center of the maze
        Eye = Vector(MAZE_COLS * CELL_SIZE / 300, 95, MAZE_ROWS * CELL_SIZE / 40); // Increase height as needed
        // Set the camera to look directly down at the center of the maze
        At = Vector(MAZE_COLS * CELL_SIZE / 300, 0, MAZE_ROWS * CELL_SIZE / 40);
        Up = Vector(0, 0, -1); // Adjust the up vector to ensure the correct orientation

        // Adjust the projection matrix to ensure the entire maze is visible
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, aspectRatio, zNear, zFar); // Adjust FOV if necessary
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
    }






    else {
        // Third-person view: Camera behind and above the player
        float offsetBehind = 20.0f; // Distance behind the player
        float offsetAbove = 20.0f;   // Height above the player

        Eye = Vector(
            playerPos.x - playerDirection.x * offsetBehind,
            playerPos.y + offsetAbove,
            playerPos.z - playerDirection.z * offsetBehind
        );
        At = Vector(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
        Up = Vector(0, 1, 0);

    }

    // Rotate the camera 180 degrees and move it a bit to the front
    Eye.x += playerDirection.x * 2.0;
    Eye.z += playerDirection.z * 2.0;
    At.x += playerDirection.x * 2.0;
    At.z += playerDirection.z * 2.0;

    // Update the view matrix
    glLoadIdentity();
    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);

    glutPostRedisplay();
}

//=======================================================================
// Motion Function
//=======================================================================
void myMotion(int x, int y)
{
    y = HEIGHT - y;

    if (cameraZoom - y > 0)
    {
        Eye.y += 0.1;
    }
    else
    {
        Eye.y -= 0.1;
    }

    cameraZoom = y;

    glLoadIdentity();
    if (firstPersonView) {
        // First-person view: Camera at the player's eye level
        Eye = Vector(playerPos.x, playerPos.y + jumpHeight + 1.8, playerPos.z); // Adjust height for eye level
        At = Eye + playerDirection;
        Up = Vector(0, 1, 0);

    }
    else if (globalView) {
        // Set the camera position above the center of the maze
        Eye = Vector(MAZE_COLS * CELL_SIZE / 300, 95, MAZE_ROWS * CELL_SIZE / 40); // Increase height as needed
        // Set the camera to look directly down at the center of the maze
        At = Vector(MAZE_COLS * CELL_SIZE / 300, 0, MAZE_ROWS * CELL_SIZE / 40);
        Up = Vector(0, 0, -1); // Adjust the up vector to ensure the correct orientation

        // Adjust the projection matrix to ensure the entire maze is visible
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, aspectRatio, zNear, zFar); // Adjust FOV if necessary
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
    }




    else {
        // Third-person view: Camera behind and above the player
        float offsetBehind = 20; // Distance behind the player
        float offsetAbove = 20;   // Height above the player

        Eye = Vector(
            playerPos.x - playerDirection.x * offsetBehind,
            playerPos.y + offsetAbove,
            playerPos.z - playerDirection.z * offsetBehind
        );
        At = Vector(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
        Up = Vector(0, 1, 0);

    }

    // Rotate the camera 180 degrees and move it a bit to the front
    Eye.x += playerDirection.x * 2.0;
    Eye.z += playerDirection.z * 2.0;
    At.x += playerDirection.x * 2.0;
    At.z += playerDirection.z * 2.0;

    // Update the view matrix
    glLoadIdentity();
    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);

    GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glutPostRedisplay();
}


//=======================================================================
// Mouse Function
//=======================================================================
void myMouse(int button, int state, int x, int y)
{
    y = HEIGHT - y;

    if (state == GLUT_DOWN)
    {
        cameraZoom = y;
    }
}

//=======================================================================
// Reshape Function
//=======================================================================
void myReshape(int w, int h)
{
    if (h == 0) {
        h = 1;
    }

    WIDTH = w;
    HEIGHT = h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fovy, (GLdouble)WIDTH / (GLdouble)HEIGHT, zNear, zFar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (firstPersonView) {
        // First-person view: Camera at the player's eye level
        Eye = Vector(playerPos.x, playerPos.y + jumpHeight + 1.8, playerPos.z); // Adjust height for eye level
        At = Eye + playerDirection;
        Up = Vector(0, 1, 0);

    }
    else if (globalView) {
        // Set the camera position above the center of the maze
        Eye = Vector(MAZE_COLS * CELL_SIZE / 300, 95, MAZE_ROWS * CELL_SIZE / 40); // Increase height as needed
        // Set the camera to look directly down at the center of the maze
        At = Vector(MAZE_COLS * CELL_SIZE / 300, 0, MAZE_ROWS * CELL_SIZE / 40);
        Up = Vector(0, 0, -1); // Adjust the up vector to ensure the correct orientation

        // Adjust the projection matrix to ensure the entire maze is visible
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, aspectRatio, zNear, zFar); // Adjust FOV if necessary
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
    }




    else {
        // Third-person view: Camera behind and above the player
        float offsetBehind = 20.0f; // Distance behind the player
        float offsetAbove = 20.0f;   // Height above the player

        Eye = Vector(
            playerPos.x - playerDirection.x * offsetBehind,
            playerPos.y + offsetAbove,
            playerPos.z - playerDirection.z * offsetBehind
        );
        At = Vector(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
        Up = Vector(0, 1, 0);

    }

    // Rotate the camera 180 degrees and move it a bit to the front
    Eye.x += playerDirection.x * 2.0;
    Eye.z += playerDirection.z * 2.0;
    At.x += playerDirection.x * 2.0;
    At.z += playerDirection.z * 2.0;

    gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
}

//=======================================================================
// Idle Function
//=======================================================================
void myIdle()
{
    if (isJumping) {
        jumpHeight += jumpSpeed;
        jumpSpeed += gravity; // Apply gravity

        if (jumpHeight <= 0.0f) {
            jumpHeight = 0.0f;
            jumpSpeed = 0.0f;
            isJumping = false;
        }
        else {

            Vector newPos(playerPos.x, playerPos.y + jumpHeight, playerPos.z);
            if (!CheckCollision(newPos)) {
                if (CheckBarrierCollision(newPos)) {
                    Vector knockbackPos = playerPos - playerDirection * 2.0f; // Calculate knockback position
                    if (!CheckCollision(knockbackPos)) { // Ensure no collision with walls
                        playerPos = knockbackPos;
                    }
                }
                else {
                    playerPos += jumpDirection * 0.1; // Move player forward while jumping

                }
            }
            else { // do not allow to jump through walls
                jumpHeight = 0.0f;
                jumpSpeed = 0.0f;
                isJumping = false;
            }
        }
    }

    UpdateLightSource(); // Update the light source
    CheckDoor2Collision(); // Check if the player exited through the door2

    // Update the torch light source if in the second environment
    if (level1Completed) {
        UpdateTorchLightSource();
    }

    glutPostRedisplay();
}

//=======================================================================
// Assets Loading Function
//=======================================================================
void LoadAssets()
{
    model_house.Load("Models/house/house.3DS");
    model_tree.Load("Models/tree/Tree1.3ds");
    model_player.Load("Models/Player/knight.3dS");
    model_concreteBarrier.Load("Models/ConcreteBarrier/ConcreteBarrier.3dS");
    model_bonyWall.Load("Models/wall/BonyWall1.3dS");
    model_RoadBlockade1.Load("Models/wall/RoadBlockade_01.3dS");
    model_RoadBlockade2.Load("Models/wall/RoadBlockade_02.3dS");
    model_Cone.Load("Models/decoration/cone.3dS");
    model_caveWall.Load("Models/wall/caveWall.3dS");
    model_stoneBridge.Load("Models/decoration/stoneBridge.3dS");
    model_rockWalkway.Load("Models/decoration/rockWalkway.3dS");
    model_caveWall1.Load("Models/wall/caveWall1.3dS");
    model_goldKey.Load("Models/decoration/goldKey.3dS");
    model_door.Load("Models/decoration/door.3dS");
    model_key2.Load("Models/decoration/key2.3dS"); // Ensure this line is present
    model_door2.Load("Models/decoration/door2.3dS");
    model_barrier2.Load("Models/decoration/barrierBrick.3dS");
    model_walltorch.Load("Models/decoration/walltorch.3dS");
    model_sphere.Load("Models/decoration/sphere.3dS");

    tex_ground.Load("Textures/ground.bmp");
    tex_skybox.Load("Textures/blu-sky-3.bmp"); // Use tex_skybox instead of tex
    tex_stones.Load("Textures/stones.bmp");
    tex_darkStone.Load("Textures/darkStone.bmp");
}

const char* title = "The Labyrinth Legend"; // Add this line

//=======================================================================
// Main Function
//=======================================================================
void main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitWindowPosition(100, 150);
    glutCreateWindow(title); // Use the defined title variable

    glutDisplayFunc(myDisplay);
    glutKeyboardFunc(myKeyboard);
    glutSpecialFunc(mySpecialKeyboard); // Add this line
    glutMotionFunc(myMotion);
    glutMouseFunc(myMouse);
    glutReshapeFunc(myReshape);
    glutIdleFunc(myIdle); // Add this line

    myInit();
    LoadAssets();
    GenerateMaze();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);

    glutMainLoop();
}