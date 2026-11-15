# CS:SO Android Source Code
Credits:
Ported by ndke, den4iklovelinux, cherrybtw
Mod author: PimonFeeD
Source engine port author: nillerusr
1.2 Updates: PowerSiderS
Shitty fork by: Star1xr

on this shitty fork

has useless full ci build and:

- [ ] dxvk native in its own branch (stopped the development. im tired of this shit. half complete, works but has bugs and is unplayable)

- [ ] CSM (idk, maybe...) (https://github.com/celisej567/Fake-CSM-Mapbase)

- [ ] Vphysics jolt into main branch

- [ ] Round buy wheel

- [ ] Crosshair follow recoil option (like CS2)

- [x] Each weapon icon in the HUD weapon list (bottom right inventory) now carries a small number on its top-right corner like CS:GO and CS:SO 2.0

- [x] Better CS:SO 2.0 chat

- [x] HUD safe area (notch margin) on HUD options in Mod Options (0-99)

- [x] New teamcounter style like csgo

- [x] Better headname like CS:GO from 1.3-update by @FJH03 from repo [CSSO-NOOFFICIAL-MP](https://github.com/FJH03/CSSO-NOOFFICIAL-MP/)

- [x] Team counter style option: Mod Options -> HUD -> choose between alive count (original), avatars new and avatars (`hud_teamcounter_style 0/1/2`).

- [x] Checkbox for headname on Mod Options -> HUD. 

- [x] CS:GO chat is always enabled, the `-nocustomchat` launch option was removed

- [x] Gyro page in Options next to Touch: enable, reverse left/right and up/down, sensitivity slider (`gyroscope`, `gyroscope_reverse_x`, `gyroscope_reverse_y`, `gyroscope_sensitivity`).

- [x] Key bindings (Keyboard page) are available in Options on Android too.

- [x] Game instructor from CS:GO ([Kisak-Strike](https://github.com/SwagSoftware/Kisak-Strike)) (on-screen hints like "Press B to open the Buy Menu"). Game files are in `docs/`.

CS:SO Apk Source Code https://github.com/ndke01/csso-android-launcher


# Source Engine
[![GitHub Actions Status](https://github.com/nillerusr/source-engine/actions/workflows/build.yml/badge.svg)](https://github.com/nillerusr/source-engine/actions/workflows/build.yml) [![GitHub Actions Status](https://github.com/nillerusr/source-engine/actions/workflows/tests.yml/badge.svg)](https://github.com/nillerusr/source-engine/actions/workflows/tests.yml)
 Discord: [![Discord Server](https://img.shields.io/discord/672055862608658432.svg)](https://discord.gg/hZRB7WMgGw)
 

Information from [wikipedia](https://wikipedia.org/wiki/Source_(game_engine)):

Source is a 3D game engine developed by Valve.
It debuted as the successor to GoldSrc with Half-Life: Source in June 2004,
followed by Counter-Strike: Source and Half-Life 2 later that year.
Source does not have a concise version numbering scheme; instead, it was released in incremental versions

Source code is based on TF2 2018 leak. Don't use it for commercial purposes.

This project is using waf buildsystem. If you have waf-related questions look https://waf.io/book

# Features:
- Android, OSX, FreeBSD, Windows, Linux( glibc, musl ) support
- Arm support( except windows )
- 64bit support
- Modern toolchains support
- Fixed many undefined behaviours
- Touch support( even on windows/linux/osx )
- VTF 7.5 support
- PBR support
- bsp v19-v21 support( bsp v21 support is partial, portal 2 and csgo maps works fine )
- mdl v46-49 support
- Removed useless/unnecessary dependencies
- Achivement system working without steam
- Fixed many bugs
- Serverbrowser works without steam
# CS:SO
- Bink video
- Lunasvg
- Ability to add your custom avatar & clantag. Added also UI implements for it
- Added Gyroscope (Dogshit Edition)
- Fixed Name & Avatar not displayed in Achievement Menu
- Now achievement menu save your stats , stuffs
- Decreased the issues of missing sounds in gameplay (use snd_restart if sounds began to disappear)
- Re-Worked on scoreboard (Avatars display and clantag)
- Fixed custom avatar not displayed on voice icon
- small optimizations lol
- CS2 Animation (Grenades Only) (Broken & Removed
- Skip HTML Panel to Choose Team Directly
- Agent Show Blind Animation
- Spam Inspection Animation
- Backgrounds changing Fixed (missed libs TF)
- HeadName like CS:GO (Re-worked on vibecode) [ `cl_headname "1/0"` ]
- New Teamcounter that's display avatars (Simple Script)
- Radio Menu (i didn't commit yet here)
- Vignette Script (or fade with ability to adjust & enable/disable) write in console `csgo_` and commands will appear
- `touch_disable_on_buymenu` Cvar (Already exist. lol)
- GameUI translated to chinese by Qwen :v
- CS:GO Chat (From CS:SO 2.0)
- New Touch UI [ `exec touch2.cfg` ] its contain radio button btw
- Fixed Name & Clantag not changed when client press **Apply** Button
- M4A1-S Amoo changed from 25 to 20 to match CS:GO
- i forgot ¯⁠\⁠_⁠(⁠ ͡⁠°⁠ ͜⁠ʖ⁠ ͡⁠°⁠)⁠_⁠/⁠¯

# Bugs
- Menu does't show avatars (IDK why) sometimes show and sometimes no
- you need `jpg` and ratio 1:1. put avatar in `csso/materials/vgui/logos` otherwise, it will show you error message

# Current tasks
- Rewrite materialsystem for OpenGL render
- dxvk-native support
- Elbrus port
- Bink audio support( for video_bink )

# How to Build?
- [Building instructions(EN)](https://github.com/nillerusr/source-engine/wiki/Source-Engine-(EN))
- [Building instructions(RU)](https://github.com/nillerusr/source-engine/wiki/Source-Engine-(RU))

# Support me
BTC: `bc1qnjq92jj9uqjtafcx2zvnwd48q89hgtd6w8a6na`

ETH: `0x5d0D561146Ed758D266E59B56e85Af0b03ABAF46`

XMR: `48iXvX61MU24m5VGc77rXQYKmoww3dZh6hn7mEwDaLVTfGhyBKq2teoPpeBq6xvqj4itsGh6EzNTzBty6ZDDevApCFNpsJ`
