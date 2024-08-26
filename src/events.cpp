#include "events.h"

void EventQueue::push_event(EventUnion event) {
    events.push(event);
}

bool EventQueue::pop_event(EventUnion* event) {
    if (events.empty()) {
        return false;
    }

    if (event != nullptr) {
        *event = events.front();
        events.pop();
    }

    return true;
}
