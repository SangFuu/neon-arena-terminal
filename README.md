# Neon Arena Terminal

I created this personal project to play in the Windows terminal. It is a solo C arena game controlled with the keyboard.

The game has four waves, contact enemies, shooters and a boss. It lets me explore a game loop, collisions and keyboard interactions.

## Play

Open `Jouer.cmd` or `jouer/neon_arena.exe`, then press Enter. Use a terminal at least 75 columns wide and 36 lines high.

- ZQSD / WASD: movement.
- Arrow keys: aim and shoot.
- Space: automatic fire.
- F or Shift: dodge.
- E: special attack.
- P / Escape: pause.
- P / Enter while paused: resume.
- R: restart.
- X: quit.

During a pause or at the end of a game, Escape returns to the menu. The game does not require any additional DLL.

## Build

With Windows, CMake and a C compiler:

```text
cmake --preset Debug
cmake --build --preset Debug
ctest --preset Debug
```

In CLion, use 64-bit MinGW and the `Neon Arena Terminal` configuration with the external console. The standard Run console does not support real-time keyboard input.

## Tests

Tests cover combat, progression, controls and Windows console output. The progression test uses a test-only safeguard that is absent from the normal game.
