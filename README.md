# brain-damage
A discord bot written in C++ using the [dpp](https://github.com/brainboxdotcc/DPP) library.

## Building

The bot is built using CMake. To build the bot, run the following commands in the root directory of the repository:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

## Configuring

The bot requires a configuration file in order to run. The configuration file should be in JSON format and contain the following fields:

* `token`: The bot's token.
* `keyWords`: A dictionary of keywords and their corresponding responses.
* `keyWordsFiles`: A dictionary of keywords and their corresponding file responses.

The configuration file is loaded from the file `../config.json` relative to the `build` directory.

## Running

To run the bot, use the following command from within the `build` directory:

```sh
./discord-bot
```

The bot will automatically load the configuration file and start up.

## Commands

The bot supports the following commands:

* `keyword <keyword> <response>`: Adds a keyword and its corresponding response to the bot's configuration.
* `keywordfile <keyword> <file>`: Adds a keyword and its corresponding file response to the bot's configuration.
* `reload`: Reloads the bot's configuration from the configuration file.
* `ping`: Responds with "Pong!".

## Command Loading

The bot supports commands as dynamically loaded libraries that can be used to extend the bot's functionality. Commands are loaded from the `build/commands` directory.

Commands in `src/commands` are constructed like this:
```cpp
/* src/commands/<command_name>/<command_name>.cpp */
#include "../command.hpp"
#include "../../main.hpp"

class CommandClass : public Command {
public:
  void execute(custom_cluster &bot, const dpp::slashcommand_t &event) override {
    /*
     * Your command code here
     */
  }

  // The name of the command
  std::string get_name() const override { return "name"; }

  // The description of the command
  std::string get_description() const override { return "Ping Pong!"; }

  // The options of the command
  std::vector<dpp::command_option> get_options() const override {
    return {
      dpp::command_option(dpp::co_string, "example", "Example option", false),
    };
  }

  // The permissions required to use the command
  dpp::permission get_permissions() const override { return dpp::p_use_application_commands; }
}

// Export
extern "C" {
  Command *create_name_command() { return new CommandClass(); }
}
```

Commands should export a single function, `create_<command_name>_command`, that returns a pointer to a `Command` class.
