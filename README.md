# Taiko
Remake of [Taiko no Tatsujin](https://en.wikipedia.org/wiki/Taiko_no_Tatsujin) from scratch in C++.

## Building
I haven't added the assets to this repo yet so try releases instead.
```
git clone https://github.com/toodemhard/taiko.git
cd taiko
mkdir build
cmake -S . -B build
cmake --build build
```
Debug, release, and distribution presets are available in CMakePresets.json.

## Getting maps
This game supports loading osu!taiko maps so you can download them from [https://osu.ppy.sh/beatmapsets?m=1&s=any](https://osu.ppy.sh/beatmapsets?m=1&s=any).

## Gallery

*Random screenshot*

![vlcsnap-2024-10-12-12h19m22s154](https://github.com/user-attachments/assets/a7eade20-9aaa-4a67-aadf-7bd415fc93d1)

*Gameplay with sound*

https://github.com/user-attachments/assets/995f6ba1-1ac9-4c90-ab36-d23b9da9a9e1

*Profile with fps capped to 1000*

![profile](https://github.com/user-attachments/assets/6a16b558-2e0c-46f8-84a1-30cb514efd7a)

*I need to make a custom renderer ;-;*

![profile_0](https://github.com/user-attachments/assets/61dbcf07-4e5c-4cd0-a544-4d4706cc1d58)
