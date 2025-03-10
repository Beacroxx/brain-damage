#pragma once
#include <dpp/dpp.h>
#include "../main.hpp"
#include <string>

#ifdef VERBOSE_DEBUG
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

class Event {
public:
  virtual ~Event() = default;
  virtual std::string get_name() const = 0;
  virtual void execute(custom_cluster &bot, const dpp::event_dispatch_t &event) = 0;
}; 