#pragma once

#include <string>
#include <vector>

enum NoteFlagBits : uint8_t {
    don_or_kat = 1 << 0,
    normal_or_big = 1 << 1,
};

using NoteFlags = uint8_t;

struct MapSetInfo {
    std::string title;
    std::string artist;

    template<class Archive>
    void save(Archive& ar) const {
        ar(title, artist);
    }

    template<class Archive>
    void load(Archive& ar) {
        ar(title, artist);
    }
};

struct MapMeta {
    std::string difficulty_name;

    double bpm = 160;
    double offset = 0;

    template<class Archive>
    void save(Archive& ar) const {
        ar(difficulty_name, bpm, offset);
    }

    template<class Archive>
    void load(Archive& ar) {
        ar(difficulty_name, bpm, offset);
    }
};

class Map {
public:
    Map() = default;
    Map(MapMeta meta_data);

    MapMeta m_meta_data;
    
    std::vector<double> times{};
    std::vector<NoteFlags> flags_list{};
    std::vector<bool> selected{};

    void insert_note(double time, NoteFlags flags);
    void remove_note(int i);

    template<class Archive>
    void save(Archive& ar) const {
        ar(m_meta_data, times, flags_list);
    }

    template<class Archive>
    void load(Archive& ar) {
        ar(m_meta_data, times, flags_list);

        selected = std::vector<bool>(times.size(), false);
    }
};
