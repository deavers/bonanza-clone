#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 224;
constexpr int SCALE = 3;

constexpr int PLAYER_W = 16;
constexpr int PLAYER_H = 22;

constexpr int TOP_FLOOR_Y = 96;
constexpr int BOTTOM_FLOOR_Y = 192;
constexpr int LADDER_X = 144;

int floorY(int floor)
{
    return floor == 0 ? TOP_FLOOR_Y : BOTTOM_FLOOR_Y;
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Side-view prototype",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W * SCALE,
        SCREEN_H * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer)
    {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    float playerX = 40.0f;
    int currentFloor = 1; // 0 = top, 1 = bottom
    int targetFloor = currentFloor;
    float playerY = static_cast<float>(floorY(currentFloor) - PLAYER_H);
    bool climbing = false;

    Uint64 previousTime = SDL_GetPerformanceCounter();
    bool running = true;

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - previousTime) /
                   static_cast<float>(SDL_GetPerformanceFrequency());
        previousTime = now;
        dt = std::min(dt, 0.05f);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                running = false;
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (!climbing)
        {
            if (keys[SDL_SCANCODE_LEFT])
                playerX -= 100.0f * dt;

            if (keys[SDL_SCANCODE_RIGHT])
                playerX += 100.0f * dt;

            playerX = std::clamp(
                playerX, 0.0f,
                static_cast<float>(SCREEN_W - PLAYER_W)
            );

            float playerCenterX = playerX + PLAYER_W / 2.0f;
            float ladderCenterX = LADDER_X + PLAYER_W / 2.0f;
            bool nearLadder = std::fabs(playerCenterX - ladderCenterX) <= 12.0f;

            if (nearLadder &&
                currentFloor == 1 &&
                keys[SDL_SCANCODE_UP])
            {
                playerX = static_cast<float>(LADDER_X);
                targetFloor = 0;
                climbing = true;
            }
            else if (nearLadder &&
                     currentFloor == 0 &&
                     keys[SDL_SCANCODE_DOWN])
            {
                playerX = static_cast<float>(LADDER_X);
                targetFloor = 1;
                climbing = true;
            }
        }

        if (climbing)
        {
            float destination =
                static_cast<float>(floorY(targetFloor) - PLAYER_H);
            float step = 70.0f * dt;

            if (playerY < destination)
                playerY = std::min(playerY + step, destination);
            else
                playerY = std::max(playerY - step, destination);

            if (playerY == destination)
            {
                currentFloor = targetFloor;
                climbing = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 22, 25, 42, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 135, 75, 60, 255);
        SDL_Rect topPlatform{ 0, TOP_FLOOR_Y, SCREEN_W, 8 };
        SDL_Rect bottomPlatform{ 0, BOTTOM_FLOOR_Y, SCREEN_W, 8 };
        SDL_RenderFillRect(renderer, &topPlatform);
        SDL_RenderFillRect(renderer, &bottomPlatform);

        SDL_SetRenderDrawColor(renderer, 220, 190, 90, 255);
        SDL_Rect ladder{
            LADDER_X + 6,
            TOP_FLOOR_Y,
            4,
            BOTTOM_FLOOR_Y - TOP_FLOOR_Y
        };
        SDL_RenderFillRect(renderer, &ladder);

        SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
        SDL_Rect player{
            static_cast<int>(playerX),
            static_cast<int>(playerY),
            PLAYER_W,
            PLAYER_H
        };
        SDL_RenderFillRect(renderer, &player);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}