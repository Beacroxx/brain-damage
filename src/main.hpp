#pragma once
#include <dpp/dpp.h>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class custom_cluster : public dpp::cluster {
public:
  using dpp::cluster::cluster; // Inherit constructors

  void load_config() {
    std::ifstream cfg_ifstream( "../config.json" );
    cfg = json::parse( cfg_ifstream );
  }
  void save_config( json config ) {
    std::ofstream cfg_ofstream( "../config.json", std::ofstream::out | std::ofstream::trunc );
    cfg_ofstream << config.dump( 2 );
    cfg = config;
  }
  json get_config() { return cfg; }

  std::unordered_map<std::string, dpp::message> starboard;
  std::mutex starboard_mutex;
  std::unordered_map<std::string, std::shared_ptr<std::thread>> starboard_threads;

protected:
  json cfg;
};
