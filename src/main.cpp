#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>

// Standart W and H (SEGA GENESIS)
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 224;
constexpr int SCALE = 3;

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

    SDL_Texture* playerTex = IMG_LoadTexture(renderer, "assets/player.png");
    if (!playerTex)
    {
        printf("Failed to load sprite: %s\n", IMG_GetError());
        return 1;
    }

    float playerX = 152.0f, playerY = 100.0f;
    constexpr int PLAYER_W = 16, PLAYER_H = 24;
    const float speed = 120.0f; // pixels per second
    bool facingLeft = false;

    Uint64 prevTime = SDL_GetPerformanceCounter();
    bool running = true;

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - prevTime) /
                   (float)SDL_GetPerformanceFrequency();
        prevTime = now;
        if (dt > 0.05f)
            dt = 0.05f;

        
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_LEFT])
        {
            playerX -= speed * dt;
            facingLeft = true;
        }
        if (keys[SDL_SCANCODE_RIGHT])
        {
            playerX += speed * dt;
            facingLeft = false;
        }
        if (keys[SDL_SCANCODE_UP])
            playerY -= speed * dt;
        if (keys[SDL_SCANCODE_DOWN])
            playerY += speed * dt;

        // Collision
        if (playerX < 0)
            playerX = 0;
        if (playerY < 0)
            playerY = 0;
        if (playerX > SCREEN_W - PLAYER_W)
            playerX = (float)(SCREEN_W - PLAYER_W);
        if (playerY > SCREEN_H - PLAYER_H)
            playerY = (float)(SCREEN_H - PLAYER_H);
        
        SDL_Rect dst {
            (int)playerX, (int)playerY,
            PLAYER_W, PLAYER_H
        };
        SDL_RendererFlip flip = facingLeft
            ? SDL_FLIP_HORIZONTAL
            : SDL_FLIP_NONE;

        // Background color and clear
        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopyEx(renderer, playerTex, nullptr, &dst, 0, nullptr, flip);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(playerTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}