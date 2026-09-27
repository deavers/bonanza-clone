#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

// Standart W and H (SEGA GENESIS)
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 224;
constexpr int SCALE = 3;
constexpr int TILE = 16;
constexpr int PLAYER_W = 16, PLAYER_H = 22;

enum class GuardMode
{
    Patrol,
    Chase
};

struct Guard
{
    float x, y;
    int dir = 1;
    GuardMode mode = GuardMode::Patrol;
    float alertTimer = 0.0f;
};

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!IMG_Init(IMG_INIT_PNG))
    {
        std::fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "bonanza-clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * SCALE, SCREEN_H * SCALE,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    // Load Level
    std::vector<std::string> level;
    {
        std::ifstream f("levels/level1.txt");
        if (!f) { printf("Cannot open levels/level1.txt\n"); return 1; }
        std::string line;
        while (std::getline(f, line)) level.push_back(line);
    }
    const int levelH = (int)level.size();
    const int levelW = (int)level[0].size();

    float spawnX = 32.0f, spawnY = 32.0f;
    std::vector<Guard> guards;
    for (int ty = 0; ty < levelH; ty++)
        for (int tx = 0; tx < levelW; tx++)
        {
            if (level[ty][tx] == 'P') { spawnX = (float)(tx * TILE); spawnY = (float)(ty * TILE); }
            if (level[ty][tx] == 'G') guards.push_back({ (float)(tx * TILE), (float)(ty * TILE), 1 });
        }
    float playerX = spawnX, playerY = spawnY;

    SDL_Texture* floorTex  = IMG_LoadTexture(renderer, "assets/floor.png");
    SDL_Texture* wallTex   = IMG_LoadTexture(renderer, "assets/wall.png");
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "assets/player.png");
    SDL_Texture* guardTex  = IMG_LoadTexture(renderer, "assets/guard.png");
    if (!floorTex || !wallTex || !playerTex || !guardTex)
    {
        printf("Failed to load textures: %s\n", IMG_GetError());
        return 1;
    }

    auto hitsWall = [&](float x, float y)
    {
        int x0 = (int)x / TILE, x1 = (int)(x + PLAYER_W - 1) / TILE;
        int y0 = (int)y / TILE, y1 = (int)(y + PLAYER_H - 1) / TILE;

        if (x0 < 0 || y0 < 0 || x1 >= levelW || y1 >= levelH) 
            return true;

        for (int ty = y0; ty <= y1; ty++)
            for (int tx = x0; tx <= x1; tx++)
                if (level[ty][tx] == '#') return true;

        return false;
    };

    auto canSeePlayer = [&](const Guard& g)
    {
        const float guardEyeX = g.x + PLAYER_W / 2.0f;
        const float guardEyeY = g.y + 8.0f;
        const float playerEyeX = playerX + PLAYER_W / 2.0f;
        const float playerEyeY = playerY + 8.0f;

        const float deltaX = playerEyeX - guardEyeX;
        const float deltaY = playerEyeY - guardEyeY;

        if (deltaX * g.dir <= 0.0f ||
            std::fabs(deltaX) > 96.0f ||
            std::fabs(deltaY) > 10.0f)
        {
            return false;
        }

        const float distance = std::fabs(deltaX);
       

        for (float travelled = 4.0f; travelled < distance; travelled += 4.0f)
        {
            const float x = guardEyeX + g.dir * travelled;
            const float y = guardEyeY + deltaY * (travelled / distance);

            const int tx = static_cast<int>(x) / TILE;
            const int ty = static_cast<int>(y) / TILE;

            if (tx < 0 || tx >= levelW || ty < 0 || ty >= levelH ||
                level[ty][tx] == '#')
            {
                return false;
            }
        }

        return true;
    };

    const float playerSpeed = 100.0f;
    const float guardSpeed  = 60.0f;

    Uint64 prevTime = SDL_GetPerformanceCounter();
    bool running = true;
    bool facingLeft = false;

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - prevTime) / (float)SDL_GetPerformanceFrequency();
        prevTime = now;
        if (dt > 0.05f) dt = 0.05f;

        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT) running = false;
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        // Player movement
        float dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_LEFT])  
        { 
            dx = -playerSpeed * dt; 
            facingLeft = true; 
        }
        if (keys[SDL_SCANCODE_RIGHT])
        { 
            dx =  playerSpeed * dt; 
            facingLeft = false; 
        }
        if (keys[SDL_SCANCODE_UP])      
            dy = -playerSpeed * dt;
        if (keys[SDL_SCANCODE_DOWN])    
            dy =  playerSpeed * dt;
        if (!hitsWall(playerX + dx, playerY)) 
            playerX += dx;
        if (!hitsWall(playerX, playerY + dy)) 
            playerY += dy;

        // Guard
        for (auto& g : guards)
        {
            if (canSeePlayer(g))
            {
                g.alertTimer = 1.5f;
            }
            else if (g.alertTimer > 0.0f)
            {
                g.alertTimer -= dt;
            }

            if (g.alertTimer > 0.0f)
            {
                g.mode = GuardMode::Chase;

                if (playerX > g.x)
                    g.dir = 1;
                else if (playerX < g.x)
                    g.dir = -1;
            }
            else
            {
                g.alertTimer = 0.0f;
                g.mode = GuardMode::Patrol;
            }

            const float speed = (g.mode == GuardMode::Chase)
                ? 85.0f
                : guardSpeed;

            const float nextX = g.x + g.dir * speed * dt;

            if (hitsWall(nextX, g.y))
            {
                if (g.mode == GuardMode::Patrol)
                    g.dir = -g.dir;
            }
            else
            {
                g.x = nextX;
            }
        }

        // Collision detection
        SDL_Rect pr
        { 
            (int)playerX, (int)playerY, 
            PLAYER_W, PLAYER_H 
        };
        bool caught = false;
        for (auto& g : guards)
        {
            SDL_Rect gr
            { 
                (int)g.x, (int)g.y, 
                PLAYER_W, PLAYER_H 
            };

            if (SDL_HasIntersection(&pr, &gr)) 
            { 
                caught = true; 
                break; 
            }
        }
        if (caught)
        {
            printf("CATCH! RESPAWN.\n");
            playerX = spawnX;
            playerY = spawnY;

            for (auto& g : guards)
            {
                g.mode = GuardMode::Patrol;
                g.alertTimer = 0.0f;
            }
        }

        pr.x = (int)playerX;
        pr.y = (int)playerY;

        // RENDER
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int ty = 0; ty < levelH; ty++)
            for (int tx = 0; tx < levelW; tx++)
            {
                SDL_Rect dst{ tx * TILE, ty * TILE, TILE, TILE };
                SDL_RenderCopy(renderer,
                    level[ty][tx] == '#' ? wallTex : floorTex, nullptr, &dst);
            }

        for (auto& g : guards)
        {
            SDL_Rect gr{ (int)g.x, (int)g.y, PLAYER_W, PLAYER_H };
            SDL_RenderCopyEx(renderer, guardTex, nullptr, &gr, 0, nullptr,
                             g.dir < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            SDL_Rect indicator{ static_cast<int>(g.x) + 6,
                    static_cast<int>(g.y) - 5, 4, 4 };

            if (g.mode == GuardMode::Chase)
            {
                SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
                SDL_RenderFillRect(renderer, &indicator);
            }
        }

        SDL_RenderCopyEx(renderer, playerTex, nullptr, &pr, 0, nullptr,
                         facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(guardTex);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(wallTex);
    SDL_DestroyTexture(floorTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
    return 0;
}