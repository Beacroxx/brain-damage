#include "main.hpp"

#include "commands/command.hpp"

#include <concepts>
#include <csignal>
#include <dlfcn.h>
#include <dpp/dpp.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Log errors from DPP
void logCallback( const dpp::confirmation_callback_t callback ) {
  if ( callback.is_error() ) {
    std::cerr << "Error: " << callback.get_error().human_readable << std::endl;
  }
}

// Exit gracefully on signal
void signalHandler( int signal ) {
  std::cout << "Caught signal " << signal << ", exiting..." << std::endl;
  exit( 0 );
}

template <typename EventType> void updateStarboardMessage( custom_cluster &bot, const EventType &event ) {
  std::unique_lock<std::mutex> lock( bot.starboard_mutex, std::defer_lock );

  // If the mutex is already locked, wait for it to unlock
  if ( !lock.try_lock() ) {
    std::cout << "Starboard: Waiting for mutex..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    lock.lock();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( stop - start );
    std::cout << "Starboard: Mutex acquired after " << duration.count() << "ms" << std::endl;
  }

  // Get the message, channel, and star count
  const dpp::message msg = bot.message_get_sync( event.message_id, event.channel_id );
  const auto starCountIt = std::find_if( msg.reactions.begin(), msg.reactions.end(),
                                         []( const dpp::reaction &r ) { return r.emoji_name == "⭐"; } );
  const int starCount = starCountIt != msg.reactions.end() ? starCountIt->count : 0;
  const dpp::channel channel = bot.channel_get_sync( event.channel_id );
  const long timestamp = msg.get_creation_time();

  // Check if the message is already in the starboard
  auto &starboard = bot.starboard;
  const bool starboarded = starboard.find( msg.get_url() ) != starboard.end();

  // If the message has less than 2 stars and a reaction has been removed, remove it from the starboard
  if ( starCount < 2 ) {
    if ( starboarded && std::is_same_v<EventType, dpp::message_reaction_remove_t> ) {
      bot.message_delete_sync( starboard[ msg.get_url() ].id, starboard[ msg.get_url() ].channel_id );
      starboard.erase( msg.get_url() );
      auto thread_it = bot.starboard_threads.find( msg.get_url() );
      if ( thread_it != bot.starboard_threads.end() ) {
        bot.starboard_threads.erase( msg.get_url() );
      }
    }
    return;
  }

  // Create the embed message
  dpp::embed e;
  e.set_author( msg.author.username, msg.author.get_url(), msg.author.get_avatar_url() );
  e.set_color( dpp::colors::yellow );
  e.set_timestamp( timestamp );

  // Get the referenced message
  const dpp::message::message_ref reference = msg.message_reference;
  dpp::message ref;
  if ( !reference.message_id.empty() )
    ref = bot.message_get_sync( reference.message_id, event.channel_id );

  // Format the referenced message content
  std::string refcontent = ref.content;
  if ( !refcontent.empty() ) {
    refcontent = "> **" + ref.author.username + ":**" + "\n" + refcontent;
    std::stringstream tmp;
    std::for_each( refcontent.begin(), refcontent.end(), [ &tmp ]( char c ) {
      if ( c == '\n' )
        tmp << "\n> ";
      else
        tmp << c;
    } );
    refcontent = tmp.str();
  }

  // Create the message
  e.set_description( ( refcontent.empty() ? "" : refcontent + "\n\n" ) + msg.content + "\n\n" );

  // Add attachments
  if ( !msg.attachments.empty() ) {
    const dpp::attachment a = msg.attachments.at( 0 );
    if ( a.content_type.find( "image" ) != std::string::npos ) {
      e.set_image( a.url );
    } else if ( a.content_type.find( "video" ) != std::string::npos ) {
      e.set_video( a.url );
    } else {
      e.add_field( "Attachment", a.url );
    }
  }

  // Check if the message is starred
  if ( starboarded ) {
    // Edit the starboard message
    dpp::message &m = starboard[ msg.get_url() ];
    m.embeds.at( 0 ) = e;
    m.set_content( "⭐ **" + std::to_string( starCount ) + "** | [`# " + channel.name + "`](<" + msg.get_url() + ">)" );
    m = bot.message_edit_sync( m );
  } else if ( starCount == 2 && std::is_same_v<EventType, dpp::message_reaction_add_t> ) {
    // Post in starboard channel
    const dpp::channel starboard_channel =
        bot.channel_get_sync( bot.get_config().at( "starboardChannel" ).get<dpp::snowflake>() );
    starboard[ msg.get_url() ] =
        bot.message_create_sync( dpp::message( "⭐ **" + std::to_string( starCount ) + "** | [`# " + channel.name +
                                               "`](<" + msg.get_url() + ">)" )
                                     .add_embed( e )
                                     .set_channel_id( starboard_channel.id ) );

    // Remove the message after 3 days ( thread magic )
    std::string url = msg.get_url();
    auto thread = std::make_shared<std::thread>( [ botPtr = &bot, url ]() {
      std::this_thread::sleep_for( std::chrono::hours( 24 * 3 ) );
      std::unique_lock<std::mutex> lock( botPtr->starboard_mutex, std::defer_lock );
      if ( !lock.try_lock() ) {
        std::cout << "Thread: Waiting for mutex..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        lock.lock();
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( stop - start );
        std::cout << "Thread: Mutex acquired after " << duration.count() << "ms" << std::endl;
      }
      auto &starboard = botPtr->starboard;
      if ( starboard.find( url ) != starboard.end() &&
           botPtr->starboard_threads.find( url ) != botPtr->starboard_threads.end() ) {
        starboard.erase( url );
      }
      botPtr->starboard_threads.erase( url );
    } );

    // Store the thread and detach it
    bot.starboard_threads[ msg.get_url() ] = thread;
    thread->detach();
  }
}

