#pragma once

inline constexpr int SCREEN_W = 320;
inline constexpr int SCREEN_H = 224;
inline constexpr int SCALE = 3;

inline constexpr int PLAYER_W = 16;
inline constexpr int PLAYER_H = 22;

inline constexpr int TOP_FLOOR_Y = 96;
inline constexpr int BOTTOM_FLOOR_Y = 192;
inline constexpr int LADDER_X = 144;

inline constexpr int DOOR_LEFT_X = 96;
inline constexpr int DOOR_RIGHT_X = 224;

constexpr int floorY(int floor)
{
    return floor == 0 ? TOP_FLOOR_Y : BOTTOM_FLOOR_Y;
}

struct Treasure
{
    int x;
    int floor;
    bool taken = false;
};

struct Guard
{
    float x;
    int floor;
    int dir;
    float minX;
    float maxX;
    float stunTimer = 0.0f;
};