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

    // DPP regex downloading
    auto regex = std::regex(
        R"(\b((?:https?:)?\/\/)?((?:www|m)\.)?((?:youtube\.com|youtu\.be|vimeo\.com|dailymotion\.com|twitch\.tv|facebook\.com|instagram\.com)\/(?:[\w\-]+\?v=|embed\/|v\/|clip\/)?)([\w\-]+)(\S+)?\b)",
        std::regex::icase);

    std::smatch match;
    if (std::regex_search(message_event.msg.content, match, regex)) {
      handleVideoDownload(bot, message_event, match);
      return;
    }

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
  void handleVideoDownload(custom_cluster &bot, const dpp::message_create_t &event, const std::smatch &match) {
    LOG_DEBUG("URL detected in message");

    // Start typing indicator thread
    std::atomic<bool> typing_indicator_running(true);
    const dpp::channel channel = bot.channel_get_sync(event.msg.channel_id);
    std::thread typing_indicator_thread([&bot, &channel, &typing_indicator_running]() {
      LOG_DEBUG("Starting typing indicator thread");
      while (typing_indicator_running) {
        bot.channel_typing(channel);
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
    });

    const std::string url = event.msg.content.substr(match.position(), match.length());
    const std::string output_dir = "../media/ytdlp";
    const std::string output_template = output_dir + "/output.%(ext)s";
    
    // Create output directory if it doesn't exist
    fs::create_directories(output_dir);

    // Clean any existing files first
    for (const auto &entry : fs::directory_iterator(output_dir)) {
      fs::remove(entry.path());
    }

    // Common yt-dlp options for better reliability
    const std::string common_opts = 
      "--no-playlist "
      "--match-filter \"duration<=300\" "
      "--max-filesize 8M "
      "-S res,ext:mp4:m4a "
      "--merge-output-format mp4 "
      "--format \"bv*[ext=mp4]+ba[ext=m4a]/b[ext=mp4]/bv*+ba/b\" ";

    // First simulate the download to check for issues
    std::string simcommand = "yt-dlp --simulate " + common_opts + 
                            " -o \"" + output_template + "\" " +
                            url + " 2>&1";

    std::array<char, 2048> buffer;
    std::string result;
    
    {
      std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(simcommand.c_str(), "r"), pclose);
      if (!pipe) {
        LOG_DEBUG("Failed to open pipe for yt-dlp simulation command");
        typing_indicator_running = false;
        typing_indicator_thread.join();
        event.reply("Failed to process video download request.", true);
        return;
      }

      while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
      }
    }

    if (result.find("skipping") != std::string::npos || 
        result.find("ERROR:") != std::string::npos ||
        result.empty()) {
      LOG_DEBUG("Video validation failed: " + result);
      typing_indicator_running = false;
      typing_indicator_thread.join();
      
      std::string error_msg = "Could not process this video. ";
      if (result.find("duration") != std::string::npos) {
        error_msg += "Video is too long (max 5 minutes).";
      } else if (result.find("filesize") != std::string::npos) {
        error_msg += "Video file is too large (max 8MB).";
      } else {
        error_msg += "Make sure the link is valid and contains a video.";
      }
      
      auto msg = dpp::message(error_msg).set_channel_id(event.msg.channel_id);
      bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
        if (!callback.is_error()) {
          const dpp::message &created_msg = std::get<dpp::message>(callback.value);
          deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
        }
      });
      return;
    }

    // Proceed with actual download
    std::string download_cmd = "yt-dlp " + common_opts +
                              " -o \"" + output_template + "\" " +
                              url;

    {
      std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(download_cmd.c_str(), "r"), pclose);
      if (!pipe) {
        LOG_DEBUG("Failed to open pipe for yt-dlp download command");
        typing_indicator_running = false;
        typing_indicator_thread.join();
        event.reply("Failed to download the video.", true);
        return;
      }
    }

    // Find the downloaded file (should be mp4 but let's be thorough)
    std::string downloaded_file;
    for (const auto &entry : fs::directory_iterator(output_dir)) {
      if (entry.is_regular_file()) {
        downloaded_file = entry.path().string();
        break;
      }
    }

    if (downloaded_file.empty()) {
      LOG_DEBUG("Downloaded video file not found");
      typing_indicator_running = false;
      typing_indicator_thread.join();

      auto msg = dpp::message("Failed to process the video.").set_channel_id(event.msg.channel_id);
      bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
        if (!callback.is_error()) {
          const dpp::message &created_msg = std::get<dpp::message>(callback.value);
          deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
        }
      });
      return;
    }

    // Read and send the file
    std::ifstream file(downloaded_file, std::ios::binary);
    if (!file) {
      LOG_DEBUG("Failed to open downloaded file for reading");
      typing_indicator_running = false;
      typing_indicator_thread.join();
      event.reply("Failed to process the downloaded video.", true);
      return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize length = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> file_buffer(length);
    if (file.read(file_buffer.data(), length)) {
      std::string filename = fs::path(downloaded_file).filename().string();
      event.reply(
        dpp::message().add_file(filename, std::string(file_buffer.begin(), file_buffer.end()), "video"),
        true,
        [&bot, &event](const dpp::confirmation_callback_t &callback) {
          if (callback.is_error()) {
            auto msg = dpp::message("Failed to send the video (file might be too large for Discord).")
                          .set_channel_id(event.msg.channel_id);
            bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
              if (!callback.is_error()) {
                const dpp::message &created_msg = std::get<dpp::message>(callback.value);
                deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
              }
            });
          }
        }
      );
    }

    file.close();
    typing_indicator_running = false;
    typing_indicator_thread.join();

    // Cleanup downloaded files
    for (const auto &entry : fs::directory_iterator(output_dir)) {
      fs::remove(entry.path());
    }
  }

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