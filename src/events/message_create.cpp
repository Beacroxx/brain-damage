#include "event.hpp"
#include "events_registry.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <thread>

namespace fs = std::filesystem;

void logCallback(const dpp::confirmation_callback_t callback);
void deleteAfterAsync(custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds);

class MessageCreateEvent : public Event {
public:
  dpp::task<void> execute(custom_cluster &bot, const dpp::event_dispatch_t &event) override {
    const auto &message_event = static_cast<const dpp::message_create_t &>(event);

    // Get the author, message, channel and config
    const dpp::user author = message_event.msg.author;
    const dpp::message msg = message_event.msg;

    // Use coroutine to get channel info
    dpp::confirmation_callback_t channel_result = co_await bot.co_channel_get(message_event.msg.channel_id);
    if (channel_result.is_error()) {
      co_return;
    }
    const dpp::channel channel = channel_result.get<dpp::channel>();

    json config = bot.get_config();
    const std::vector<dpp::snowflake> botChannels = config.at("botChannels");

    // Ignore messages from the bot itself and from channels that are not bot channels
    if (author.id == bot.me.id || std::find(botChannels.begin(), botChannels.end(), channel.id) == botChannels.end()) {
      co_return;
    }

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
        // delay for 50ms per char while typing (clamp at 2s)
        bot.channel_typing(channel);
        std::this_thread::sleep_for(std::chrono::milliseconds(std::min(2000ul, 50 * keyword.second.length())));
        message_event.reply(dpp::message(keyword.second), true, logCallback);
      }
    }

    // Check for custom keywords with file responses and reply with the corresponding file
    for (const auto &keyWordFile : keyWordsFiles) {
      if (content.find(keyWordFile.first) != std::string::npos) {
        co_await handleFileResponse(bot, message_event, keyWordFile.second);
      }
    }

    // Holy hell easter egg
    if (content.find("holy hell") != std::string::npos) {
      co_await handleHolyHellEasterEgg(bot, message_event);
    }
  }

  std::string get_name() const override { return "message_create"; }

private:
  dpp::task<void> handleFileResponse(custom_cluster &bot, const dpp::message_create_t &event,
                                     const std::string &filename) {
    std::ifstream f("../media/" + filename);
    if (f) {
      // typing indicator coroutine
      dpp::confirmation_callback_t channel_result = co_await bot.co_channel_get(event.msg.channel_id);
      if (!channel_result.is_error()) {
        bot.channel_typing(channel_result.get<dpp::channel>());
      }

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

  dpp::task<void> handleHolyHellEasterEgg(custom_cluster &bot, const dpp::message_create_t &event) {
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

    // Start typing indicator
    dpp::confirmation_callback_t channel_result = co_await bot.co_channel_get(event.msg.channel_id);
    dpp::channel channel;
    if (!channel_result.is_error()) {
      channel = channel_result.get<dpp::channel>();
    }

    // Reply with a random sentence
    dpp::message msg_ = dpp::message().set_reference(event.msg.id).set_channel_id(event.msg.channel_id);
    msg_.allowed_mentions.replied_user = true;

    // Reply with a sentence and set the reference to the sent message
    for (const auto &sentence : arr) {
      // sleep for 50ms per char typing
      bot.channel_typing(channel);
      std::this_thread::sleep_for(std::chrono::milliseconds(50 * sentence.length()));
      dpp::confirmation_callback_t result = co_await bot.co_message_create(msg_.set_content(sentence));
      if (!result.is_error()) {
        msg_.set_reference(result.get<dpp::message>().id);
      }
    }
  }
};

// Register the event
REGISTER_EVENT(MessageCreateEvent)