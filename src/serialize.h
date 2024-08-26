#pragma once

#include <filesystem>
#include <fstream>

#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>


template<typename T>
inline void save_binary(const T& object, std::filesystem::path path) {
    std::ofstream file(path, std::ios::binary);
    cereal::BinaryOutputArchive ar(file);
    ar(object);
}

template<typename T>
inline void load_binary(T& object, std::filesystem::path path) {
    std::ifstream file(path, std::ios::binary);
    cereal::BinaryInputArchive ar(file);
    ar(object);
}