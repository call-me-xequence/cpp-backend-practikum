#include "request_handler.h"
#include <boost/json.hpp>
namespace http_handler {
namespace json = boost::json;
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type )
{
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}
StringResponse JsonErrorResponse( http::status status, std::string  code, std::string  message,
                                  unsigned int http_version, bool keep_alive )
{
    json::object response_json;
    response_json["code"] = code;
    response_json["message"] = message;
    std::stringstream ss;
    ss << response_json;

    StringResponse response = MakeStringResponse( status, ss.str(), http_version, keep_alive, ContentType::APP_JSON );

    return response;
}
///////////////////
/// class RequestHandler
///////////////////
StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };
    StringResponse response;
    response.version( req.version() );
    response.keep_alive( req.keep_alive() );


    if( req.method() == http::verb::get )
    {
        if( req.target().starts_with("/api/") )
        {
            response = MakeResponseForAPI( req );
        }
        else  response = text_response(http::status::not_found , "Not found" );

    }
    else if ( req.method() == http::verb::head )
    {
        response.result( http::status::ok );
    }
    else
    {
        response = text_response(http::status::method_not_allowed, "Invalid method" );
    }

    return response;
}
StringResponse RequestHandler::MakeResponseForAPI( StringRequest& req )
{
    std::string_view target_sv = req.target();
    std::string target_str(target_sv.begin(), target_sv.end() );
    if( target_str.ends_with('/') )
        target_str.pop_back();

    StringResponse response;
    if( target_str == "/api/v1/maps")
     response = ResponseForApiMaps( req );
    else if( target_str.starts_with("/api/v1/maps/") )
    {
        response = ResponseForApiMapById ( req );
    }
    else
    {
        response = JsonErrorResponse( http::status::bad_request, "badRequest", "Bad request", req.version(), req.keep_alive() );
    }

    return response;
}
StringResponse RequestHandler::ResponseForApiMaps ( StringRequest& req ){

    json::array mapsArray;
    for( auto& map : game_.GetMaps() )
    {
        json::object mapObj;
        mapObj["id"] = *map.GetId();
        mapObj["name"] = map.GetName();
        mapsArray.emplace_back ( mapObj );
    }

    std::stringstream ss ;
    ss << mapsArray;
    return MakeStringResponse( http::status::ok, ss.str() , req.version(), req.keep_alive(), ContentType::APP_JSON );
}

StringResponse RequestHandler::ResponseForApiMapById ( StringRequest& req ){
    size_t idx = "/api/v1/maps/"sv.size();

    std::string_view id_sv  = req.target().substr(idx);
    std::string id( id_sv.begin(), id_sv.end() );

    const model::Map* map = game_.FindMap( model::Map::Id(id) );
    if( !map )
    {
        return JsonErrorResponse( http::status::not_found, "mapNotFound", "Map not found", req.version(), req.keep_alive() );
    }

    json::object map_json;
    map_json ["id"] = *(map->GetId());
    map_json ["name"] = map->GetName();

    //roads
    json::array roads_json;
    json::array buildings_json;
    json::array offices_json;

    for( auto & road : map->GetRoads() )
    {
        json::object road_obj;
        road_obj["x0"] = road.GetStart().x;
        road_obj["y0"] = road.GetStart().y;
        if( road.IsHorizontal() ){
            road_obj["x1"] = road.GetEnd().x;
        }
            else{
            road_obj["y1"] = road.GetEnd().y;
        }

        roads_json.emplace_back (road_obj);
    }

    //buildings
    for( auto & building :map->GetBuildings() ){
        json::object building_ojb;
        building_ojb["x"] = building.GetBounds().position.x;
        building_ojb["y"] = building.GetBounds().position.y;
        building_ojb["w"] = building.GetBounds().size.width;
        building_ojb["h"] = building.GetBounds().size.height;

        buildings_json.emplace_back ( building_ojb );
    }

    for( auto & office : map->GetOffices() ) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;

        offices_json.emplace_back( office_obj );

    }

    if( !map->GetRoads().empty() )
        map_json.emplace( "roads", roads_json );
    if( !map->GetBuildings().empty() )
        map_json.emplace( "buildings", buildings_json );
    if( !map->GetOffices().empty() )
        map_json.emplace( "offices", offices_json );

    std::stringstream ss;
    ss << map_json;
    return MakeStringResponse( http::status::ok, ss.str() , req.version(), req.keep_alive(), ContentType::APP_JSON );

}
}  // namespace http_handler
