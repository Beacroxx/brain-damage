#define VERBOSE_DEBUG

#include "main.hpp"

#include "events.hpp"

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

// Exit gracefully on signal
void signalHandler( int signal ) {
  std::cout << "Caught signal " << signal << ", exiting..." << std::endl;
  exit( 0 );
}

void callAsThread( std::function<void()> func ) {
  std::thread( func ).detach();
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
      // LOG_DEBUG( "[Heartbeat] Sequence: " + std::to_string( data.get<int>() ) );
      break;
    case 2:                                        // Identify
      LOG_DEBUG( "[Identify] " + data.dump( 4 ) ); // Pretty-print JSON with indentation
      break;
    case 3: // Presence Update
      LOG_DEBUG( "[Presence Update] Status: " + data[ "status" ].get<std::string>() );
      break;
    case 8: // REQUEST_GUILD_MEMBERS
      LOG_DEBUG( "[Request Guild Members] Guild ID: " + data[ "guild_id" ].get<std::string>() +
                 ", Limit: " + std::to_string( data[ "limit" ].get<int>() ) +
                 ", Presences: " + ( data[ "presences" ].get<bool>() ? "true" : "false" ) + ", Query: '" +
                 data[ "query" ].get<std::string>() + "'" );
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

  // Bot ready event
  bot.on_ready( [ &bot, &commands ]( const dpp::ready_t &event ) {
    callAsThread( [ &bot, &commands, event ]() { ready( bot, event, commands ); } );
  } );

  bot.on_message_create( [ &bot ]( const dpp::message_create_t &event ) {
    callAsThread( [ &bot, event ]() { messageCreate( bot, event ); } );
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
          callAsThread( [ &bot, event, &command ]() { command->execute( bot, event ); } );
        } else {
          // Send an ephemeral message if it's not allowed
          event.reply( dpp::message( "No." ).set_flags( dpp::m_ephemeral ) );
        }
        break;
      }
    }
  } );

  // Handle message reaction add
  bot.on_message_reaction_add( [ &bot ]( const dpp::message_reaction_add_t &event ) {
    LOG_DEBUG( "Message reaction add event triggered" );
    callAsThread( [ &bot, event ]() { updateStarboardMessage( bot, event ); } );
  } );

  // Handle message reaction remove
  bot.on_message_reaction_remove( [ &bot ]( const dpp::message_reaction_remove_t &event ) {
    LOG_DEBUG( "Message reaction remove event triggered" );
    callAsThread( [ &bot, event ]() { updateStarboardMessage( bot, event ); } );
  } );

  LOG_DEBUG( "Starting bot" );
  // Start bot
  bot.start( dpp::st_wait );
}
