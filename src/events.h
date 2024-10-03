#pragma once

#include <variant>
#include <filesystem>
#include <optional>
#include <string>
#include <queue>

namespace Event {
    struct EditNewMap {};
    struct EditMap {
        std::filesystem::path map_directory;
        std::optional<std::string> map_difficulty;
    };
    struct TestMap {};
    struct QuitTest {};
    struct PlayMap {
        std::filesystem::path mapset_directory;
        std::string map_filename;
    };
    struct Return {};
    struct GameReset{};
}

using EventUnion = std::variant<
    Event::EditNewMap,
    Event::EditMap,
    Event::TestMap,
    Event::QuitTest,
    Event::PlayMap,
    Event::Return,
    Event::GameReset
>;

class EventQueue {
public:
    bool pop_event(EventUnion* event);
    void push_event(EventUnion event);
private:
    std::queue<EventUnion> events{};
};

namespace EventType {
    enum EventType {
        EditNewMap,
        EditMap,
        TestMap,
        QuitTest,
        PlayMap,
        Return,
        GameReset,
    };
}
