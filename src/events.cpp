#include "events.hpp"

// Log errors from DPP
void logCallback(const dpp::confirmation_callback_t callback) {
  if (callback.is_error()) {
    std::cerr << "Error: " << callback.get_error().human_readable << std::endl;
  }
}

void deleteAfterAsync(custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds) {
  LOG_DEBUG("Scheduling message deletion in " + std::to_string(seconds) + " seconds");
  std::thread([&bot, msgid, channelid, seconds]() {
    LOG_DEBUG("Starting deleteAfterAsync thread");
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    LOG_DEBUG("Deleting message after " + std::to_string(seconds) + " seconds");
    bot.message_delete(msgid, channelid, logCallback);
  }).detach();
}

void ready(custom_cluster &bot, const dpp::ready_t &event, const std::vector<std::unique_ptr<Command>> &commands) {
  LOG_DEBUG("Bot ready event triggered");
  // Cast event to void to avoid unused variable warning
  (void)event;

  // Get the guild ID
  dpp::snowflake guild_id = bot.get_config().at("guildId").get<dpp::snowflake>();
  LOG_DEBUG("Deleting all commands in the guild");
  // Delete all commands in the guild
  bot.guild_bulk_command_delete_sync(guild_id);

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

void messageCreate(custom_cluster &bot, const dpp::message_create_t &event) {
  // Get the author, message, channel and config
  const dpp::user author = event.msg.author;
  const dpp::message msg = event.msg;
  const dpp::channel channel = bot.channel_get_sync(event.msg.channel_id);
  json config = bot.get_config();
  const std::vector<dpp::snowflake> botChannels = config.at("botChannels");

  // DPP regex downloading
  auto regex = std::regex(
      R"(\b((?:https?:)?\/\/)?((?:www|m)\.)?((?:youtube\.com|youtu\.be|vimeo\.com|dailymotion\.com|twitch\.tv|facebook\.com|instagram\.com)\/(?:[\w\-]+\?v=|embed\/|v\/|clip\/)?)([\w\-]+)(\S+)?\b)",
      std::regex::icase);

  std::smatch match;
  if (std::regex_search(event.msg.content, match, regex)) {
    LOG_DEBUG("URL detected in message");

    // Start typing indicator thread
    std::atomic<bool> typing_indicator_running(true);
    std::thread typing_indicator_thread([&bot, &channel, &typing_indicator_running]() {
      LOG_DEBUG("Starting typing indicator thread");
      while (typing_indicator_running) {
        bot.channel_typing(channel);
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
    });

    std::string url = event.msg.content.substr(match.position(), match.length());

    std::string simcommand = "yt-dlp --simulate -f \"bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best\" --match-filter \"duration<=300\" --max-filesize 8M --recode mp4 -S res,ext:mp4:m4a " +
                             url + " -o \"../media/ytdlp/output.mp4\" 2>&1";
    char buffer[2048];
    std::string result = "";
    FILE *pipe = popen(simcommand.c_str(), "r");
    if (!pipe) {
      LOG_DEBUG("Failed to open pipe for yt-dlp simulation command");
      typing_indicator_running = false;
      typing_indicator_thread.join();
      return;
    }
    while (!feof(pipe)) {
      if (fgets(buffer, 2048, pipe) != NULL) {
        result += buffer;
      }
    }
    pclose(pipe);

    if (result.find("skipping") != std::string::npos) {
      LOG_DEBUG("Video too long or too large");
      typing_indicator_running = false;
      typing_indicator_thread.join();
      auto msg = dpp::message("The video is too long or too large.").set_channel_id(event.msg.channel_id);
      bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
        if (!callback.is_error()) {
          const dpp::message &created_msg = std::get<dpp::message>(callback.value);
          deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
        }
      });
      return;
    }

    if (result.find("ERROR:") != std::string::npos) {
      LOG_DEBUG("Error downloading video");
      typing_indicator_running = false;
      typing_indicator_thread.join();
      auto msg = dpp::message("Couldn't download the video. Make sure the link is valid and contains a video")
                     .set_channel_id(event.msg.channel_id);
      bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
        if (!callback.is_error()) {
          const dpp::message &created_msg = std::get<dpp::message>(callback.value);
          deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
        }
      });
      return;
    }

    std::string command = "yt-dlp -f \"bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best\" --match-filter \"duration<=300\" --max-filesize 8M --recode mp4 -S res,ext:mp4:m4a " +
                          url + " -o \"../media/ytdlp/output.mp4\"";
    pipe = popen(command.c_str(), "r");
    if (!pipe) {
      LOG_DEBUG("Failed to open pipe for yt-dlp command");
      typing_indicator_running = false;
      typing_indicator_thread.join();
      return;
    }
    while (!feof(pipe)) {
      if (fgets(buffer, 2048, pipe) != NULL) {
        result += buffer;
      }
    }
    pclose(pipe);

    // find the file
    std::string path;
    for (const auto &entry : fs::directory_iterator("../media/ytdlp/")) {
      if (entry.is_regular_file() && entry.path().filename().string().find("output.mp4") != std::string::npos) {
        path = entry.path().string();
        break;
      }
    }

    if (path.empty()) {
      LOG_DEBUG("Downloaded video file not found");
      typing_indicator_running = false;
      typing_indicator_thread.join();

      auto msg = dpp::message("The video dissapeared :)").set_channel_id(event.msg.channel_id);
      bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
        if (!callback.is_error()) {
          const dpp::message &created_msg = std::get<dpp::message>(callback.value);
          deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
        }
      });
      return;
    }

    // Reply with the file
    std::ifstream f(path);
    if (f) {

      // Read the file size
      f.seekg(0, std::ios::end);
      std::streamsize length = f.tellg();
      f.seekg(0, std::ios::beg);

      // Read the file content
      std::vector<char> buffer(length);
      if (f.read(buffer.data(), length)) {
        std::string fileContent(buffer.begin(), buffer.end());

				event.reply(dpp::message().add_file(path, fileContent, "video"), true, [&bot, &event](const dpp::confirmation_callback_t &callback) {
					if (callback.is_error()) {
						auto msg = dpp::message("Video too fat").set_channel_id(event.msg.channel_id);
						bot.message_create(msg, [&bot](const dpp::confirmation_callback_t &callback) {
							if (!callback.is_error()) {
								const dpp::message &created_msg = std::get<dpp::message>(callback.value);
								deleteAfterAsync(bot, created_msg.id, created_msg.channel_id, 10);
							}
						});
					}
				});
        f.close();
      }
    }

    typing_indicator_running = false;
    typing_indicator_thread.join();

    // remove all files in the directory ytdlp
    for (const auto &entry : fs::directory_iterator("../media/ytdlp/")) {
      if (entry.is_regular_file()) {
        fs::remove(entry.path());
      }
    }
  }

  // Ignore messages from the bot itself and from channels that are not bot channels
  if (author.id == bot.me.id ||
      std::find(botChannels.begin(), botChannels.end(), channel.id) == botChannels.end())
    return;

  // Get the content of the message
  std::string content = event.msg.content;
  std::transform(content.begin(), content.end(), content.begin(), ::tolower);

  // Get the mentions, attachments and keywords
  const std::vector<std::pair<dpp::user, dpp::guild_member>> mentions = event.msg.mentions;
  const std::vector<dpp::attachment> attachments = event.msg.attachments;
  const std::map<std::string, std::string> keywords = config.at("keyWords");
  const std::map<std::string, std::string> keyWordsFiles = config.at("keyWordsFiles");

  // React with an emoji to all attachments in the specified channel
  if (channel.id == config.at("specialChannel").get<dpp::snowflake>() && !attachments.empty()) {
    bot.message_add_reaction(msg, config.at("specialChannelEmote").get<std::string>(), logCallback);
  }

  // Check for custom keywords and reply with the corresponding response
  for (const auto &keyword : keywords) {
    if (content.find(keyword.first) != std::string::npos) {
      event.reply(dpp::message(keyword.second), true, logCallback);
    }
  }

  // Check for custom keywords with file responses and reply with the corresponding file
  for (const auto &keyWordFile : keyWordsFiles) {
    if (content.find(keyWordFile.first) != std::string::npos) {
      std::ifstream f("../media/" + keyWordFile.second);
      if (f) {
        // typing indicator coroutine
        bot.channel_typing(channel);

        // Read the file size
        f.seekg(0, std::ios::end);
        std::streamsize length = f.tellg();
        f.seekg(0, std::ios::beg);

        // Read the file content
        std::vector<char> buffer(length);
        if (f.read(buffer.data(), length)) {
          std::string fileContent(buffer.begin(), buffer.end());

          // Reply with the file
          event.reply(dpp::message().add_file(keyWordFile.second, fileContent), true, logCallback);
          f.close();
        }
      }
    }
  }

  // Holy hell easter egg
  if (content.find("holy hell") != std::string::npos) {
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
    dpp::message msg_ = dpp::message().set_reference(msg.id).set_channel_id(channel.id);
    msg_.allowed_mentions.replied_user = true;

    // Reply with a random sentence and set the reference to the sent message
    for (const auto &sentence : arr) {

      msg_.set_reference(bot.message_create_sync(msg_.set_content(sentence)).id);

      // Sleep for 1 second (rate limit)
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
}
