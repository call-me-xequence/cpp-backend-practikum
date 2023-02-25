#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
namespace net = boost::asio;
using net::ip::udp;
#include <stdlib.h>

using namespace std::literals;
void startServer ( uint16_t port );
void startClient ( uint16_t port );
static const size_t max_buffer_size = 65200;
int main(int argc, char** argv)
{


    if( argc != 3 )
    {
        std::cout<< "wrong parameters" << std::endl;
    }
    uint16_t port = atoi( argv[2] );

    if( argv[1] == "client"sv )
    {
        startClient( port );
    }
    else if ( argv[1] == "server"sv  )
    {
        startServer( port );
    }
    else
    {
        std::cout<< "wrong parameter server/port" <<std::endl;
    }

    return 0;
}

void startClient ( uint16_t port )
{
    Recorder recorder(ma_format_u8, 1);
    net::io_context io_context;



    std::cout<< "client started" <<std::endl;
    while (true)
    {
        net::streambuf buffer;
        std::ostream os (&buffer);
        std::string strHostAddress;

        std::cout << "Enter ip to record message..." << std::endl;
        std::getline(std::cin, strHostAddress);
        auto rec_result = recorder.Record(65000, 1.5s);
        udp::socket socket( io_context, udp::v4());


        os << static_cast<uint16_t>( rec_result.frames );
        for( char ch : rec_result.data )
        {
            os << ch;
        }

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(strHostAddress, ec), port);
        if( ec )
        {
            std::cout<< "wrong ip" << std::endl;
            continue;
        }


        socket.send_to( buffer.data() , endpoint);
        std::cout<< " Bytes sent:"<< buffer.size() << std::endl;



    }
}

void startServer ( uint16_t port )
{

    Player player(ma_format_u8, 1);
    net::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
    std::cout<< "server started" << std::endl;
    while (true)
    {
        net::streambuf buffer;
        auto prep = buffer.prepare( 65500 );
        std::istream is(&buffer);
        auto size = socket.receive( prep );
        buffer.commit( size );

        uint16_t frameNumber;
        std::vector<char> data;
        is >> frameNumber;
        data.reserve( frameNumber);
        for( int i = 0; i < frameNumber; ++i )
        {
            char ch;
            is >> ch;
            data.push_back( ch );
        }

        player.PlayBuffer( data.data(), frameNumber, 1.5s);
        std::cout << "Playing done. Packet size:" << size << std::endl;
        std:: cout << "frameNumber:" << frameNumber<< std::endl;
    }
}

