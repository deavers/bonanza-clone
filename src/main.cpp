#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// Standart W and H (SEGA GENESIS)
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 224;
constexpr int SCALE = 3;
constexpr int TILE = 16;

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

    float playerX = 32.0f, playerY = 32.0f;
    for (int ty = 0; ty < levelH; ty++)
        for (int tx = 0; tx < levelW; tx++)
            if (level[ty][tx] == 'P')
            {
                playerX = (float)(tx * TILE);
                playerY = (float)(ty * TILE);
            }


    SDL_Texture* floorTex  = IMG_LoadTexture(renderer, "assets/floor.png");
    SDL_Texture* wallTex   = IMG_LoadTexture(renderer, "assets/wall.png");
    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "assets/player.png");
    if (!floorTex || !wallTex || !playerTex)
    {
        printf("Failed to load textures: %s\n", IMG_GetError());
        return 1;
    }

    // Collision detection
    auto hitsWall = [&](float x, float y, int w, int h) 
    {
        int x0 = (int)x / TILE,        x1 = (int)(x + w - 1) / TILE;
        int y0 = (int)y / TILE,        y1 = (int)(y + h - 1) / TILE;

        if (x0 < 0 || y0 < 0 || x1 >= levelW || y1 >= levelH) 
            return true;

        for (int ty = y0; ty <= y1; ty++)
            for (int tx = x0; tx <= x1; tx++)
                if (level[ty][tx] == '#') 
                    return true;

        return false;
    };

    constexpr int PLAYER_W = 16, PLAYER_H = 22;
    const float speed = 100.0f;

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

        float dx = 0, dy = 0;
        if (keys[SDL_SCANCODE_LEFT])  { 
            dx = -speed * dt; facingLeft = true; 
        }
        if (keys[SDL_SCANCODE_RIGHT]) { 
            dx =  speed * dt; facingLeft = false; 
        }
        
        if (keys[SDL_SCANCODE_UP])      
            dy = -speed * dt;
        if (keys[SDL_SCANCODE_DOWN])    
            dy =  speed * dt;

        if (!hitsWall(playerX + dx, playerY, PLAYER_W, PLAYER_H)) 
            playerX += dx;
        if (!hitsWall(playerX, playerY + dy, PLAYER_W, PLAYER_H)) 
            playerY += dy;

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int ty = 0; ty < levelH; ty++)
            for (int tx = 0; tx < levelW; tx++)
            {
                SDL_Rect dst{ tx * TILE, ty * TILE, TILE, TILE };
                bool solid = level[ty][tx] == '#';
                SDL_RenderCopy(renderer, solid ? wallTex : floorTex, nullptr, &dst);
            }

        SDL_Rect pr{ (int)playerX, (int)playerY, PLAYER_W, PLAYER_H };
        SDL_RenderCopyEx(renderer, playerTex, nullptr, &pr, 0, nullptr,
                         facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(wallTex);
    SDL_DestroyTexture(floorTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();

    return 0;
}