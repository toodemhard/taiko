# Taiko
Remake of [Taiko no Tatsujin](https://en.wikipedia.org/wiki/Taiko_no_Tatsujin) from scratch in C++.

https://github.com/user-attachments/assets/30f94557-f5a9-431c-ade0-2e7a6dc55f4a

*It's a rythm game so unmute the vid.*

![image](https://github.com/user-attachments/assets/e34aaedb-eb2d-4dd4-9e6f-02ee6b05b115)

## Building
```
git clone https://github.com/toodemhard/taiko.git
cd taiko
git submodule update --init --recursive
mkdir build
cmake -S . -B build
cmake --build build
```
Debug, release, and distribution presets are available in CMakePresets.json.

## Getting maps
This game supports loading osu!taiko maps so you can download them from [https://osu.ppy.sh/beatmapsets?m=1&s=any](https://osu.ppy.sh/beatmapsets?m=1&s=any).
