#include "command.hpp"
#include <dpp/dpp.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <cstdlib>
#include <future>
#include <unordered_map>

class ExecuteCommand : public Command {
private:
  const dpp::snowflake ALLOWED_USER_ID = 539322589391093780;
  // Map of user ID to pair of (channel ID, execution type)
  static inline std::unordered_map<dpp::snowflake, std::pair<dpp::snowflake, std::string>> awaiting_code;
  
  std::string generate_random_filename() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);
    static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    
    std::string filename = "temp_";
    for(int i = 0; i < 10; i++) {
      filename += charset[dis(gen)];
    }
    return filename;
  }

  std::string compile_and_run_cpp(const std::string& code) {
    std::string filename = generate_random_filename();
    std::string cpp_file = filename + ".cpp";
    std::string exe_file = "./" + filename;
    
    // Write code to file
    std::ofstream file(cpp_file);
    file << code;
    file.close();
    
    // Compile with a timeout and restricted permissions
    std::string compile_cmd = "g++ -o " + filename + " " + cpp_file + " -std=c++20 2>&1";
    std::string compile_output = execute_command(compile_cmd);
    
    if(std::filesystem::exists(filename)) {
      // Run the compiled program with restrictions
      std::string run_cmd = "timeout 5s " + exe_file + " 2>&1";
      std::string output = execute_command(run_cmd);
      
      // Cleanup
      std::filesystem::remove(cpp_file);
      std::filesystem::remove(filename);
      
      return "Compilation successful.\nOutput:\n" + output;
    } else {
      std::filesystem::remove(cpp_file);
      return "Compilation failed:\n" + compile_output;
    }
  }
  
  std::string execute_command(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) {
      return "Failed to execute command.";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    
    return result;
  }

public:
  void execute(custom_cluster& bot, const dpp::slashcommand_t& event) override {
    if(event.command.usr.id != ALLOWED_USER_ID) {
      event.reply(dpp::message("You do not have permission to use this command.").set_flags(dpp::m_ephemeral));
      return;
    }

    auto options = event.command.get_command_interaction().options;
    std::string type = std::get<std::string>(options[0].value);
    
    if(options.size() > 1) {
      // Direct input provided
      std::string input = std::get<std::string>(options[1].value);
      
      if(type == "shell") {
        std::string output = execute_command(input);
        event.reply(dpp::message("```\n" + output + "\n```"));
      } else if(type == "cpp") {
        std::string output = compile_and_run_cpp(input);
        event.reply(dpp::message("```\n" + output + "\n```"));
      }
    } else {
      // Wait for message with code block
      awaiting_code[event.command.usr.id] = std::make_pair(event.command.channel_id, type);
      event.reply(dpp::message("Please send your " + type + " code in a code block in this channel. You have 60 seconds.").set_flags(dpp::m_ephemeral));
      
      // Set a timer to remove the user from awaiting_code after 60 seconds
      std::thread([user_id = event.command.usr.id]() {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        awaiting_code.erase(user_id);
      }).detach();
      
      bot.on_message_create([this, &bot](const dpp::message_create_t& msg_event) {
        auto it = awaiting_code.find(msg_event.msg.author.id);
        if(it == awaiting_code.end()) return;
        
        // Check if message is in the same channel as the command
        if(msg_event.msg.channel_id != it->second.first) return;
        
        std::string content = msg_event.msg.content;
        if(content.find("```") == std::string::npos) return;
        
        // Extract code from code block
        std::string code = content.substr(content.find("```") + 3);
        if(code.find("```") != std::string::npos) {
          code = code.substr(0, code.find("```"));
        }
        
        // Remove language identifier if present
        if(code.find("\n") != std::string::npos) {
          code = code.substr(code.find("\n") + 1);
        }
        
        std::string output;
        std::string type = it->second.second;
        if(type == "shell") {
          output = execute_command(code);
        } else if(type == "cpp") {
          output = compile_and_run_cpp(code);
        }
        
        // Remove user from awaiting list after processing
        awaiting_code.erase(it);
        
        msg_event.reply(dpp::message("```\n" + output + "\n```"));
      });
    }
  }

  std::string get_name() const override {
    return "execute";
  }

  std::string get_description() const override {
    return "Execute shell commands or C++ code (restricted access)";
  }

  std::vector<dpp::command_option> get_options() const override {
    return {
      dpp::command_option(dpp::co_string, "type", "Type of execution (shell/cpp)", true)
        .add_choice(dpp::command_option_choice("Shell Command", std::string("shell")))
        .add_choice(dpp::command_option_choice("C++ Code", std::string("cpp"))),
      dpp::command_option(dpp::co_string, "input", "Command or code to execute", false)
    };
  }

  dpp::permission get_permissions() const override {
    return dpp::p_use_application_commands;
  }
};

// Register the command
#include "commands_registry.hpp"
REGISTER_COMMAND(ExecuteCommand); 