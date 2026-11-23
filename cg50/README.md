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
| EXE | Start / retry / resume |
| EXIT, OPTN or F1 | Pause (from pause: EXE resumes, EXIT quits to title) |
| MENU | Back to the calculator's main menu |

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
