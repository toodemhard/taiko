#include "map.h"

Map::Map(MapMeta meta_data) : m_meta_data{ meta_data } {}

void Map::insert_note(double time, NoteFlags flags) {
    int i = std::upper_bound(times.begin(), times.end(), time) - times.begin();
    times.insert(times.begin() + i, time);
    flags_vec.insert(flags_vec.begin() + i, flags);
    selected.insert(selected.begin() + i, false);
}

void Map::remove_note(int i) {
    times.erase(times.begin() + i);
    flags_vec.erase(flags_vec.begin() + i);
    selected.erase(selected.begin() + i);
}
