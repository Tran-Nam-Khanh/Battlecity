#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <vector>
#include <algorithm>
#include <ctime>
using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 40;
const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;  // 20 ô
const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE; // 15 ô

class Wall {
public:
    int x, y;
    SDL_Rect rect;
    bool active;

    Wall(int startX, int startY) {
        x = startX;
        y = startY;
        active = true;
        rect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    }

    void render(SDL_Renderer* renderer) {
        if (active) {
            SDL_SetRenderDrawColor(renderer, 150, 75, 0, 255); // Màu nâu
            SDL_RenderFillRect(renderer, &rect);
        }
    }
};

class Bullet {
public:
    int x, y;
    int dx, dy;
    SDL_Rect rect;
    bool active;
    float scaleFactor;

    Bullet(int startX, int startY, int dirX, int dirY) {
        x = startX;
        y = startY;
        dx = dirX;
        dy = dirY;
        active = true;
        scaleFactor = 2.0f; // Tăng kích thước bullet lên 2 lần
        int baseSize = 10; // Kích thước cơ bản
        int scaledSize = static_cast<int>(baseSize * scaleFactor); // Kích thước mới
        rect = { x, y, scaledSize, scaledSize }; // Đã sửa lỗi "tempat"
    }

    void move() {
        x += dx;
        y += dy;
        rect.x = x;
        rect.y = y;

        if (x < TILE_SIZE || x > SCREEN_WIDTH - TILE_SIZE ||
            y < TILE_SIZE || y > SCREEN_HEIGHT - TILE_SIZE) {
            active = false;
        }
    }

    void render(SDL_Renderer* renderer, SDL_Texture* bulletTexture) {
        if (active) {
            SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
        }
    }
};

class PlayerTank {
public:
    int x, y;
    int dirX, dirY;
    SDL_Rect rect;
    float scaleFactor;

    PlayerTank(int startX, int startY) {
        x = startX;
        y = startY;
        scaleFactor = 1.5f; // Tăng kích thước tank lên 1.5 lần
        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor); // Kích thước mới
        rect = { x * TILE_SIZE, y * TILE_SIZE, scaledSize, scaledSize }; // Áp dụng kích thước mới
        dirX = 0;
        dirY = -1;
    }

    void move(int dx, int dy, const vector<Wall>& walls) {
        int newX = x + dx;
        int newY = y + dy;

        dirX = dx;
        dirY = dy;

        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor);
        SDL_Rect newRect = { newX * TILE_SIZE, newY * TILE_SIZE, scaledSize, scaledSize };
        for (const auto& wall : walls) {
            if (wall.active && SDL_HasIntersection(&newRect, &wall.rect)) {
                return;
            }
        }

        if (newX >= 1 && newX < MAP_WIDTH - 1 && newY >= 1 && newY < MAP_HEIGHT - 1) {
            x = newX;
            y = newY;
            rect.x = x * TILE_SIZE;
            rect.y = y * TILE_SIZE;
            rect.w = scaledSize;
            rect.h = scaledSize;
        }
    }

    Bullet shoot() {
        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor);
        int bulletX = x * TILE_SIZE + scaledSize / 2 - static_cast<int>(10 * 2.0f / 2); // Đạn xuất phát từ giữa tank
        int bulletY = y * TILE_SIZE + scaledSize / 2 - static_cast<int>(10 * 2.0f / 2);

        int bulletSpeed = 5;
        int bulletDirX = dirX * bulletSpeed;
        int bulletDirY = dirY * bulletSpeed;

        if (dirX == 0 && dirY == 0) {
            bulletDirX = 0;
            bulletDirY = -bulletSpeed;
        }

        return Bullet(bulletX, bulletY, bulletDirX, bulletDirY);
    }

    void render(SDL_Renderer* renderer, SDL_Texture* tankTexture) {
        SDL_RenderCopy(renderer, tankTexture, NULL, &rect);
    }
};

class EnemyTank {
public:
    int x, y;
    int dirX, dirY;
    int moveDelay, shootDelay;
    SDL_Rect rect;
    bool active;
    vector<Bullet> bullets;
    float scaleFactor;

