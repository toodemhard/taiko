#include "map.h"

Map::Map(MapMeta meta_data) : m_meta_data{ meta_data } {}


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
