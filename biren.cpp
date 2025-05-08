#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <limits> // For high score

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
float boostedJumpStrength = 18.0f; // For spring power-up
bool hasBoost = false;
int boostDuration = 300; // Frames, increased duration for high jump
int boostTimer = 0;

struct Platform {
    float x, y;
    float width = 60.0f;
    float height = 10.0f;
    bool moving = false;
    float velX = 2.0f;
    bool breakable = false;
    bool broken = false;
};

// REMOVE OLD PowerUp struct
// struct PowerUp {
// float x, y;
// float size = 20.0f;
// bool active = true;
// };

struct Coin {
    float x, y;
    float size = 15.0f; // Smaller than old power-ups
    bool active = true;
};

struct HighJumpPowerUp {
    float x, y;
    float size = 20.0f; // Similar to old power-ups, but blue
    bool active = true;
};

std::vector<Platform> platforms;
// REMOVE OLD powerUps vector
// std::vector<PowerUp> powerUps;
std::vector<Coin> coins;
std::vector<HighJumpPowerUp> highJumpPowerUps;

int initialPlatforms = 10;
float platformSpacing = 80.0f;

float cameraY = 0.0f;
int score = 0;
int highScore = 0;
int coinsCollected = 0; // New counter for coins

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
        if (p.broken) continue;
        if (p.breakable) glColor3f(0.8f, 0.5f, 0.5f); // Pink for breakable
        else if (p.moving) glColor3f(0.4f, 0.4f, 0.9f); // Blueish for moving
        else glColor3f(0.5f, 0.25f, 0.0f); // Brown for regular
        drawRect(p.x, p.y, p.width, p.height);
    }
}

// REMOVE OLD drawPowerUps
// void drawPowerUps() {
// for (const auto& pu : powerUps) {
// if (!pu.active) continue;
// glColor3f(1.0f, 0.8f, 0.2f);
// drawRect(pu.x, pu.y, pu.size, pu.size);
// }
// }

void drawCoins() {
    glColor3f(1.0f, 0.84f, 0.0f); // Gold color for coins
    for (const auto& c : coins) {
        if (!c.active) continue;
        drawRect(c.x, c.y, c.size, c.size);
    }
}

void drawHighJumpPowerUps() {
    glColor3f(0.2f, 0.2f, 1.0f); // Blue color for high jump power-up
    for (const auto& hjpu : highJumpPowerUps) {
        if (!hjpu.active) continue;
        drawRect(hjpu.x, hjpu.y, hjpu.size, hjpu.size);
    }
}

void generateInitialPlatforms() {
    platforms.clear();
    coins.clear();
    highJumpPowerUps.clear();

    platforms.push_back({ windowWidth / 2.0f, 50.0f });
    // Coin for the first platform
    coins.push_back({ platforms.back().x, platforms.back().y + platforms.back().height / 2 + 7.5f + 5.0f, 15.0f, true });


    float currentY = platforms[0].y + platformSpacing;
    for (int i = 1; i < initialPlatforms; ++i) {
        float randX = (rand() % (windowWidth - 60)) + 30;
        Platform p = { randX, currentY };
        int type = rand() % 10;
        if (type < 2) { p.moving = true; }
        else if (type < 4) { p.breakable = true; }
        platforms.push_back(p);
        // Add a coin for this platform
        coins.push_back({ p.x, p.y + p.height / 2 + 7.5f + 5.0f, 15.0f, true });

        currentY += platformSpacing;
    }

    // Add one initial high jump power-up if enough platforms
    if (initialPlatforms > 4) {
        float randX_hj = (rand() % (windowWidth - 40)) + 20;
        float randY_hj = platforms[initialPlatforms / 2].y + platformSpacing * 0.75f; // Place it interestingly
        highJumpPowerUps.push_back({randX_hj, randY_hj, 20.0f, true});
    }
}

void generateNewPlatforms() {
    while (platforms.back().y < cameraY + windowHeight + platformSpacing) {
        float lastY = platforms.back().y;
        float randX = (rand() % (windowWidth - 60)) + 30;
        Platform p = { randX, lastY + platformSpacing };
        int type = rand() % 10;
        if (type < 2) p.moving = true;
        else if (type < 4) p.breakable = true;
        platforms.push_back(p);
        score += 10;

        // Add a coin for this new platform
        coins.push_back({ p.x, p.y + p.height / 2 + 7.5f + 5.0f, 15.0f, true });

        // Scarce high jump power-up (e.g., 1 in 15 platforms might get one nearby)
        if (rand() % 15 == 0) {
            float hjpuX = (rand() % (windowWidth - 60)) + 30;
            // Place it a bit above the platform, can be anywhere horizontally
            float hjpuY = p.y + platformSpacing * (0.25f + (rand() % 50) / 100.0f); // Vary height a bit
            highJumpPowerUps.push_back({hjpuX, hjpuY, 20.0f, true});
        }
    }
}