    EnemyTank(int startX, int startY) {
        moveDelay = 15;
        shootDelay = 100;
        x = startX;
        y = startY;
        scaleFactor = 1.5f; // Tăng kích thước tank lên 1.5 lần
        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor); // Kích thước mới
        rect = { x * TILE_SIZE, y * TILE_SIZE, scaledSize, scaledSize }; // Áp dụng kích thước mới
        dirX = 0;
        dirY = 1;
        active = true;
    }

    void move(const vector<Wall>& walls) {
        if (--moveDelay > 0) return;
        moveDelay = 15;

        int r = rand() % 4;
        if (r == 0) { dirX = 0; dirY = -1; }
        else if (r == 1) { dirX = 0; dirY = 1; }
        else if (r == 2) { dirY = 0; dirX = -1; }
        else if (r == 3) { dirY = 0; dirX = 1; }

        int newX = x + this->dirX;
        int newY = y + this->dirY;

        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor);
        SDL_Rect newRect = { newX * TILE_SIZE, newY * TILE_SIZE, scaledSize, scaledSize };
        for (const auto& wall : walls) {
            if (wall.active && SDL_HasIntersection(&newRect, &wall.rect)) {
                return;
            }
        }

        if (newX >= 1 && newX < MAP_WIDTH - 1 && newY >= 1 && newY < MAP_HEIGHT - 1) {
            x = newX;
            y = newY;
            rect.x = x * TILE_SIZE;
            rect.y = y * TILE_SIZE;
            rect.w = scaledSize;
            rect.h = scaledSize;
        }
    }

    void shoot() {
        if (--shootDelay > 0) return;
        shootDelay = 15;

        int scaledSize = static_cast<int>(TILE_SIZE * scaleFactor);
        bullets.push_back(Bullet(
            x * TILE_SIZE + scaledSize / 2 - static_cast<int>(10 * 2.0f / 2),
            y * TILE_SIZE + scaledSize / 2 - static_cast<int>(10 * 2.0f / 2),
            dirX * 5,
            dirY * 5
        ));
    }

    void updateBullets() {
        for (auto& bullet : bullets) {
            bullet.move();
        }
        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b) { return !b.active; }),
            bullets.end()
        );
    }

    void render(SDL_Renderer* renderer, SDL_Texture* tankTexture, SDL_Texture* bulletTexture) {
        if (active) {
            SDL_RenderCopy(renderer, tankTexture, NULL, &rect);
            for (auto& bullet : bullets) {
                bullet.render(renderer, bulletTexture);
            }
        }
    }
};
enum GameState {
    MENU,
    GAMEPLAY
};
class Game {
public:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    GameState state; // New: Track game state
    vector<Wall> walls;
    vector<Bullet> bullets;
    int enemyNumber = 3;
    vector<EnemyTank> enemies;
    PlayerTank player;
    SDL_Texture* tankTexture;
    SDL_Texture* bulletTexture;
    TTF_Font* font; // New: For menu text
    SDL_Texture* startTextTexture; // New: Texture for "Start" text

    Game() : player(MAP_WIDTH / 2, MAP_HEIGHT - 2) {
        running = true;
        state = MENU; // Start in menu state

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << endl;
            running = false;
        }

