#include "event.hpp"
#include "events_registry.hpp"
#include "../commands/command.hpp"
#include "../commands/commands_registry.hpp"

class ReadyEvent : public Event {
public:
  void execute(custom_cluster &bot, const dpp::event_dispatch_t &event) override {
    const auto &ready_event = static_cast<const dpp::ready_t &>(event);
    (void)ready_event; // Suppress unused variable warning
    LOG_DEBUG("Bot ready event triggered");

    // Get the guild ID
    dpp::snowflake guild_id = bot.get_config().at("guildId").get<dpp::snowflake>();
    LOG_DEBUG("Deleting all commands in the guild");
    
    // Delete all commands in the guild
    bot.guild_bulk_command_delete_sync(guild_id);

    // Get commands from the command registry
    std::vector<std::unique_ptr<Command>> commands = CommandRegistry::instance().create_all_commands();

    // Create a vector of slash commands from the commands vector
    std::vector<dpp::slashcommand> scommands;
    for (const auto &command : commands) {
      dpp::slashcommand scommand = dpp::slashcommand(command->get_name(), command->get_description(), bot.me.id);
      for (const auto &option : command->get_options()) {
        scommand.add_option(option);
      }
      scommand.set_default_permissions(command->get_permissions());
      scommands.push_back(scommand);
    }

    LOG_DEBUG("Creating all slash commands in the guild");
    // Create all slash commands in the guild
    bot.guild_bulk_command_create_sync(scommands, guild_id);

    // Set the bot's status
    dpp::activity activity;
    activity.type = dpp::activity_type::at_game;
    activity.name = "with fire";

    bot.set_presence(dpp::presence(dpp::presence_status::ps_online, activity));
  }

  std::string get_name() const override { return "ready"; }
};

// Register the event
REGISTER_EVENT(ReadyEvent) 