void removeOldPlatforms() { // Also remove coins and powerups
    platforms.erase(std::remove_if(platforms.begin(), platforms.end(),
        [&](const Platform& p) {
            return p.y < cameraY - p.height;
        }), platforms.end());

    coins.erase(std::remove_if(coins.begin(), coins.end(),
        [&](const Coin& c) {
            return c.y < cameraY - c.size;
        }), coins.end());

    highJumpPowerUps.erase(std::remove_if(highJumpPowerUps.begin(), highJumpPowerUps.end(),
        [&](const HighJumpPowerUp& hjpu) {
            return hjpu.y < cameraY - hjpu.size;
        }), highJumpPowerUps.end());
}

void resetGame() {
    playerX = windowWidth / 2.0f;
    playerY = windowHeight / 5.0f;
    playerVelX = 0.0f;
    playerVelY = 0.0f;
    cameraY = 0.0f;
    score = 0;
    coinsCollected = 0; // Reset coin counter
    hasBoost = false;
    boostTimer = 0;
    generateInitialPlatforms(); // This will clear and repopulate platforms, coins, highJumpPowerUps
}

void updatePlatforms() {
    for (auto& p : platforms) {
        if (p.moving && !p.broken) {
            p.x += p.velX;
            if (p.x < p.width / 2 || p.x > windowWidth - p.width / 2) {
                p.velX *= -1;
            }
        }
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

    if (playerVelY < 0) { // Player is falling
        for (auto& p : platforms) {
            if (p.broken) continue;

            bool xOverlap = playerX + playerWidth / 2 > p.x - p.width / 2 &&
                            playerX - playerWidth / 2 < p.x + p.width / 2;

            if (xOverlap) { // Check for Y collision only if X overlaps
                float player_bottom_current = playerY - playerHeight / 2;
                // playerY has been updated by playerY += playerVelY.
                // So, playerY_old = playerY (current) - playerVelY (that was just added).
                float player_bottom_previous = (playerY - playerVelY) - playerHeight / 2;
                float platform_top_surface = p.y + p.height / 2;
                // float platform_bottom_surface = p.y - p.height / 2; // Not strictly needed for this logic

                // Check if player crossed the top surface of the platform from above
                if (player_bottom_previous >= platform_top_surface &&  // Was at or above platform top in previous step
                    player_bottom_current < platform_top_surface) {    // And is now below platform top (i.e., crossed it)
                    
                    playerY = platform_top_surface + playerHeight / 2; // Position player exactly on top
                    playerVelY = hasBoost ? boostedJumpStrength : jumpStrength;
                    if (p.breakable) p.broken = true;
                    // playJumpSound();  <-- SOUND placeholder
                    break; // Landed on one platform, no need to check others
                }
            }
        }
    }

    // Power-up collision (now for coins and high jump)
    // Coin collision
    for (auto& c : coins) {
        if (!c.active) continue;
        bool xOverlapCoin = playerX + playerWidth / 2 > c.x - c.size / 2 &&
                            playerX - playerWidth / 2 < c.x + c.size / 2;
        bool yOverlapCoin = playerY + playerHeight / 2 > c.y - c.size / 2 &&
                            playerY - playerHeight / 2 < c.y + c.size / 2;
        if (xOverlapCoin && yOverlapCoin) {
            c.active = false;
            coinsCollected++;
            // playCoinSound(); // Placeholder
        }
    }

    // High Jump PowerUp collision
    for (auto& hjpu : highJumpPowerUps) {
        if (!hjpu.active) continue;
        bool xOverlapHJPU = playerX + playerWidth / 2 > hjpu.x - hjpu.size / 2 &&
                            playerX - playerWidth / 2 < hjpu.x + hjpu.size / 2;
        bool yOverlapHJPU = playerY + playerHeight / 2 > hjpu.y - hjpu.size / 2 &&
                            playerY - playerHeight / 2 < hjpu.y + hjpu.size / 2;
        if (xOverlapHJPU && yOverlapHJPU) {
            hjpu.active = false;
            hasBoost = true;
            boostTimer = boostDuration; // Use the existing boost mechanism
            // playPowerUpSound(); // Placeholder
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
        // playGameOverSound();  <-- SOUND placeholder
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
        renderBitmapString(windowWidth / 2 - 90, windowHeight / 2 - 50, GLUT_BITMAP_HELVETICA_18, "Press R to Restart or ESC to Menu");
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

        // Display Coin Counter
        std::stringstream coin_ss;
        coin_ss << "Coins: " << coinsCollected;
        renderBitmapString(10.0f, windowHeight - 40.0f, GLUT_BITMAP_HELVETICA_18, coin_ss.str().c_str());


        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glTranslatef(0.0f, -cameraY, 0.0f);
        drawPlatforms();
        // drawPowerUps(); // Removed
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
    if (gameState == PLAYING && (key == 'a' || key == 'd' || key == 'A' || key == 'D')) { // Added 'A', 'D'
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