        // Load font (adjust path to a valid .ttf file on your system)
        font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 40); // Example path, replace with your font path
        if (!font) {
            cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << endl;
            running = false;
        }

        // Create "Start" text texture
        SDL_Color textColor = { 255, 255, 255, 255 }; // White text
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Start", textColor);
        if (!textSurface) {
            cerr << "Failed to create text surface! TTF_Error: " << TTF_GetError() << endl;
            running = false;
        }
        startTextTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        // Load textures
        tankTexture = IMG_LoadTexture(renderer, "C:/Users/NAM KHANH/OneDrive/Pictures/Screenshots/sdl2/sdl2/Project1/x64/Debug/tank.png");
        if (!tankTexture) {
            cerr << "Failed to load tank texture! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }
        bulletTexture = IMG_LoadTexture(renderer, "C:/Users/NAM KHANH/OneDrive/Pictures/Screenshots/sdl2/sdl2/Project1/x64/Debug/bullet.png");
        if (!bulletTexture) {
            cerr << "Failed to load bullet texture! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        generateWalls();
        spawnEnemies();
    }

    void generateWalls() {
        for (int i = 3; i < MAP_HEIGHT - 3; i += 2) {
            for (int j = 3; j < MAP_WIDTH - 3; j += 2) {
                walls.push_back(Wall(j, i));
            }
        }
    }

    void spawnEnemies() {
        enemies.clear();
        for (int i = 0; i < enemyNumber; ++i) {
            int ex, ey;
            bool validPosition = false;
            while (!validPosition) {
                ex = rand() % (MAP_WIDTH - 2) + 1;
                ey = rand() % (MAP_HEIGHT - 2) + 1;
                validPosition = true;
                for (const auto& wall : walls) {
                    if (wall.active && wall.x == ex && wall.y == ey) {
                        validPosition = false;
                        break;
                    }
                }
            }
            enemies.push_back(EnemyTank(ex, ey));
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (state == MENU) {
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
                    state = GAMEPLAY; // Start game on Enter key
                }
            }
            else if (state == GAMEPLAY) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        player.move(0, -1, walls);
                        break;
                    case SDLK_DOWN:
                        player.move(0, 1, walls);
                        break;
                    case SDLK_LEFT:
                        player.move(-1, 0, walls);
                        break;
                    case SDLK_RIGHT:
                        player.move(1, 0, walls);
                        break;
                    case SDLK_SPACE:
                        bullets.push_back(player.shoot());
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    }
                }
            }
        }
    }

    void update() {
        if (state == GAMEPLAY) {
            for (auto& bullet : bullets) {
                bullet.move();
                for (auto& wall : walls) {
                    if (bullet.active && wall.active && SDL_HasIntersection(&bullet.rect, &wall.rect)) {
                        bullet.active = false;
                        wall.active = false;
                        break;
                    }
                }
                for (auto& enemy : enemies) {
                    if (bullet.active && enemy.active && SDL_HasIntersection(&bullet.rect, &enemy.rect)) {
                        bullet.active = false;
                        enemy.active = false;
                        break;
                    }
                }
            }

            bullets.erase(
                remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.active; }),
                bullets.end()
            );

            enemies.erase(
                remove_if(enemies.begin(), enemies.end(), [](const EnemyTank& e) { return !e.active; }),
                enemies.end()
            );

            for (auto& enemy : enemies) {
                if (enemy.active) {
                    enemy.move(walls);
                    enemy.shoot();
                    enemy.updateBullets();

                    if (SDL_HasIntersection(&player.rect, &enemy.rect)) {
                        running = false;
                    }

                    for (auto& enemyBullet : enemy.bullets) {
                        if (enemyBullet.active && SDL_HasIntersection(&enemyBullet.rect, &player.rect)) {
                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Over", "You Lose", window);
                            running = false;
                            break;
                        }
                    }

                    for (auto& playerBullet : bullets) {
                        for (auto& enemyBullet : enemy.bullets) {
                            if (playerBullet.active && enemyBullet.active &&
                                SDL_HasIntersection(&playerBullet.rect, &enemyBullet.rect)) {
                                playerBullet.active = false;
                                enemyBullet.active = false;
                            }
                        }
                    }

                    for (auto& enemyBullet : enemy.bullets) {
                        for (auto& wall : walls) {
                            if (enemyBullet.active && wall.active &&
                                SDL_HasIntersection(&enemyBullet.rect, &wall.rect)) {
                                enemyBullet.active = false;
                                wall.active = false;
                                break;
                            }
                        }
                    }
                }
            }

            if (enemies.empty()) {
                cout << "YOU WIN ! YOU ARE NUMBER 1 !" << endl;
                running = false;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray background
        SDL_RenderClear(renderer);

        if (state == MENU) {
            // Render "Start" text in the center
            int textW, textH;
            SDL_QueryTexture(startTextTexture, NULL, NULL, &textW, &textH);
            SDL_Rect textRect = { (SCREEN_WIDTH - textW) / 2, (SCREEN_HEIGHT - textH) / 2, textW, textH };
            SDL_RenderCopy(renderer, startTextTexture, NULL, &textRect);
        }
        else if (state == GAMEPLAY) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            for (int i = 1; i < MAP_HEIGHT - 1; ++i) {
                for (int j = 1; j < MAP_WIDTH - 1; ++j) {
                    SDL_Rect tile = { j * TILE_SIZE, i * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    SDL_RenderFillRect(renderer, &tile);
                }
            }

            for (auto& wall : walls) {
                wall.render(renderer);
            }

            for (auto& bullet : bullets) {
                bullet.render(renderer, bulletTexture);
            }

            player.render(renderer, tankTexture);

            for (auto& enemy : enemies) {
                enemy.render(renderer, tankTexture, bulletTexture);
            }
        }

        SDL_RenderPresent(renderer);
    }

    void run() {
        while (running) {
            handleEvents();
            update();
            render();
            SDL_Delay(16);
        }
    }

    ~Game() {
        SDL_DestroyTexture(tankTexture);
        SDL_DestroyTexture(bulletTexture);
        SDL_DestroyTexture(startTextTexture);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    srand(time(0));
    Game game;
    if (game.running) {
        game.run();
    }
    return 0;
}
