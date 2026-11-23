# Squad Rush — fx-CG50 add-in

A port of the HTML game (`../index.html`) to the Casio fx-CG50, built with the
PrizmSDK (libfxcg). The finished add-in is **`SquadRush.g3a`**.

## Install on the calculator

1. Connect the calculator to the PC with its USB cable.
2. On the calculator, choose **USB Flash** (press F1) when the connection menu appears.
3. Copy `SquadRush.g3a` to the root of the calculator's drive.
4. Eject the drive in Windows, then unplug.
5. Press **MENU** and pick the **SquadRush** icon.

## Controls

| Key | Action |
|---|---|
| LEFT / RIGHT | Move the squad (on the title screen: change difficulty) |
| UP / DOWN | Title screen: choose CONTINUE or NEW GAME |
| EXE | Start / retry / resume |
| EXIT, OPTN or F1 | Pause (from pause: EXE resumes, EXIT saves and goes to the title) |
| MENU | Back to the calculator's main menu (works from every screen) |

The run is saved to main memory whenever a menu or the pause screen opens, so after
visiting the calculator's main menu you can pick **CONTINUE** on the title screen.

Best distances (per difficulty) are kept in main memory under `@SQRUSH`.

## Build

Needs the PrizmSDK at `C:\Users\salvador\PrizmSDK\PrizmSDK-win-0.6`
(or set `FXCGSDK` to wherever it lives), then from this folder:

```
make.bat
```

`make.bat clean` removes the build output. `src/main.c` is the whole game;
`icon-uns.bmp` / `icon-sel.bmp` are the 92x64 menu icons.

## Differences from the HTML version

The CG50 has no floating-point unit available to add-ins and a much slower
CPU, so the port draws like the HTML "CG50 view": solid shapes only, a 3x5
pixel font, no fog or transparency, and at most 30 soldiers drawn per group
(the real counts are unlimited, just like the browser version). The screen is
384x216, the area the OS gives add-ins.

## Gameplay notes

Gate pairs are close calls (the two results are similar, so you have to do the maths) and you
must commit to one side. Enemy groups shrink visibly as you shoot them. The boss strafes and
marks red danger zones on the road: move out of them before they land, and line up to shoot him.

`SquadRush.g3a` has to be rebuilt with the PrizmSDK (`make.bat`) to pick up changes to `src/main.c`.
