#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <limits>

int windowWidth = 400;
int windowHeight = 600;

float playerX = windowWidth / 2.0f;
float playerY = windowHeight / 5.0f;
float playerWidth = 50.0f;
float playerHeight = 60.0f;
float playerVelX = 0.0f;
float playerVelY = 0.0f;
float moveSpeed = 4.0f;
float gravity = 0.3f;
float jumpStrength = 10.0f;
float boostedJumpStrength = 18.0f;
bool hasBoost = false;
int boostDuration = 300;
int boostTimer = 0;

void drawRect(float x, float y, float width, float height);

class Platform {
public:
    float x, y;
    float width = 60.0f;
    float height = 10.0f;
    bool moving = false;
    float velX = 2.0f;
    bool breakable = false;
    bool broken = false;

    Platform(float startX, float startY, bool isMoving = false, bool isBreakable = false)
        : x(startX), y(startY), moving(isMoving), breakable(isBreakable) {}

    void draw() const {
        if (broken) return;
        if (breakable) glColor3f(0.8f, 0.5f, 0.5f);
        else if (moving) glColor3f(0.4f, 0.4f, 0.9f);
        else glColor3f(0.5f, 0.25f, 0.0f);
        drawRect(x, y, width, height);
    }

    void update() {
        if (moving && !broken) {
            x += velX;
            if (x < width / 2 || x > windowWidth - width / 2) {
                velX *= -1;
            }
        }
    }
};

class Collectible {
public:
    float x, y;
    float size;
    bool active = true;

    Collectible(float startX, float startY, float itemSize)
        : x(startX), y(startY), size(itemSize) {}

    virtual ~Collectible() = default;

    virtual void draw() const = 0;

    bool checkCollision(float pX, float pY, float pWidth, float pHeight) {
        if (!active) return false;

        bool xOverlap = pX + pWidth / 2 > x - size / 2 &&
                        pX - pWidth / 2 < x + size / 2;
        bool yOverlap = pY + pHeight / 2 > y - size / 2 &&
                        pY - pHeight / 2 < y + size / 2;
        
        if (xOverlap && yOverlap) {
            active = false;
            return true;
        }
        return false;
    }
    
    virtual void applyEffect() = 0;
};

class Coin : public Collectible {
public:
    Coin(float startX, float startY) : Collectible(startX, startY, 15.0f) {}

    void draw() const override {
        if (!active) return;
        glColor3f(1.0f, 0.84f, 0.0f);
        drawRect(x, y, size, size);
    }

    void applyEffect() override {
        extern int coinsCollected;
        coinsCollected++;
    }
};

class HighJumpPowerUp : public Collectible {
public:
    HighJumpPowerUp(float startX, float startY) : Collectible(startX, startY, 20.0f) {}

    void draw() const override {
        if (!active) return;
        glColor3f(0.2f, 0.2f, 1.0f);
        drawRect(x, y, size, size);
    }

    void applyEffect() override {
        extern bool hasBoost;
        extern int boostTimer;
        extern int boostDuration;
        hasBoost = true;
        boostTimer = boostDuration;
    }
};


std::vector<Platform> platforms;
std::vector<Coin> coins;
std::vector<HighJumpPowerUp> highJumpPowerUps;

int initialPlatforms = 10;
float platformSpacing = 80.0f;

float cameraY = 0.0f;
int score = 0;
int highScore = 0;
int coinsCollected = 0;

enum GameState { MENU, PLAYING, GAME_OVER };
GameState gameState = MENU;

void drawRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x - width / 2, y - height / 2);
    glVertex2f(x + width / 2, y - height / 2);
    glVertex2f(x + width / 2, y + height / 2);
    glVertex2f(x - width / 2, y + height / 2);
    glEnd();
}

void renderBitmapString(float x, float y, void* font, const char* string) {
    glRasterPos2f(x, y);
    while (*string) {
        glutBitmapCharacter(font, *string);
        ++string;
    }
}

void drawPlayer() {
    glColor3f(0.9f, 0.1f, 0.1f);
    drawRect(playerX, playerY, playerWidth, playerHeight);
}

void drawPlatforms() {
    for (const auto& p : platforms) {
        p.draw();
    }
}

void drawCoins() {
    for (const auto& c : coins) {
        c.draw();
    }
}

void drawHighJumpPowerUps() {
    for (const auto& hjpu : highJumpPowerUps) {
        hjpu.draw();
    }
}

