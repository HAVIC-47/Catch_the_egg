# README

This project is a complete playable **Catch The Eggs** implementation in C++ using OpenGL (FreeGLUT).

## Features Implemented
- Catch The Eggs core mechanics (basket catches falling eggs from chickens on sticks)
- Basket control with keyboard and mouse
- 6 levels with increasing difficulty:
  - Lost Tourist
  - Casual Farmer
  - Skilled Harvester
  - Sleep-Deprived Genius
  - Mad Farmer
  - Ruthless Farmer (blood moon)
- Per-level timer + score + combo HUD
- Wind system that pushes eggs sideways (with on-screen wind indicator)
- Egg variants:
  - Normal Egg (+1)
  - Blue Egg (+5)
  - Gold Egg (+10)
- Hazards (avoid!):
  - Poop (-10)
  - Bomb (-25)
- Power-ups (catch these):
  - Large Basket (10s)
  - Slow Time (10s)
  - Extra Time (+10s)
  - Double Points (10s)
  - Magnet pulls eggs (8s)
  - Shield blocks 1 bad hit (stacks up to x2)
- Power-downs (avoid):
  - Small Basket (5s)
  - Reverse Controls (5s)
  - Freeze Basket (3s)
- Combo system: 5 catches in a row multiplies your score
- Menu page:
  - Play
  - Level Select
  - Help
  - Quit
- Pause/resume and exit support during gameplay
- Game Over screen with retry
- Per-level high score storage (persistent)
- Animated chickens, clouds, particle effects, screen shake/flash, BGM (`catcheggs_bgm.wav`)

## Controls
- Move basket: Left/Right or A/D
- Move basket (mouse): move mouse horizontally
- Pause/resume: P
- Back to menu from gameplay: Esc (or M from Help/Game Over)
- Retry (after game over): R
- Exit from anywhere: Q
- Menu navigation: mouse click, or keyboard (1/2/3/4, H for Help)

## Build (Windows, MinGW + FreeGLUT)
If your setup already has FreeGLUT and OpenGL libs available:

```
g++ "Catch The Eggs Game Compact.cpp" -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o "Catch The Eggs Game Compact.exe"
"./Catch The Eggs Game Compact.exe"
```

The full (non-compact) version builds the same way:

```
g++ "Catch The Eggs Game.cpp" -std=c++17 -lfreeglut -lopengl32 -lglu32 -lwinmm -o "Catch The Eggs Game.exe"
"./Catch The Eggs Game.exe"
```

## Files
- `Catch The Eggs Game.cpp` — full source
- `Catch The Eggs Game Compact.cpp` — compact edition (<=1500 lines)
- `Catch The Eggs part 1..8.cpp` — staged build-up versions
- `catcheggs_bgm.wav` — background music
- `Catch The Eggs.pdf` — design / report doc
