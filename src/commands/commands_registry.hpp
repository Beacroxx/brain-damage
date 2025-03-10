#pragma once

#include "command.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Command registry to hold all commands
class CommandRegistry {
public:
  static CommandRegistry& instance() {
    static CommandRegistry registry;
    return registry;
  }
  
  // Register a command factory function
  void register_command(const std::string& name, std::unique_ptr<Command> (*creator)()) {
    command_factories[name] = creator;
  }
  
  // Create all registered commands
  std::vector<std::unique_ptr<Command>> create_all_commands() {
    std::vector<std::unique_ptr<Command>> commands;
    for (const auto& [name, factory] : command_factories) {
      commands.push_back(factory());
    }
    return commands;
  }
  
private:
  CommandRegistry() = default;
  std::unordered_map<std::string, std::unique_ptr<Command> (*)()> command_factories;
};

// Macro to register a command (single line version to avoid backslash-newline issues)
#define REGISTER_COMMAND(CommandClass) namespace { std::unique_ptr<Command> create_##CommandClass() { return std::make_unique<CommandClass>(); } struct CommandClass##Registrar { CommandClass##Registrar() { CommandRegistry::instance().register_command(#CommandClass, create_##CommandClass); } }; CommandClass##Registrar CommandClass##_registrar; } 