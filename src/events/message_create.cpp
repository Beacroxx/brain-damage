#include "event.hpp"
#include "events_registry.hpp"
#include <regex>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

void logCallback(const dpp::confirmation_callback_t callback);
void deleteAfterAsync(custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds);

class MessageCreateEvent : public Event {
public:
  void execute(custom_cluster &bot, const dpp::event_dispatch_t &event) override {
    const auto &message_event = static_cast<const dpp::message_create_t &>(event);
    
    // Get the author, message, channel and config
    const dpp::user author = message_event.msg.author;
    const dpp::message msg = message_event.msg;
    const dpp::channel channel = bot.channel_get_sync(message_event.msg.channel_id);
    json config = bot.get_config();
    const std::vector<dpp::snowflake> botChannels = config.at("botChannels");

    // Ignore messages from the bot itself and from channels that are not bot channels
    if (author.id == bot.me.id ||
        std::find(botChannels.begin(), botChannels.end(), channel.id) == botChannels.end())
      return;

    // Get the content of the message
    std::string content = message_event.msg.content;
    std::transform(content.begin(), content.end(), content.begin(), ::tolower);

    // Get the mentions, attachments and keywords
    const std::vector<std::pair<dpp::user, dpp::guild_member>> mentions = message_event.msg.mentions;
    const std::vector<dpp::attachment> attachments = message_event.msg.attachments;
    const std::map<std::string, std::string> keywords = config.at("keyWords");
    const std::map<std::string, std::string> keyWordsFiles = config.at("keyWordsFiles");

    // React with an emoji to all attachments in the specified channel
    if (channel.id == config.at("specialChannel").get<dpp::snowflake>() && !attachments.empty()) {
      bot.message_add_reaction(msg, config.at("specialChannelEmote").get<std::string>(), logCallback);
    }

    // Check for custom keywords and reply with the corresponding response
    for (const auto &keyword : keywords) {
      if (content.find(keyword.first) != std::string::npos) {
        message_event.reply(dpp::message(keyword.second), true, logCallback);
      }
    }

    // Check for custom keywords with file responses and reply with the corresponding file
    for (const auto &keyWordFile : keyWordsFiles) {
      if (content.find(keyWordFile.first) != std::string::npos) {
        handleFileResponse(bot, message_event, keyWordFile.second);
      }
    }

    // Holy hell easter egg
    if (content.find("holy hell") != std::string::npos) {
      handleHolyHellEasterEgg(bot, message_event);
    }
  }

  std::string get_name() const override { return "message_create"; }

private:
  void handleFileResponse(custom_cluster &bot, const dpp::message_create_t &event, const std::string &filename) {
    std::ifstream f("../media/" + filename);
    if (f) {
      // typing indicator coroutine
      bot.channel_typing(bot.channel_get_sync(event.msg.channel_id));

      // Read the file size
      f.seekg(0, std::ios::end);
      std::streamsize length = f.tellg();
      f.seekg(0, std::ios::beg);

      // Read the file content
      std::vector<char> buffer(length);
      if (f.read(buffer.data(), length)) {
        std::string fileContent(buffer.begin(), buffer.end());

        // Reply with the file
        event.reply(dpp::message().add_file(filename, fileContent), true, logCallback);
        f.close();
      }
    }
  }

  void handleHolyHellEasterEgg(custom_cluster &bot, const dpp::message_create_t &event) {
    std::vector<std::string> arr = {"New Response just dropped",
                                   "Actual Zombie",
                                   "Call the exorcist",
                                   "Bishop goes on vacation, never comes back",
                                   "Knightmare fuel",
                                   "Pawn storm incoming!",
                                   "Checkmate or riot!",
                                   "Queen sacrifice, anyone?",
                                   "Rook in the corner, plotting world domination",
                                   "Brainless Parrots",
                                   "Ignite the Chessboard!",
                                   "Jessica is not fucking welcome here!",
                                   "Holy bishops on skateboards"};

    // Reply with a random sentence
    dpp::message msg_ = dpp::message().set_reference(event.msg.id).set_channel_id(event.msg.channel_id);
    msg_.allowed_mentions.replied_user = true;

    // Reply with a random sentence and set the reference to the sent message
    for (const auto &sentence : arr) {
      msg_.set_reference(bot.message_create_sync(msg_.set_content(sentence)).id);
      // Sleep for 1 second (rate limit)
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
};

// Register the event
REGISTER_EVENT(MessageCreateEvent) 