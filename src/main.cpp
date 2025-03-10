#define VERBOSE_DEBUG

#include "main.hpp"
#include "events/event.hpp"
#include "events/events_registry.hpp"
#include "commands/command.hpp"
#include "commands/commands_registry.hpp"
#include "starboard.hpp"

#include <boost/asio.hpp>
#include <concepts>
#include <csignal>
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

// Log errors from DPP
void logCallback( const dpp::confirmation_callback_t callback ) {
  if ( callback.is_error() ) {
    std::cerr << "Error: " << callback.get_error().human_readable << std::endl;
  }
}

void deleteAfterAsync( custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds ) {
  LOG_DEBUG( "Scheduling message deletion in " + std::to_string( seconds ) + " seconds" );
  std::thread( [&bot, msgid, channelid, seconds]() {
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
  std::ifstream cfg_fstream;
  cfg_fstream.open( "../config.json", std::fstream::in );
  if ( !cfg_fstream.is_open() ) {
    std::cerr << "Failed to open config.json" << std::endl;
    return 1;
  }
  json config = json::parse( cfg_fstream );
  cfg_fstream.close();

  custom_cluster bot( config.at( "token" ), dpp::intents::i_all_intents );
  bot.load_config();
  bot.on_log( dpp::utility::cout_logger() );

  LOG_DEBUG( "Loading commands" );
  // Get commands from the command registry
  std::vector<std::unique_ptr<Command>> commands = CommandRegistry::instance().create_all_commands();
  LOG_DEBUG( "Loaded " + std::to_string(commands.size()) + " commands" );

  LOG_DEBUG( "Loading events" );
  // Get events from the event registry
  std::vector<std::unique_ptr<Event>> events = EventRegistry::instance().create_all_events();
  LOG_DEBUG( "Loaded " + std::to_string(events.size()) + " events" );

  bot.on_log( [ &bot ]( const dpp::log_t &event ) {
    if ( event.severity == dpp::loglevel::ll_error ) {
      std::cerr << "Error: " << event.message << std::endl;
    } else if ( event.message.rfind( "W:", 0 ) == 0 ) { // Check if the log starts with "W:"
      log_websocket_message( event.message );
    } else {
      LOG_DEBUG( event.message );
    }
  } );

  // Set up event handlers
  bot.on_ready( [ &bot, &events ]( const dpp::ready_t &event ) {
    for ( const auto &e : events ) {
      if ( e->get_name() == "ready" ) {
        callAsThread( [ &bot, &e, event ]() { e->execute( bot, event ); } );
        break;
      }
    }
  } );

  bot.on_message_create( [ &bot, &events ]( const dpp::message_create_t &event ) {
    for ( const auto &e : events ) {
      if ( e->get_name() == "message_create" ) {
        callAsThread( [ &bot, &e, event ]() { e->execute( bot, event ); } );
        break;
      }
    }
  } );

  bot.on_message_reaction_add( [ &bot, &events ]( const dpp::message_reaction_add_t &event ) {
    for ( const auto &e : events ) {
      if ( e->get_name() == "reaction" ) {
        callAsThread( [ &bot, &e, event ]() { e->execute( bot, event ); } );
        break;
      }
    }
  } );

  bot.on_message_reaction_remove( [ &bot, &events ]( const dpp::message_reaction_remove_t &event ) {
    for ( const auto &e : events ) {
      if ( e->get_name() == "reaction" ) {
        callAsThread( [ &bot, &e, event ]() { e->execute( bot, event ); } );
        break;
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
          callAsThread( [ &bot, event, &command ]() { command->execute( bot, event ); } );
        } else {
          // Send an ephemeral message if it's not allowed
          event.reply( dpp::message( "No." ).set_flags( dpp::m_ephemeral ) );
        }
        break;
      }
    }
  } );

  LOG_DEBUG( "Starting bot" );
  // Start bot
  bot.start( dpp::st_wait );
}