int main() {
  // Initialize signal handler
  std::signal( SIGINT, signalHandler );

  // Load config
  std::fstream cfg_fstream;
  cfg_fstream.open( "../config.json", std::fstream::in );
  if ( !cfg_fstream.is_open() ) {
    std::cerr << "Failed to open config.json" << std::endl;
    return 1;
  }
  json config = json::parse( cfg_fstream );
  cfg_fstream.close();

  // Initialize bot
  custom_cluster bot( config.at( "token" ), dpp::intents::i_all_intents );
  bot.load_config();
  bot.on_log( dpp::utility::cout_logger() );

  // Initialize commands
  std::vector<std::unique_ptr<Command>> commands;

  // Iterate over all subdirectories in build/commands/
  for ( const auto &entry : fs::directory_iterator( "./commands/" ) ) {
    if ( entry.is_directory() ) {
      // Iterate over .so files within each subdirectory
      for ( const auto &file : fs::directory_iterator( entry.path() ) ) {
        if ( file.is_regular_file() && file.path().extension() == ".so" ) {
          // Load the .so file
          void *lib_handle = dlopen( file.path().c_str(), RTLD_LAZY );
          if ( lib_handle ) {
            typedef Command *( *create_command_func )();
            // Assume the function name is "create_<cmd_name>_command"
            std::string command_name = entry.path().filename().string();
            std::string func_name = "create_" + command_name + "_command";
            create_command_func create_command = (create_command_func)dlsym( lib_handle, func_name.c_str() );
            if ( create_command ) {
              commands.push_back( std::unique_ptr<Command>( create_command() ) );
            } else {
              std::cerr << "Error loading command from library: " << dlerror() << std::endl;
            }
          } else {
            std::cerr << "Error opening library: " << dlerror() << std::endl;
          }
        }
      }
    }
  }

  // Bot ready event
  bot.on_ready( [ &bot, &commands ]( const dpp::ready_t &event ) {
    dpp::snowflake guild_id = bot.get_config().at( "guildId" ).get<dpp::snowflake>();

    // Delete all commands in the guild
    bot.guild_bulk_command_delete_sync( guild_id );

    // Create a vector of slash commands from the commands vector
    std::vector<dpp::slashcommand> scommands;
    for ( const auto &command : commands ) {
      dpp::slashcommand scommand = dpp::slashcommand( command->get_name(), command->get_description(), bot.me.id );
      for ( const auto &option : command->get_options() ) {
        scommand.add_option( option );
      }
      scommand.set_default_permissions( command->get_permissions() );
      scommands.push_back( scommand );
    }

    // Create all slash commands in the guild
    bot.guild_bulk_command_create_sync( scommands, guild_id );

    // set status
    dpp::activity activity;
    activity.type = dpp::activity_type::at_game;
    activity.name = "with fire";

    bot.set_presence( dpp::presence( dpp::presence_status::ps_online, activity ) );
  } );

  bot.on_message_create( [ &bot ]( const dpp::message_create_t &event ) {
    // Get the author, message, channel and config
    const dpp::user author = event.msg.author;
    const dpp::message msg = event.msg;
    const dpp::channel channel = bot.channel_get_sync( event.msg.channel_id );
    json config = bot.get_config();
    const std::vector<dpp::snowflake> botChannels = config.at( "botChannels" );

    // DPP regex downloading
    auto regex = std::regex(
      R"(\b((?:https?:)?\/\/)?((?:www|m)\.)?((?:youtube\.com|youtu\.be|vimeo\.com|dailymotion\.com|twitch\.tv|tiktok\.com|facebook\.com|instagram\.com)\/(?:[\w\-]+\?v=|embed\/|v\/|clip\/)?)([\w\-]+)(\S+)?\b)",
      std::regex::icase
    );
  
    std::smatch match;
    if ( std::regex_search( event.msg.content, match, regex ) ) {
      std::string url = event.msg.content.substr( match.position(), match.length() );

      std::string simcommand = "yt-dlp --simulate --match-filter \"duration<=300\" --max-filesize 10M -f \"bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best\" " + url +
                            " -o \"../media//ytdlp/output.webm\"";
      char buffer[128];
      std::string result = "";
      FILE *pipe = popen( simcommand.c_str(), "r" );
      if ( !pipe ) {
        return;
      }
      while ( !feof( pipe ) ) {
        if ( fgets( buffer, 128, pipe ) != NULL ) {
          result += buffer;
        }
      }
      pclose( pipe );
      if (result.empty()) {
        // did not match filter or max filesize
        std ::cout << "did not match filter or max filesize" << std::endl;
        return;
      }
      // Start typing indicator thread
      std::atomic<bool> typing_indicator_running(true);
      std::thread typing_indicator_thread([&bot, &channel, &typing_indicator_running]() {
        while (typing_indicator_running) {
          bot.channel_typing(channel);
          std::this_thread::sleep_for(std::chrono::seconds(2));
        }
      });

      std::string command = "yt-dlp --match-filter \"duration<=300\" --max-filesize 10M -f \"bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best\" --recode webm " + url +
                " -o \"../media//ytdlp/output.webm\"";
      system(command.c_str()); // Download the video

      // find the file
      std::string path;
      for (const auto &entry : fs::directory_iterator("../media/ytdlp/")) {
        if (entry.is_regular_file() && entry.path().filename().string().find("output") != std::string::npos) {
          path = entry.path().string();
          break;
        }
      }

      if (path.empty()) {
        typing_indicator_running = false;
        typing_indicator_thread.join();
        return;
      }

      // Reply with the file
      std::ifstream f(path);
      if (f) {

        // Read the file size
        f.seekg( 0, std::ios::end );
        std::streamsize length = f.tellg();
        f.seekg( 0, std::ios::beg );

        // Read the file content
        std::vector<char> buffer( length );
        if ( f.read( buffer.data(), length ) ) {
          std::string fileContent( buffer.begin(), buffer.end() );

            event.reply( dpp::message().add_file( path, fileContent, "video" ), true, logCallback );
          f.close();
        }
      }

      typing_indicator_running = false;
      typing_indicator_thread.join();

      // remove all files in the directory ytdlp
      for ( const auto &entry : fs::directory_iterator( "../media/ytdlp/" ) ) {
        if ( entry.is_regular_file() ) {
          fs::remove( entry.path() );
        }
      }
    }

    // Ignore messages from the bot itself and from channels that are not bot channels
    if ( author.id == bot.me.id ||
         std::find( botChannels.begin(), botChannels.end(), channel.id ) == botChannels.end() )
      return;

    // Get the content of the message
    std::string content = event.msg.content;
    std::transform( content.begin(), content.end(), content.begin(), ::tolower );

    // Get the mentions, attachments and keywords
    const std::vector<std::pair<dpp::user, dpp::guild_member>> mentions = event.msg.mentions;
    const std::vector<dpp::attachment> attachments = event.msg.attachments;
    const std::map<std::string, std::string> keywords = config.at( "keyWords" );
    const std::map<std::string, std::string> keyWordsFiles = config.at( "keyWordsFiles" );

    // React with an emoji to all attachments in the specified channel
    if ( channel.id == config.at( "specialChannel" ).get<dpp::snowflake>() && !attachments.empty() ) {
      bot.message_add_reaction( msg, config.at( "specialChannelEmote" ).get<std::string>(), logCallback );
    }

    // Check for custom keywords and reply with the corresponding response
    for ( const auto &keyword : keywords ) {
      if ( content.find( keyword.first ) != std::string::npos ) {
        event.reply( dpp::message( keyword.second ), true, logCallback );
      }
    }

    // Check for custom keywords with file responses and reply with the corresponding file
    for ( const auto &keyWordFile : keyWordsFiles ) {
      if ( content.find( keyWordFile.first ) != std::string::npos ) {
        std::ifstream f( "../media/" + keyWordFile.second );
        if ( f ) {
          // typing indicator coroutine
          bot.channel_typing( channel );

          // Read the file size
          f.seekg( 0, std::ios::end );
          std::streamsize length = f.tellg();
          f.seekg( 0, std::ios::beg );

          // Read the file content
          std::vector<char> buffer( length );
          if ( f.read( buffer.data(), length ) ) {
            std::string fileContent( buffer.begin(), buffer.end() );

            // Reply with the file
            event.reply( dpp::message().add_file( keyWordFile.second, fileContent ), true, logCallback );
            f.close();
          }
        }
      }
    }

    // Holy hell easter egg
    if ( content.find( "holy hell" ) != std::string::npos ) {
      std::vector<std::string> arr = { "New Response just dropped",
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
                                       "Holy bishops on skateboards" };

      // Reply with a random sentence
      dpp::message msg_ = dpp::message().set_reference( msg.id ).set_channel_id( channel.id );
      msg_.allowed_mentions.replied_user = true;

      // Reply with a random sentence and set the reference to the sent message
      for ( const auto &sentence : arr ) {
        msg_.set_reference( bot.message_create_sync( msg_.set_content( sentence ) ).id );

        // Sleep for 1 second (rate limit)
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
      }
    }
  } );

  // Execute slash command if it's allowed in the channel
  bot.on_slashcommand( [ &bot, &commands ]( const dpp::slashcommand_t &event ) {
    for ( const auto &command : commands ) {
      if ( event.command.get_command_name() == command->get_name() ) {

        // Get channel ID and check if it's allowed
        dpp::snowflake channel_id = event.command.channel_id;
        json config = bot.get_config();
        std::vector<dpp::snowflake> channel_ids = config.at( "botChannels" );
        if ( std::find( channel_ids.begin(), channel_ids.end(), channel_id ) != channel_ids.end() ) {
          // Execute the command if it's allowed
          command->execute( bot, event );
        } else {
          // Send an ephemeral message if it's not allowed
          event.reply( dpp::message( "No." ).set_flags( dpp::m_ephemeral ) );
        }
        break;
      }
    }
  } );

  // Handle message reaction add
  bot.on_message_reaction_add(
      [ &bot ]( const dpp::message_reaction_add_t &event ) { updateStarboardMessage( bot, event ); } );

  // Handle message reaction remove
  bot.on_message_reaction_remove(
      [ &bot ]( const dpp::message_reaction_remove_t &event ) { updateStarboardMessage( bot, event ); } );

  // Start bot
  bot.start( dpp::st_wait );
}
