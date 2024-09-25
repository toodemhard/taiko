#include "map.h"
#include <filesystem>

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

#include "constants.h"
#include "serialize.h"
using namespace constants;

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

std::vector<std::string> split_once(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    getline(ss, item, delim);
    result.push_back(item);
    getline(ss, item);
    result.push_back(item);

    return result;
}

std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

Map load_osu_map(std::filesystem::path map_file_path);

int load_osz(std::filesystem::path osz_file_path) {
    if (osz_file_path.extension() != ".osz") {
        return 1;
    }

    auto extracted_dir = std::filesystem::path("temp");
    std::filesystem::remove_all(extracted_dir);
    std::filesystem::create_directory(extracted_dir);

    elz::extractZip(osz_file_path, extracted_dir);


    std::filesystem::path mapset_directory;
    for (const auto& entry : std::filesystem::directory_iterator(extracted_dir)) {
        if (entry.path().extension().string().compare(osu_file_extension) == 0) {
            std::ifstream file(entry.path());

            MapSetInfo mapset_info{};
            std::string audio_filename;

            std::string line;

            while (std::getline(file, line) && line.compare("[General]") != 0) {
            }
            while (std::getline(file, line)) {
                auto values = split_once(line, ':');
                if ( values[0].compare("AudioFilename") == 0) {
                    audio_filename = std::move(ltrim(values[1]));
                    break;
                }
            }
            while (std::getline(file, line)) {
                auto values = split_once(line, ':');
                if ( values[0].compare("PreviewTime") == 0) {
                    mapset_info.preview_time = std::stoi(values[1]) / 1000.0;
                    break;
                }
            }

            while (std::getline(file, line) && line.compare("[Metadata]") != 0) {
            }
            while (std::getline(file, line)) {
                auto values = split_once(line, ':');
                if ( values[0].compare("Title") == 0) {
                    mapset_info.title = std::move(values[1]);
                    break;
                }
            }
            while (std::getline(file, line)) {
                auto values = split_once(line, ':');
                if ( values[0].compare("Artist") == 0) {
                    mapset_info.artist = std::move(values[1]);
                    break;
                }
            }

            std::string dir_string = "data/maps/" + std::format("{} - {}", mapset_info.artist, mapset_info.title);
            auto banned_chars = [](char c) {
                switch (c) {
                case '.':
                case '*':
                case ':':
                    return true;
                default:
                    return false;
                }
            };
            dir_string.erase(std::remove_if(dir_string.begin(), dir_string.end(), banned_chars), dir_string.end());
            mapset_directory = std::filesystem::path(dir_string);

            std::filesystem::create_directories(mapset_directory);
            std::filesystem::copy_file(extracted_dir / audio_filename, mapset_directory / audio_filename, std::filesystem::copy_options::overwrite_existing);
            save_binary(mapset_info, (mapset_directory / mapset_filename));

            for (const auto& entry : std::filesystem::directory_iterator(extracted_dir)) {
                if (entry.path().extension().string().compare(osu_file_extension) == 0) {
                    auto map = load_osu_map(entry.path());
                    save_binary(map, mapset_directory / (map.m_meta_data.difficulty_name + map_file_extension));
                }
            }

            break;
        }
    }

    std::filesystem::remove_all(extracted_dir);

    return 0;
}

Map load_osu_map(std::filesystem::path map_file_path) {
    Map map{};

    std::ifstream file(map_file_path);

    std::string line;
    while (std::getline(file, line) && line.compare("[Metadata]") != 0) {
    }
    while (std::getline(file, line)) {
        auto values = split_once(line, ':');
        if ( values[0].compare("Version") == 0) {
            map.m_meta_data.difficulty_name = std::move(values[1]);
            break;
        }
    }


    while (std::getline(file, line) && line.compare("[HitObjects]") != 0) {
    }
    while (std::getline(file, line) && line.size() > 0) {
        auto values = split(line, ',');
        map.times.push_back(std::stoi(values[2]) / 1000.0);

        int note_type = std::stoi(values[4]);

        NoteFlags note_flags{};

        note_flags |= ((note_type >> 2) & 1) ? 0 : NoteFlagBits::small;
        note_flags |= ((note_type >> 3) & 1 || (note_type >> 1) & 1) ? 0 : NoteFlagBits::don;

        map.flags_list.push_back(note_flags);
    }

    return map;
}
