#include "../main.hpp"
#include "command.hpp"
#include "commands_registry.hpp"

class PingCommand : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    (void)bot; // Unused

    // Get the timestamp of the message
    const long timestamp = event.command.get_creation_time() * 1000; // Convert to milliseconds

    // Calculate the latency from timestamp to now
    const long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const long latency = now - timestamp;

    // Reply with "Pong! Latency: <latency>ms"
    event.reply("Pong! Latency: " + std::to_string(latency) + "ms");
  }

  std::string get_name() const override { return "ping"; }

  std::string get_description() const override { return "Ping Pong!"; }

  std::vector<dpp::command_option> get_options() const override { return {}; }

  dpp::permission get_permissions() const override { return dpp::p_use_application_commands; }
};

// Register the command using the macro
REGISTER_COMMAND(PingCommand)
