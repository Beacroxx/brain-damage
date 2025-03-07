#include "main.hpp"
#include "commands/command.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>
#include <vector>

#ifdef VERBOSE_DEBUG
#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

// Function declarations
void logCallback(const dpp::confirmation_callback_t callback);
void deleteAfterAsync(custom_cluster &bot, dpp::snowflake msgid, dpp::snowflake channelid, int seconds);
void ready(custom_cluster &bot, const dpp::ready_t &event, const std::vector<std::unique_ptr<Command>> &commands);
void messageCreate(custom_cluster &bot, const dpp::message_create_t &event);