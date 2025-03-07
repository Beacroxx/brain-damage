#define VERBOSE_DEBUG

#include "main.hpp"

#include "commands/command.hpp"
#include "starboard.hpp"

#include <boost/asio.hpp>
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

#ifdef VERBOSE_DEBUG
#define LOG_DEBUG( msg ) std::cout << "[DEBUG] " << msg << std::endl
#else
#define LOG_DEBUG( msg )
#endif

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

void deleteAfterAsync( custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds ) {
  LOG_DEBUG( "Scheduling message deletion in " + std::to_string( seconds ) + " seconds" );
  std::thread( [ &bot, msgid, channelid, seconds ]() {
    LOG_DEBUG( "Starting deleteAfterAsync thread" );
    std::this_thread::sleep_for( std::chrono::seconds( seconds ) );
    LOG_DEBUG( "Deleting message after " + std::to_string( seconds ) + " seconds" );
    bot.message_delete( msgid, channelid, logCallback );
  } ).detach();
}

void log_websocket_message( const std::string &raw_message ) {
  try {
    // Parse the JSON message
    auto json_msg = nlohmann::json::parse( raw_message.substr( 3 ) ); // Skip "W: "

    // Extract opcode and data
    int opcode = json_msg[ "op" ];
    auto data = json_msg[ "d" ];

    // Mask sensitive fields
    if ( data.contains( "token" ) ) {
      data[ "token" ] = "*****";
    }

    // Log based on opcode
    switch ( opcode ) {
    case 1: // Heartbeat
      LOG_DEBUG( "[Heartbeat] Sequence: " + std::to_string( data.get<int>() ) );
      break;
    case 2:                                        // Identify
      LOG_DEBUG( "[Identify] " + data.dump( 4 ) ); // Pretty-print JSON with indentation
      break;
    case 3: // Presence Update
      LOG_DEBUG( "[Presence Update] Status: " + data[ "status" ].get<std::string>() );
      break;
    default:
      LOG_DEBUG( "[Unhandled Opcode] " + json_msg.dump( 4 ) );
      break;
    }
  } catch ( const std::exception &e ) {
    std::cerr << "[ERROR] Failed to parse WebSocket message: " << e.what() << std::endl;
  }
}

