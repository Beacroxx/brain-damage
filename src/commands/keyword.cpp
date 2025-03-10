#include "../main.hpp"
#include "command.hpp"
#include "commands_registry.hpp"

using json = nlohmann::json;

class KeywordCommand : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    json config = bot.get_config(); 
    std::string keyword = std::get<std::string>(event.get_parameter("keyword"));
    std::string response = std::get<std::string>(event.get_parameter("response"));

    config.at("keyWords")[keyword] = response;

    event.reply("Keyword Added!");

    bot.save_config(config);
  }

  std::string get_name() const override { return "keyword"; }

  std::string get_description() const override { return "Add a keyword"; }

  std::vector<dpp::command_option> get_options() const override { return {
    dpp::command_option(dpp::co_string, "keyword", "Keyword", true),
    dpp::command_option(dpp::co_string, "response", "Response", true)
  }; }

  dpp::permission get_permissions() const override { return dpp::p_use_application_commands; }
};

// Register the command using the macro
REGISTER_COMMAND(KeywordCommand)

