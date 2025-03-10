#include "../main.hpp"
#include "command.hpp"
#include "commands_registry.hpp"

using json = nlohmann::json;

class KeywordFileCommand : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    event.reply("Downloading file...");
    json config = bot.get_config(); 
    std::string keyword = std::get<std::string>(event.get_parameter("keyword"));
    dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("response"));

    dpp::attachment response = event.command.resolved.attachments.at(file_id);

    int ret = system(("curl -L '" + response.url + "' -o ../media/" + response.filename).c_str());

    if (ret != 0) {
      event.edit_response("Failed to download file! Return Code: " + std::to_string(ret));
      return;
    }

    config.at("keyWordsFiles")[keyword] = response.filename;

    event.edit_response("Keyword added!");

    bot.save_config(config);
  }

  std::string get_name() const override { return "keywordfile"; }

  std::string get_description() const override { return "Add a keyword"; }

  std::vector<dpp::command_option> get_options() const override { return {
    dpp::command_option(dpp::co_string, "keyword", "Keyword", true),
    dpp::command_option(dpp::co_attachment, "response", "Response", true)
  }; }

  dpp::permission get_permissions() const override { return dpp::p_use_application_commands; }
};

// Register the command using the macro
REGISTER_COMMAND(KeywordFileCommand)

