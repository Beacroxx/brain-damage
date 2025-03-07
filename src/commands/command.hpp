#pragma once
#include <dpp/dpp.h>
#include "../main.hpp"
#include <string>

class Command {
public:
  virtual void execute(custom_cluster &bot, const dpp::slashcommand_t &event) = 0;
  virtual std::string get_name() const = 0;
  virtual std::string get_description() const = 0;
  virtual std::vector<dpp::command_option> get_options() const = 0;
  virtual dpp::permission get_permissions() const = 0;
};