int main() {
  LOG_DEBUG( "Initializing signal handler" );
  std::signal( SIGINT, signalHandler );

  LOG_DEBUG( "Loading config" );
  std::fstream cfg_fstream;
  cfg_fstream.open( "../config.json", std::fstream::in );
  if ( !cfg_fstream.is_open() ) {
    std::cerr << "Failed to open config.json" << std::endl;
    return 1;
  }
  json config = json::parse( cfg_fstream );
  cfg_fstream.close();

  LOG_DEBUG( "Initializing bot" );
  custom_cluster bot( config.at( "token" ), dpp::intents::i_all_intents );
  bot.load_config();
  bot.on_log( dpp::utility::cout_logger() );

  LOG_DEBUG( "Loading commands" );
  std::vector<std::unique_ptr<Command>> commands;
  // Iterate over all .so files in the commands/ directory
  for ( const auto &file : fs::directory_iterator( "./commands/" ) ) {
    if ( file.is_regular_file() && file.path().extension() == ".so" ) {
      LOG_DEBUG( "Loading .so file: " + file.path().string() );
      // Load the .so file
      void *lib_handle = dlopen( file.path().c_str(), RTLD_LAZY );
      if ( lib_handle ) {
        typedef Command *( *create_command_func )();
        // Assume the function name is "create_<cmd_name>_command"
        std::string command_name = file.path().stem().string();
        std::string func_name = "create_" + command_name + "_command";
        func_name.erase( 7, 3 );
        create_command_func create_command = (create_command_func)dlsym( lib_handle, func_name.c_str() );
        if ( create_command ) {
          commands.push_back( std::unique_ptr<Command>( create_command() ) );
          LOG_DEBUG( "Loaded command: " + command_name );
        } else {
          std::cerr << "Error loading command from library: " << dlerror() << std::endl;
        }
      } else {
        std::cerr << "Error opening library: " << dlerror() << std::endl;
      }
    }
  }

  bot.on_log( [ &bot ]( const dpp::log_t &event ) {
    if ( event.severity == dpp::loglevel::ll_error ) {
      std::cerr << "Error: " << event.message << std::endl;
    } else if ( event.message.rfind( "W:", 0 ) == 0 ) { // Check if the log starts with "W:"
      log_websocket_message( event.message );
    } else {
      LOG_DEBUG( event.message );
    }
  } );

  bot.on_socket_close( [ &bot ]( const dpp::socket_close_t &event ) {
    LOG_DEBUG( "Socket closed: " + std::to_string( event.from()->shard_id ) );
  } );

  // Bot ready event
  bot.on_ready( [ &bot, &commands ]( const dpp::ready_t &event ) -> dpp::task<void> {
    LOG_DEBUG( "Bot ready event triggered" );
    // Cast event to void to avoid unused variable warning
    (void)event;

    // Get the guild ID
    dpp::snowflake guild_id = bot.get_config().at( "guildId" ).get<dpp::snowflake>();
    LOG_DEBUG( "Deleting all commands in the guild" );
    // Delete all commands in the guild
    co_await bot.co_guild_bulk_command_delete( guild_id );

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

    LOG_DEBUG( "Creating all slash commands in the guild" );
    // Create all slash commands in the guild
    co_await bot.co_guild_bulk_command_create( scommands, guild_id );

    // Set the bot's status
    dpp::activity activity;
    activity.type = dpp::activity_type::at_game;
    activity.name = "with fire";

    bot.set_presence( dpp::presence( dpp::presence_status::ps_online, activity ) );
  } );

  bot.on_message_create( [ &bot ]( const dpp::message_create_t &event ) -> dpp::task<void> {
    // Get the author, message, channel and config
    const dpp::user author = event.msg.author;
    const dpp::message msg = event.msg;
    auto channel_result = co_await bot.co_channel_get( event.msg.channel_id );
    const dpp::channel channel = std::get<dpp::channel>( channel_result.value );
    json config = bot.get_config();
    const std::vector<dpp::snowflake> botChannels = config.at( "botChannels" );

    // DPP regex downloading
    auto regex = std::regex(
        R"(\b((?:https?:)?\/\/)?((?:www|m)\.)?((?:youtube\.com|youtu\.be|vimeo\.com|dailymotion\.com|twitch\.tv|facebook\.com|instagram\.com)\/(?:[\w\-]+\?v=|embed\/|v\/|clip\/)?)([\w\-]+)(\S+)?\b)",
        std::regex::icase );

    std::smatch match;
    if ( std::regex_search( event.msg.content, match, regex ) ) {
      LOG_DEBUG( "URL detected in message" );

      // Start typing indicator thread
      std::atomic<bool> typing_indicator_running( true );
      std::thread typing_indicator_thread( [ &bot, &channel, &typing_indicator_running ]() {
        LOG_DEBUG( "Starting typing indicator thread" );
        while ( typing_indicator_running ) {
          bot.channel_typing( channel );
          std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        }
      } );

      std::string url = event.msg.content.substr( match.position(), match.length() );

      std::string simcommand = "yt-dlp --simulate --match-filter \"duration<=300\" --max-filesize 8M -f "
                               "\"bestvideo[ext=mp4][vcodec^=avc1]+bestaudio[ext=m4a]/best[ext=mp4]/best\" " +
                               url + " -o \"../media//ytdlp/output.mp4\" 2>&1";
      char buffer[ 2048 ];
      std::string result = "";
      FILE *pipe = popen( simcommand.c_str(), "r" );
      if ( !pipe ) {
        LOG_DEBUG( "Failed to open pipe for yt-dlp simulation command" );
        typing_indicator_running = false;
        typing_indicator_thread.join();
        co_return;
      }
      while ( !feof( pipe ) ) {
        if ( fgets( buffer, 2048, pipe ) != NULL ) {
          result += buffer;
        }
      }
      pclose( pipe );

      if ( result.find( "skipping" ) != std::string::npos ) {
        LOG_DEBUG( "Video too long or too large" );
        typing_indicator_running = false;
        typing_indicator_thread.join();
        auto msg = dpp::message( "The video is too long or too large." ).set_channel_id( event.msg.channel_id );
        bot.message_create( msg, [ &bot ]( const dpp::confirmation_callback_t &callback ) {
          if ( !callback.is_error() ) {
            const dpp::message &created_msg = std::get<dpp::message>( callback.value );
            deleteAfterAsync( bot, created_msg.id, created_msg.channel_id, 10 );
          }
        } );
        co_return;
      }

      if ( result.find( "ERROR:" ) != std::string::npos ) {
        LOG_DEBUG( "Error downloading video" );
        typing_indicator_running = false;
        typing_indicator_thread.join();
        auto msg = dpp::message( "Couldn't download the video. Make sure the link is valid and contains a video" )
                       .set_channel_id( event.msg.channel_id );
        bot.message_create( msg, [ &bot ]( const dpp::confirmation_callback_t &callback ) {
          if ( !callback.is_error() ) {
            const dpp::message &created_msg = std::get<dpp::message>( callback.value );
            deleteAfterAsync( bot, created_msg.id, created_msg.channel_id, 10 );
          }
        } );
        co_return;
      }

      std::string command = "yt-dlp --match-filter \"duration<=300\" --max-filesize 8M -f "
                            "\"bestvideo[ext=mp4][vcodec^=avc1]+bestaudio[ext=m4a]/best[ext=mp4]/best\" " +
                            url + " -o \"../media//ytdlp/output.%(ext)s\"";
      pipe = popen( command.c_str(), "r" );
      if ( !pipe ) {
        LOG_DEBUG( "Failed to open pipe for yt-dlp command" );
        typing_indicator_running = false;
        typing_indicator_thread.join();
        co_return;
      }
      while ( !feof( pipe ) ) {
        if ( fgets( buffer, 2048, pipe ) != NULL ) {
          result += buffer;
        }
      }
      pclose( pipe );

      // find the file
      std::string path;
      for ( const auto &entry : fs::directory_iterator( "../media/ytdlp/" ) ) {
        if ( entry.is_regular_file() && entry.path().filename().string().find( "output" ) != std::string::npos ) {
          path = entry.path().string();
          break;
        }
      }

      if ( path.empty() ) {
        LOG_DEBUG( "Downloaded video file not found" );
        typing_indicator_running = false;
        typing_indicator_thread.join();

        auto msg = dpp::message( "The video dissapeared :)" ).set_channel_id( event.msg.channel_id );
        bot.message_create( msg, [ &bot ]( const dpp::confirmation_callback_t &callback ) {
          if ( !callback.is_error() ) {
            const dpp::message &created_msg = std::get<dpp::message>( callback.value );
            deleteAfterAsync( bot, created_msg.id, created_msg.channel_id, 10 );
          }
        } );
        co_return;
      }

      // Reply with the file
      std::ifstream f( path );
      if ( f ) {

        // Read the file size
        f.seekg( 0, std::ios::end );
        std::streamsize length = f.tellg();
        f.seekg( 0, std::ios::beg );

        // Read the file content
        std::vector<char> buffer( length );
        if ( f.read( buffer.data(), length ) ) {
          std::string fileContent( buffer.begin(), buffer.end() );

          try {
            event.reply( dpp::message().add_file( path, fileContent, "video" ), true, logCallback );
          } catch ( const std::exception &e ) {
            auto msg = dpp::message( "Video too fat" ).set_channel_id( event.msg.channel_id );
            bot.message_create( msg, [ &bot ]( const dpp::confirmation_callback_t &callback ) {
              if ( !callback.is_error() ) {
                const dpp::message &created_msg = std::get<dpp::message>( callback.value );
                deleteAfterAsync( bot, created_msg.id, created_msg.channel_id, 10 );
              }
            } );
          }
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
      co_return;

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

        auto response = co_await bot.co_message_create( msg_.set_content( sentence ) );
        msg_.set_reference( std::get<dpp::message>( response.value ).id );

        // Sleep for 1 second (rate limit)
        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
      }
    }
  } );

  // Execute slash command if it's allowed in the channel
  bot.on_slashcommand( [ &bot, &commands ]( const dpp::slashcommand_t &event ) {
    for ( const auto &command : commands ) {
      if ( event.command.get_command_name() == command->get_name() ) {
        LOG_DEBUG( "Executing slash command: " + command->get_name() );

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
  bot.on_message_reaction_add( [ &bot ]( const dpp::message_reaction_add_t &event ) -> dpp::task<void> {
    LOG_DEBUG( "Message reaction add event triggered" );
    co_await updateStarboardMessage( bot, event );
  } );

  // Handle message reaction remove
  bot.on_message_reaction_remove( [ &bot ]( const dpp::message_reaction_remove_t &event ) -> dpp::task<void> {
    LOG_DEBUG( "Message reaction remove event triggered" );
    co_await updateStarboardMessage( bot, event );
  } );

  LOG_DEBUG( "Starting bot" );
  // Start bot
  bot.start( dpp::st_wait );
}
