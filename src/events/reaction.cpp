#include "event.hpp"
#include "events_registry.hpp"
#include "../starboard.hpp"

class ReactionEvent : public Event {
public:
  void execute(custom_cluster &bot, const dpp::event_dispatch_t &event) override {
    if (const auto *add_event = dynamic_cast<const dpp::message_reaction_add_t *>(&event)) {
      updateStarboardMessage(bot, *add_event);
    } else if (const auto *remove_event = dynamic_cast<const dpp::message_reaction_remove_t *>(&event)) {
      updateStarboardMessage(bot, *remove_event);
    }
  }

  std::string get_name() const override { return "reaction"; }
};

// Register the event
REGISTER_EVENT(ReactionEvent) 