void generateInitialPlatforms() {
    platforms.clear();
    coins.clear();
    highJumpPowerUps.clear();

    platforms.emplace_back(windowWidth / 2.0f, 50.0f);
    coins.emplace_back(platforms.back().x, platforms.back().y + platforms.back().height / 2 + 7.5f + 5.0f);


    float currentY = platforms[0].y + platformSpacing;
    for (int i = 1; i < initialPlatforms; ++i) {
        float randX = (rand() % (windowWidth - 60)) + 30;
        bool isMoving = (rand() % 10 < 2);
        bool isBreakable = (rand() % 10 < 2 && !isMoving);
        
        platforms.emplace_back(randX, currentY, isMoving, isBreakable);
        coins.emplace_back(platforms.back().x, platforms.back().y + platforms.back().height / 2 + 7.5f + 5.0f);

        currentY += platformSpacing;
    }

    if (initialPlatforms > 4) {
        float randX_hj = (rand() % (windowWidth - 40)) + 20;
        int targetPlatformIndex = rand() % (platforms.size() / 2) + (platforms.size() / 3) ;
        float randY_hj = platforms[targetPlatformIndex].y + platforms[targetPlatformIndex].height / 2 + 10.0f + 5.0f;
        highJumpPowerUps.emplace_back(randX_hj, randY_hj);
    }
}

void generateNewPlatforms() {
    while (platforms.empty() || platforms.back().y < cameraY + windowHeight + platformSpacing) {
        float lastY = platforms.empty() ? cameraY - windowHeight : platforms.back().y;
        float randX = (rand() % (windowWidth - 60)) + 30;
        
        bool isMoving = (rand() % 10 < 2);
        bool isBreakable = (rand() % 10 < 2 && !isMoving);

        platforms.emplace_back(randX, lastY + platformSpacing, isMoving, isBreakable);
        score += 10;

        coins.emplace_back(platforms.back().x, platforms.back().y + platforms.back().height / 2 + 7.5f + 5.0f);

        if (rand() % 15 == 0) {
            float hjpuX = (rand() % (windowWidth - 60)) + 30;
            float hjpuY = platforms.back().y + platforms.back().height / 2 + 10.0f + (rand() % 20);
            highJumpPowerUps.emplace_back(hjpuX, hjpuY);
        }
    }
}

void removeOldPlatforms() {
    platforms.erase(std::remove_if(platforms.begin(), platforms.end(),
        [&](const Platform& p) {
            return p.y < cameraY - p.height;
        }), platforms.end());

    coins.erase(std::remove_if(coins.begin(), coins.end(),
        [&](const Coin& c) {
            return !c.active || c.y < cameraY - c.size;
        }), coins.end());

    highJumpPowerUps.erase(std::remove_if(highJumpPowerUps.begin(), highJumpPowerUps.end(),
        [&](const HighJumpPowerUp& hjpu) {
            return !hjpu.active || hjpu.y < cameraY - hjpu.size;
        }), highJumpPowerUps.end());
}

void resetGame() {
    playerX = windowWidth / 2.0f;
    playerY = windowHeight / 5.0f;
    playerVelX = 0.0f;
    playerVelY = 0.0f;
    cameraY = 0.0f;
    score = 0;
    coinsCollected = 0;
    hasBoost = false;
    boostTimer = 0;
    generateInitialPlatforms();
}

void updatePlatforms() {
    for (auto& p : platforms) {
        p.update();
    }
}

void update() {
    if (gameState != PLAYING) return;

    playerVelY -= gravity;
    playerY += playerVelY;
    playerX += playerVelX;

    if (playerX > windowWidth + playerWidth / 2) playerX = -playerWidth / 2;
    else if (playerX < -playerWidth / 2) playerX = windowWidth + playerWidth / 2;

    updatePlatforms();

    if (playerVelY < 0) {
        for (auto& p : platforms) {
            if (p.broken) continue;

            bool xOverlap = playerX + playerWidth / 2 > p.x - p.width / 2 &&
                            playerX - playerWidth / 2 < p.x + p.width / 2;

            if (xOverlap) { 
                float player_bottom_current = playerY - playerHeight / 2;
                float player_bottom_previous = (playerY - playerVelY) - playerHeight / 2;
                float platform_top_surface = p.y + p.height / 2;

                if (player_bottom_previous >= platform_top_surface &&
                    player_bottom_current < platform_top_surface) {    
                    
                    playerY = platform_top_surface + playerHeight / 2;
                    playerVelY = hasBoost ? boostedJumpStrength : jumpStrength;
                    if (p.breakable) p.broken = true;
                    break; 
                }
            }
        }
    }

    for (auto& c : coins) {
        if (c.checkCollision(playerX, playerY, playerWidth, playerHeight)) {
            c.applyEffect();
        }
    }

    for (auto& hjpu : highJumpPowerUps) {
        if (hjpu.checkCollision(playerX, playerY, playerWidth, playerHeight)) {
            hjpu.applyEffect();
        }
    }


    if (hasBoost) {
        boostTimer--;
        if (boostTimer <= 0) hasBoost = false;
    }

    if (playerY > cameraY + windowHeight / 2.0f) {
        cameraY = playerY - windowHeight / 2.0f;
    }

    generateNewPlatforms();
    removeOldPlatforms();

    if (playerY < cameraY - playerHeight) {
        gameState = GAME_OVER;
        if (score > highScore) highScore = score;
        std::cout << "Game Over! Final Score: " << score << std::endl;
    }

    glutPostRedisplay();
}

