#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "sideview/LevelModel.h"
#include <fstream>
#include <sstream>

bool loadTreasures(const std::string& path, std::vector<Treasure>& result)
{
    std::ifstream file(path);

    if (!file)
        return false;

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::istringstream row(line);
        int x = 0;
        int floor = 0;
        std::string extra;

        if (!(row >> x >> floor) || (row >> extra))
            return false;

        if (x < 0 || x > SCREEN_W - PLAYER_W ||
            (floor != 0 && floor != 1))
        {
            return false;
        }

        result.push_back({ x, floor, false });
    }

    return !result.empty() && !file.bad();
}

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

    std::string baseDirectory(basePath);
    SDL_free(basePath);

    std::string assetsPath = baseDirectory + "assets/";
    std::string levelPath = baseDirectory + "levels/sideview01.txt";

    std::string playerPath = assetsPath + "player.png";
    std::string guardPath = assetsPath + "guard.png";

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

    SDL_Texture* guardTexture =
    IMG_LoadTexture(renderer, guardPath.c_str());

    if (!guardTexture)
    {
        std::fprintf(stderr, "IMG_LoadTexture (%s): %s\n",
                    guardPath.c_str(), IMG_GetError());
        SDL_DestroyTexture(playerTexture);
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
    bool inBackLane = false;

    std::vector<Guard> guards{
        { 200.0f, 1,  1, 170.0f, 280.0f },
        { 200.0f, 0, -1, 160.0f, 260.0f }
    };

    const std::vector<Guard> initialGuards = guards;

    std::vector<Treasure> treasures;

    if (!loadTreasures(levelPath, treasures))
    {
        std::fprintf(stderr, "Cannot load level: %s\n", levelPath.c_str());

        SDL_DestroyTexture(guardTexture);
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    constexpr int EXIT_X = 280;
    constexpr int EXIT_FLOOR = 0;
    bool won = false;

    constexpr float ROUND_SECONDS = 90.0f;
    float timeLeft = ROUND_SECONDS;
    bool timeUp = false;
    int lastShownSecond = -1;

    Uint64 previousTime = SDL_GetPerformanceCounter();
    bool running = true;

    while (running)
    {
        Uint64 now = SDL_GetPerformanceCounter();

        float elapsed = static_cast<float>(now - previousTime) /
                        static_cast<float>(SDL_GetPerformanceFrequency());

        previousTime = now;
        float dt = std::min(elapsed, 0.05f);

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
                (won || timeUp))
            {
                playerX = 40.0f;
                currentFloor = 1;
                targetFloor = 1;
                playerY = static_cast<float>(floorY(1) - PLAYER_H);
                climbing = false;
                facingLeft = false;
                inBackLane = false;
                guards = initialGuards;
                won = false;
                timeUp = false;
                timeLeft = ROUND_SECONDS;
                lastShownSecond = -1;

                for (auto& treasure : treasures)
                    treasure.taken = false;

                SDL_SetWindowTitle(window, "Side-view prototype");
            }

            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_SPACE &&
                event.key.repeat == 0 &&
                !won &&
                !timeUp &&
                !climbing &&
                !inBackLane)
            {
                float playerCenter = playerX + PLAYER_W / 2.0f;
                int shotDirection = facingLeft ? -1 : 1;

                for (auto& guard : guards)
                {
                    if (guard.floor != currentFloor || guard.stunTimer > 0.0f)
                        continue;

                    float guardCenter = guard.x + PLAYER_W / 2.0f;
                    float distance = (guardCenter - playerCenter) * shotDirection;

                    if (distance > 0.0f && distance <= 70.0f)
                    {
                        guard.stunTimer = 2.5f;
                        std::printf("Guard stunned!\n");
                        break;
                    }
                }
            }

            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.scancode == SDL_SCANCODE_E &&
                event.key.repeat == 0 &&
                !won &&
                !timeUp &&
                !climbing)
            {
                float playerCenter = playerX + PLAYER_W / 2.0f;

                bool nearLeftDoor =
                    std::fabs(playerCenter - (DOOR_LEFT_X + PLAYER_W / 2.0f)) <= 12.0f;

                bool nearRightDoor =
                    std::fabs(playerCenter - (DOOR_RIGHT_X + PLAYER_W / 2.0f)) <= 12.0f;

                if (nearLeftDoor || nearRightDoor)
                {
                    inBackLane = !inBackLane;
                    std::printf("Lane: %s\n", inBackLane ? "back" : "front");
                }
            }
        }

        if (!won && !timeUp)
        {
            timeLeft = std::max(0.0f, timeLeft - elapsed);

            if (timeLeft <= 0.0f)
            {
                timeUp = true;
                SDL_SetWindowTitle(window, "TIME UP! Press R to restart");
                std::printf("TIME UP!\n");
            }
            else
            {
                int shownSecond = static_cast<int>(std::ceil(timeLeft));

                if (shownSecond != lastShownSecond)
                {
                    lastShownSecond = shownSecond;

                    char title[96];
                    std::snprintf(title, sizeof(title),
                                "Side-view prototype | Time: %d",
                                shownSecond);
                    SDL_SetWindowTitle(window, title);
                }
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        if (!won && !timeUp)
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

                if (nearLadder && !inBackLane &&
                    currentFloor == 1 &&
                    keys[SDL_SCANCODE_UP])
                {
                    playerX = static_cast<float>(LADDER_X);
                    targetFloor = 0;
                    climbing = true;
                }
                else if (nearLadder && !inBackLane &&
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
                    inBackLane = false;
                }
            }
 
            for (auto& guard : guards)
            {
                if (guard.stunTimer > 0.0f)
                {
                    guard.stunTimer = std::max(0.0f, guard.stunTimer - dt);
                    guard.alertTimer = 0.0f;
                    continue;
                }

                float distanceX = playerX - guard.x;

                bool seesPlayer =
                    !climbing &&
                    !inBackLane &&
                    currentFloor == guard.floor &&
                    distanceX * guard.dir > 0.0f &&
                    std::fabs(distanceX) <= 80.0f;

                if (seesPlayer)
                {
                    guard.alertTimer = 1.2f;
                }
                else
                {
                    guard.alertTimer = std::max(0.0f, guard.alertTimer - dt);
                }

                float guardSpeed = guard.alertTimer > 0.0f ? 80.0f : 55.0f;
                guard.x += guard.dir * guardSpeed * dt;

                if (guard.x >= guard.maxX)
                {
                    guard.x = guard.maxX;
                    guard.dir = -1;
                }
                else if (guard.x <= guard.minX)
                {
                    guard.x = guard.minX;
                    guard.dir = 1;
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

                bool caught = false;

                for (const auto& guard : guards)
                {
                    if (inBackLane ||
                        guard.floor != currentFloor ||
                        guard.stunTimer > 0.0f)
                    {
                        continue;
                    }

                    SDL_Rect guardBox{
                        static_cast<int>(guard.x),
                        floorY(guard.floor) - PLAYER_H,
                        PLAYER_W,
                        PLAYER_H
                    };

                    if (SDL_HasIntersection(&playerBox, &guardBox))
                    {
                        caught = true;
                        break;
                    }
                }

                if (caught)
                {
                    std::printf("CAUGHT! Back to start.\n");

                    playerX = 40.0f;
                    currentFloor = 1;
                    targetFloor = 1;
                    playerY = static_cast<float>(floorY(1) - PLAYER_H);
                    climbing = false;
                    inBackLane = false;
                    guards = initialGuards;

                    continue; // Skip the rest of the loop to avoid processing further after being caught
                }

                for (auto& treasure : treasures)
                {
                    if (inBackLane ||
                        treasure.taken ||
                        treasure.floor != currentFloor)
                    {
                        continue;
                    }

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

                if (!inBackLane &&
                    allCollected &&
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

        SDL_SetRenderDrawColor(renderer, 95, 140, 170, 255);

        const int doorXs[] = { DOOR_LEFT_X, DOOR_RIGHT_X };

        for (int floor = 0; floor < 2; ++floor)
        {
            for (int doorX : doorXs)
            {
                SDL_Rect door{
                    doorX,
                    floorY(floor) - PLAYER_H,
                    PLAYER_W,
                    PLAYER_H
                };

                SDL_RenderDrawRect(renderer, &door);
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

        SDL_SetRenderDrawColor(renderer, 70, 110, 220, 255);

        for (const auto& guard : guards)
        {
            SDL_Rect guardBox{
                static_cast<int>(guard.x),
                floorY(guard.floor) - PLAYER_H,
                PLAYER_W,
                PLAYER_H
            };

            if (guard.stunTimer > 0.0f)
                SDL_SetTextureColorMod(guardTexture, 110, 110, 110);
            else
                SDL_SetTextureColorMod(guardTexture, 255, 255, 255);

            SDL_RenderCopyEx(
                renderer,
                guardTexture,
                nullptr,
                &guardBox,
                0,
                nullptr,
                guard.dir < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE
            );

            if (guard.alertTimer > 0.0f && guard.stunTimer <= 0.0f)
            {
                SDL_SetRenderDrawColor(renderer, 255, 65, 65, 255);

                SDL_Rect alertMark{
                    static_cast<int>(guard.x) + 6,
                    floorY(guard.floor) - PLAYER_H - 6,
                    4,
                    4
                };

                SDL_RenderFillRect(renderer, &alertMark);
            }
        }

        SDL_Rect player{
            static_cast<int>(playerX),
            static_cast<int>(playerY),
            PLAYER_W,
            PLAYER_H
        };

        if (inBackLane)
            SDL_SetTextureColorMod(playerTexture, 135, 135, 135);
        else
            SDL_SetTextureColorMod(playerTexture, 255, 255, 255);

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

    SDL_DestroyTexture(guardTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}