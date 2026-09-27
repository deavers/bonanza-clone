#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

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

struct Treasure
{
    int x;
    int floor; // 0 - top, 1 - bottom
    bool taken = false;
};

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG)
    {
        std::fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

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

    char* basePath = SDL_GetBasePath();
    if (!basePath)
    {
        std::fprintf(stderr, "SDL_GetBasePath: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    std::string playerPath = std::string(basePath) + "assets/player.png";
    SDL_free(basePath);

    SDL_Texture* playerTexture =
        IMG_LoadTexture(renderer, playerPath.c_str());

    if (!playerTexture)
    {
        std::fprintf(stderr, "IMG_LoadTexture (%s): %s\n",
                    playerPath.c_str(), IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);

    float playerX = 40.0f;
    int currentFloor = 1; // 0 = top, 1 = bottom
    int targetFloor = currentFloor;
    float playerY = static_cast<float>(floorY(currentFloor) - PLAYER_H);
    bool climbing = false;
    bool facingLeft = false;

    std::vector<Treasure> treasures
    {
        { 260, 1, false }, // Lower floor
        { 40,  0, false }  // Top floor
    };

    constexpr int EXIT_X = 280;
    constexpr int EXIT_FLOOR = 0;
    bool won = false;

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
            
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_R &&
                won)
            {
                playerX = 40.0f;
                currentFloor = 1;
                targetFloor = 1;
                playerY = static_cast<float>(floorY(1) - PLAYER_H);
                climbing = false;
                facingLeft = false;
                won = false;

                for (auto& treasure : treasures)
                    treasure.taken = false;

                SDL_SetWindowTitle(window, "Side-view prototype");
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (!won)
        {
            if (!climbing)
            {
                if (keys[SDL_SCANCODE_LEFT])
                {
                    playerX -= 100.0f * dt;
                    facingLeft = true;
                }

                if (keys[SDL_SCANCODE_RIGHT])
                {
                    playerX += 100.0f * dt;
                    facingLeft = false;
                }

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

            if (!climbing)
            {
                SDL_Rect playerBox{
                    static_cast<int>(playerX),
                    static_cast<int>(playerY),
                    PLAYER_W,
                    PLAYER_H
                };

                for (auto& treasure : treasures)
                {
                    if (treasure.taken || treasure.floor != currentFloor)
                        continue;

                    SDL_Rect treasureBox{
                        treasure.x + 3,
                        floorY(treasure.floor) - 12,
                        10,
                        10
                    };

                    if (SDL_HasIntersection(&playerBox, &treasureBox))
                    {
                        treasure.taken = true;
                        std::printf("Treasure collected!\n");
                    }
                }

                bool allCollected = std::all_of(
                    treasures.begin(),
                    treasures.end(),
                    [](const Treasure& treasure)
                    {
                        return treasure.taken;
                    }
                );

                SDL_Rect exitBox{
                    EXIT_X,
                    floorY(EXIT_FLOOR) - PLAYER_H,
                    PLAYER_W,
                    PLAYER_H
                };

                if (allCollected &&
                    currentFloor == EXIT_FLOOR &&
                    SDL_HasIntersection(&playerBox, &exitBox))
                {
                    won = true;
                    SDL_SetWindowTitle(window, "Mission complete! Press R to restart");
                    std::printf("MISSION COMPLETE!\n");
                }
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

        bool allCollected = std::all_of(
            treasures.begin(),
            treasures.end(),
            [](const Treasure& treasure)
            {
                return treasure.taken;
            }
        );

        SDL_Rect exitBox{
            EXIT_X,
            floorY(EXIT_FLOOR) - PLAYER_H,
            PLAYER_W,
            PLAYER_H
        };

        if (allCollected)
            SDL_SetRenderDrawColor(renderer, 70, 210, 110, 255);
        else
            SDL_SetRenderDrawColor(renderer, 120, 120, 130, 255);

        SDL_RenderFillRect(renderer, &exitBox);

        SDL_SetRenderDrawColor(renderer, 255, 210, 60, 255);

        for (const auto& treasure : treasures)
        {
            if (treasure.taken)
                continue;

            SDL_Rect box{
                treasure.x + 3,
                floorY(treasure.floor) - 12,
                10,
                10
            };
            SDL_RenderFillRect(renderer, &box);
        }

        SDL_Rect player{
            static_cast<int>(playerX),
            static_cast<int>(playerY),
            PLAYER_W,
            PLAYER_H
        };

        SDL_RenderCopyEx(
            renderer,
            playerTexture,
            nullptr,
            &player,
            0,
            nullptr,
            facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE
        );

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}