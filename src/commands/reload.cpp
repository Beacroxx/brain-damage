#include "../main.hpp"
#include "command.hpp"

class ReloadCommand : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    bot.load_config();
    event.reply("Config reloaded!");
  }

  std::string get_name() const override { return "reload"; }
  std::string get_description() const override { return "Reloads the config"; }
  std::vector<dpp::command_option> get_options() const override { return {}; }
  dpp::permission get_permissions() const override { return dpp::p_administrator; }
};

// Export the function
extern "C" {
Command *create_reload_command() { return new ReloadCommand(); }
}
