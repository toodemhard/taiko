#pragma once

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <elzip.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <optional>

std::optional<std::filesystem::path> find_music_file(std::filesystem::path mapset_directory);
int load_osz(std::filesystem::path osz_file_path);

enum NoteFlagBits : uint8_t {
    kat = 0,
    don = 1 << 0,
    big = 0,
    small = 1 << 1,
};

using NoteFlags = uint8_t;

struct MapSetInfo {
    std::string title;
    std::string artist;
    double preview_time;

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        ar(title, artist, preview_time);
    }
};

CEREAL_CLASS_VERSION(MapSetInfo, 0);

struct MapMeta {
    std::string difficulty_name;
    double bpm = 160;
    double offset = 0;

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        ar(difficulty_name, bpm, offset);
    }
};

CEREAL_CLASS_VERSION(MapMeta, 0);

inline void partial_load_map_metadata(MapMeta& object, std::filesystem::path path) {
    std::ifstream file(path, std::ios::binary);
    file.seekg(sizeof(uint32_t)); // skip the version number of Map
    cereal::BinaryInputArchive ar(file);
    ar(object);
}

struct Map {
    Map() = default;
    Map(MapMeta meta_data);

    MapMeta m_meta_data;
    
    std::vector<double> times{};
    std::vector<NoteFlags> flags_list{};

    template<class Archive>
    void serialize(Archive& ar, const uint32_t version) {
        ar(m_meta_data, times, flags_list);
    }
};

CEREAL_CLASS_VERSION(Map, 0);
