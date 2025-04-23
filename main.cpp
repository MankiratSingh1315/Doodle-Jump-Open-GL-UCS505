#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

int windowWidth = 400;
int windowHeight = 600;

float playerX = windowWidth / 2.0f;
float playerY = windowHeight / 5.0f;
float playerWidth = 20.0f;
float playerHeight = 30.0f;
float playerVelX = 0.0f;
float playerVelY = 0.0f;
float moveSpeed = 4.0f;
float gravity = 0.3f;
float jumpStrength = 10.0f;

struct Platform {
    float x, y;
    float width = 60.0f;
    float height = 10.0f;
};
std::vector<Platform> platforms;
int initialPlatforms = 10;
float platformSpacing = 80.0f;

float cameraY = 0.0f;
int score = 0;
bool gameOver = false;

void drawRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x - width / 2, y - height / 2);
    glVertex2f(x + width / 2, y - height / 2);
    glVertex2f(x + width / 2, y + height / 2);
    glVertex2f(x - width / 2, y + height / 2);
    glEnd();
}

void drawPlayer() {
    glColor3f(0.0f, 1.0f, 0.0f);
    drawRect(playerX, playerY, playerWidth, playerHeight);
}

void drawPlatforms() {
    glColor3f(0.5f, 0.25f, 0.0f);
    for (const auto& p : platforms) {
        drawRect(p.x, p.y, p.width, p.height);
    }
}

void generateInitialPlatforms() {
    platforms.clear();
    platforms.push_back({windowWidth / 2.0f, 50.0f});
    float currentY = platforms[0].y + platformSpacing;
    for (int i = 1; i < initialPlatforms; ++i) {
        float randX = (rand() % (windowWidth - (int)platforms[0].width)) + platforms[0].width / 2;
        platforms.push_back({randX, currentY});
        currentY += platformSpacing;
    }
}

void generateNewPlatforms() {
    while (platforms.back().y < cameraY + windowHeight + platformSpacing) {
        float lastY = platforms.back().y;
        float randX = (rand() % (windowWidth - (int)platforms[0].width)) + platforms[0].width / 2;
        platforms.push_back({randX, lastY + platformSpacing});
        score += 10;
    }
}

void removeOldPlatforms() {
    platforms.erase(std::remove_if(platforms.begin(), platforms.end(),
        [&](const Platform& p) {
            return p.y < cameraY - p.height;
        }), platforms.end());
}

void update() {
    if (gameOver) return;
    playerVelY -= gravity;
    playerY += playerVelY;
    playerX += playerVelX;
    if (playerX > windowWidth + playerWidth / 2) {
        playerX = -playerWidth / 2;
    } else if (playerX < -playerWidth / 2) {
        playerX = windowWidth + playerWidth / 2;
    }
    if (playerVelY < 0) {
        for (const auto& p : platforms) {
            bool xOverlap = playerX + playerWidth / 2 > p.x - p.width / 2 &&
                            playerX - playerWidth / 2 < p.x + p.width / 2;
            bool yOverlap = playerY - playerHeight / 2 < p.y + p.height / 2 &&
                            playerY - playerHeight / 2 > p.y - p.height / 2;
            if (xOverlap && yOverlap) {
                playerY = p.y + p.height / 2 + playerHeight / 2;
                playerVelY = jumpStrength;
                break;
            }
        }
    }
    if (playerY > cameraY + windowHeight / 2.0f) {
        cameraY = playerY - windowHeight / 2.0f;
    }
    generateNewPlatforms();
    removeOldPlatforms();
    if (playerY < cameraY - playerHeight) {
        gameOver = true;
        std::cout << "Game Over! Final Score: " << score << std::endl;
        std::cout << "Press 'r' to restart." << std::endl;
    }
    glutPostRedisplay();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, -cameraY, 0.0f);
    drawPlayer();
    drawPlatforms();
    if (gameOver) {
    }
    glutSwapBuffers();
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, w, 0.0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'r' || key == 'R') {
        if (gameOver) {
            playerX = windowWidth / 2.0f;
            playerY = windowHeight / 5.0f;
            playerVelX = 0.0f;
            playerVelY = 0.0f;
            cameraY = 0.0f;
            score = 0;
            gameOver = false;
            generateInitialPlatforms();
        }
    } else if (key == 27) {
        exit(0);
    }
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            playerVelX = -moveSpeed;
            break;
        case GLUT_KEY_RIGHT:
            playerVelX = moveSpeed;
            break;
    }
}

void specialKeysUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:
            if (playerVelX < 0.0f) playerVelX = 0.0f;
            break;
        case GLUT_KEY_RIGHT:
            if (playerVelX > 0.0f) playerVelX = 0.0f;
            break;
    }
}

void timer(int value) {
    update();
    glutTimerFunc(16, timer, 0);
}

void init() {
    glClearColor(0.8f, 0.9f, 1.0f, 1.0f);
    srand(time(0));
    generateInitialPlatforms();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Simple Doodle Jump Clone");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}
