#include "../command.hpp"
#include "../../main.hpp"

class PingCommand : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    event.reply("Pong!");
  }

  std::string get_name() const override { return "ping"; }

  std::string get_description() const override { return "Ping Pong!"; }

  std::vector<dpp::command_option> get_options() const override { return {}; }

  dpp::permission get_permissions() const override { return dpp::p_use_application_commands; }
};

// Export the function
extern "C" {
Command *create_ping_command() { return new PingCommand(); }
}
