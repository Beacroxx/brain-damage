#pragma once

#include "event.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Event registry to hold all events
class EventRegistry {
public:
  static EventRegistry& instance() {
    static EventRegistry registry;
    return registry;
  }
  
  // Register an event factory function
  void register_event(const std::string& name, std::unique_ptr<Event> (*creator)()) {
    event_factories[name] = creator;
  }
  
  // Create all registered events
  std::vector<std::unique_ptr<Event>> create_all_events() {
    std::vector<std::unique_ptr<Event>> events;
    for (const auto& [name, factory] : event_factories) {
      events.push_back(factory());
    }
    return events;
  }
  
private:
  EventRegistry() = default;
  std::unordered_map<std::string, std::unique_ptr<Event> (*)()> event_factories;
};

// Macro to register an event
#define REGISTER_EVENT(EventClass) namespace { std::unique_ptr<Event> create_##EventClass() { return std::make_unique<EventClass>(); } struct EventClass##Registrar { EventClass##Registrar() { EventRegistry::instance().register_event(#EventClass, create_##EventClass); } }; EventClass##Registrar EventClass##_registrar; } 