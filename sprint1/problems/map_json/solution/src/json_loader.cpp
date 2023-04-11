#include "json_loader.h"

#include <fstream>
#include <exception>
#include <iostream>
namespace json_loader {
namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    std::ifstream json_file ( json_path );
    std::stringstream buffer;
    buffer << json_file.rdbuf();
    json_file.close();

    // Распарсить строку как JSON,
    model::Game game;
    auto json_maps = json::parse( buffer.str() );

   for( auto map_json : json_maps.at("maps").as_array() )
   {

      auto obj_map = map_json.as_object();

      //id, name
      model::Map::Id id ( json::value_to<std::string> (obj_map.at("id") ) );
      std::string map_name = json::value_to<std::string> (obj_map.at("name") );

      model::Map map ( id, map_name );

      //roads
      std::list<model::Road> roadList = LoadRoads( map_json.as_object() );
      for( auto road : roadList )
      {
          map.AddRoad( road );
      }

      //buildings
      std::list<model::Building> buildingList = LoadBildings( map_json.as_object() );
      for( auto building : buildingList )
      {
          map.AddBuilding( building );
      }

      //offices

      std::list<model::Office> officeList = LoadOffices( map_json.as_object() );
      for( auto office : officeList )
      {
          map.AddOffice( office );
      }

       game.AddMap( map );
   }

    // Загрузить модель игры из файла


   return game;
}
namespace  { //private namespace
std::list<model::Road> LoadRoads(const json::object & map)
{
    std::list<model::Road> roadList;
    for( auto json_road : map.at("roads").as_array() )
    {

      auto obj_road = json_road.as_object();
      model::Coord x0 = obj_road.at("x0").as_int64();
      model::Coord y0 = obj_road.at("y0").as_int64();
      int icoord;
      bool isVertical = false;
      obj_road.if_contains("x1");
      if( obj_road.if_contains("x1") )
      {
          icoord =  ( obj_road.at("x1").as_int64() );
          isVertical = false;
      }
      else if( obj_road.if_contains("y1") )
      {
          icoord = obj_road.at("y1").as_int64();
          isVertical = true;
      }
      else
      {
          //throw exception
      }
      model::Coord mCoord ( icoord );
      model::Point point { x0, y0 };
      if( isVertical )
      {
      model::Road road ( model::Road::VERTICAL , point, mCoord );
      roadList.emplace_back( road );
      }
      else
      {
      model::Road road ( model::Road::HORIZONTAL , point, mCoord );
      roadList.emplace_back( road );
      }

    }
    return roadList;
}

std::list<model::Building> LoadBildings(const json::object &map)
{

    std::list<model::Building> builingList;
    if( !map.if_contains( "buildings" ) )
    {
        return builingList;
    }
    for( auto json_building : map.at("buildings").as_array() )
    {
      auto obj_road = json_building.as_object();
      model::Coord x = obj_road.at("x").as_int64();
      model::Coord y = obj_road.at("y").as_int64();
      model::Dimension w = obj_road.at("w").as_int64();
      model::Dimension h = obj_road.at("h").as_int64();

      model::Rectangle rect { model::Point{x,y}, model::Size {w,h} };
      model::Building building (rect);
      builingList.emplace_back( building );

    }
    return builingList;
}

std::list<model::Office> LoadOffices ( const json::object & map )
{
    std::list<model::Office> officeList;
    if( !map.if_contains( "offices" ) )
    {
        return officeList;
    }
    for( auto json_office : map.at("offices").as_array() )
    {
      auto obj_office = json_office.as_object();
      model::Office::Id id ( json::value_to<std::string> ( obj_office.at("id") ) );

      model::Coord x = obj_office.at("x").as_int64();
      model::Coord y = obj_office.at("y").as_int64();


      model::Dimension dx = obj_office.at("offsetX").as_int64();
      model::Dimension dy = obj_office.at("offsetY").as_int64();

      model::Office office (id, {x,y}, {dx, dy} );

      officeList.emplace_back( office );

    }
    return officeList;
}
}//private namespace

}  // namespace json_loader
