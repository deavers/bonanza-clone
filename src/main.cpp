#include <SDL.h>
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

    SDL_Window* window = SDL_CreateWindow(
        "bonanza-clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * SCALE, SCREEN_H * SCALE,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // Rendering virtual window
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    // Player (16x24)
    SDL_Rect player {
        152, 100,
        16, 24
    };
    const int speed = 2; // pixels for frame

    bool running = true;
    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (keys[SDL_SCANCODE_LEFT])
            player.x -= speed;
        if (keys[SDL_SCANCODE_RIGHT])
            player.x += speed;
        if (keys[SDL_SCANCODE_UP])
            player.y -= speed;
        if (keys[SDL_SCANCODE_DOWN])
            player.y += speed;

        // Collision
        if (player.x < 0)
            player.x = 0;
        if (player.y < 0)
            player.y = 0;
        if (player.x > SCREEN_W - player.w)
            player.x = SCREEN_W - player.w;
        if (player.y > SCREEN_H - player.h)
            player.y = SCREEN_H - player.h;
        
        // Background
        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);

        // Brother
        SDL_SetRenderDrawColor(renderer, 250, 220, 60, 255);
        SDL_RenderFillRect(renderer, &player);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}