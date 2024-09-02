#include "map.h"

Map::Map(MapMeta meta_data) : m_meta_data{ meta_data } {}

void Map::insert_note(double time, NoteFlags flags) {
    int i = std::upper_bound(times.begin(), times.end(), time) - times.begin();
    times.insert(times.begin() + i, time);
    flags_list.insert(flags_list.begin() + i, flags);
    selected.insert(selected.begin() + i, false);
}

void Map::remove_note(int i) {
    times.erase(times.begin() + i);
    flags_list.erase(flags_list.begin() + i);
    selected.erase(selected.begin() + i);
}

bool match_extension(std::filesystem::path file_path, const char* extension) {
    return file_path.extension().string().compare(extension) == 0;
}

std::optional<std::filesystem::path> find_music_file(std::filesystem::path mapset_directory) {
    for (const auto& entry : std::filesystem::directory_iterator(mapset_directory)) {
        if (match_extension(entry.path(), ".mp3") || match_extension(entry.path(), ".ogg")) {
            return entry.path();
        }
    }

    return {};
}