void setBackgroundColorByScore() {
    int stage = score / 100;
    switch (stage % 4) {
    case 0: glClearColor(0.8f, 0.9f, 1.0f, 1.0f); break;
    case 1: glClearColor(0.9f, 0.8f, 0.9f, 1.0f); break;
    case 2: glClearColor(0.9f, 0.9f, 0.7f, 1.0f); break;
    case 3: glClearColor(0.7f, 0.9f, 0.7f, 1.0f); break;
    }
}

void display() {
    setBackgroundColorByScore();
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (gameState == MENU) {
        glColor3f(0.2f, 0.6f, 1.0f);
        renderBitmapString(windowWidth / 2 - 80, windowHeight / 2 + 20, GLUT_BITMAP_HELVETICA_18, "Simple Jump Game");
        glColor3f(0.0f, 0.0f, 0.0f);
        renderBitmapString(windowWidth / 2 - 100, windowHeight / 2 - 20, GLUT_BITMAP_HELVETICA_18, "Press SPACE to Start");
    }
    else if (gameState == GAME_OVER) {
        glColor3f(0.8f, 0.1f, 0.1f);
        renderBitmapString(windowWidth / 2 - 60, windowHeight / 2 + 20, GLUT_BITMAP_HELVETICA_18, "Game Over!");
        std::stringstream ss;
        ss << "Final Score: " << score;
        renderBitmapString(windowWidth / 2 - 70, windowHeight / 2 - 10, GLUT_BITMAP_HELVETICA_18, ss.str().c_str());
        ss.str(""); ss.clear();
        ss << "High Score: " << highScore;
        renderBitmapString(windowWidth / 2 - 70, windowHeight / 2 - 30, GLUT_BITMAP_HELVETICA_18, ss.str().c_str());
        glColor3f(0.0f, 0.0f, 0.0f);
        renderBitmapString(windowWidth / 2 - 90, windowHeight / 2 - 50, GLUT_BITMAP_HELVETICA_18, "Press R to Restart");
    }
    else if (gameState == PLAYING) {
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, windowWidth, 0, windowHeight);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(0.0f, 0.0f, 0.0f);
        std::stringstream ss;
        ss << "Score: " << score;
        renderBitmapString(10.0f, windowHeight - 20.0f, GLUT_BITMAP_HELVETICA_18, ss.str().c_str());

        std::stringstream coin_ss;
        coin_ss << "Coins: " << coinsCollected;
        renderBitmapString(10.0f, windowHeight - 40.0f, GLUT_BITMAP_HELVETICA_18, coin_ss.str().c_str());


        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glTranslatef(0.0f, -cameraY, 0.0f);
        drawPlatforms();
        drawCoins();
        drawHighJumpPowerUps();
        drawPlayer();
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (gameState == MENU && key == 32) { // Space to start
        resetGame();
        gameState = PLAYING;
    }
    else if (gameState == GAME_OVER) {
        if (key == 'r' || key == 'R') { // Added 'R' for convenience
            resetGame();
            gameState = PLAYING;
        }
        else if (key == 27) { // ESC key
            gameState = MENU;
        }
    }
    else if (gameState == PLAYING) {
        if (key == 'a' || key == 'A') playerVelX = -moveSpeed; // Added 'A'
        else if (key == 'd' || key == 'D') playerVelX = moveSpeed; // Added 'D'
        else if (key == 27) { // ESC key
            gameState = MENU;
            playerVelX = 0.0f; // Stop horizontal movement when returning to menu
            // playerVelY = 0.0f; // Optional: resetGame() handles this if restarting
        }
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    if (gameState == PLAYING && (key == 'a' || key == 'd' || key == 'A' || key == 'D')) {
        playerVelX = 0.0f;
    }
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void timer(int) {
    update();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned int>(time(0)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Simple Jump Game");

    glClearColor(0.8f, 0.9f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}