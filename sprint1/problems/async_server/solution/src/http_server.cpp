#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {
void ReportError(sys::error_code ec, std::string_view str)
{
    std::cout << ec.what() <<std::endl;
    std::cout << str <<std::endl;
}

}  // namespace http_server
