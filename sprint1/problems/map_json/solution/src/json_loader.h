#pragma once

#include <filesystem>

#include "model.h"
#include <boost/json.hpp>
#include <list>
namespace json_loader {
namespace json = boost::json;
model::Game LoadGame(const std::filesystem::path& json_path);

//private functions
namespace  {
std::list<model::Road> LoadRoads( const json::object & map );
std::list<model::Building> LoadBildings( const json::object & map );
std::list<model::Office> LoadOffices ( const json::object & map );

}

}  // namespace json_loader
