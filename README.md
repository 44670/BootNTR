# BootNTRSelector
A mod of BootNTR which will allows you to choose the version of NTR you want to load (and it is also much faster than regular BootNTR).

# Latest stable builds
The latest stable builds can be found in the [releases section](https://github.com/Nanquitas/BootNTR/releases).

# Installation
### CIA
Install the .cia file through [FBI](https://github.com/Steveice10/FBI) and run it.

### 3DSX
Copy the .3dsx file into `sdmc:/3ds/BootNTRSelector/` and run it through the [New Homebrew Menu](https://github.com/fincs/new-hbmenu).

# How to use
- Launch BootNTRSelector through your method of choice and follow the on-screen instructions, no extra steps or files are required.

- BootNTRSelector will now default to whichever NTR version you selected last.

# Notes for Old3DS users
- You need to use one of the **Mode3** releases in order to use NTR with extended memory games (such as Monster Hunter, Pokémon Sun/Moon, Smash Bros, etc.). Mode3 has no effect on New3DS.

- Launching BootNTRSelector before the home menu is completely loaded may result in a crash / error. So before launching it, wait a couple of seconds for the home menu to be fully loaded.

# Screenshots
![Main Menu](https://imgur.com/EWuJOLV.png)

![Failed to load](https://i.imgur.com/8LYUJXN.png)

![Auto updater](https://i.imgur.com/7a3Wjzw.png)

# Building
Building BootNTRSelector is handled through buildtools. You have to have the following installed:
- [devkitPro with devkitARM](https://sourceforge.net/projects/devkitpro/files/Automated%20Installer/)
- ctrulib (installed automatically with the devkitARM script)
- [citro3d](https://github.com/fincs/citro3d)
- [portlibs](https://github.com/devkitPro/3ds_portlibs)

Once you have installed all the dependencies simply run `make` in the root directory and if you set it all up correctly it should build.
