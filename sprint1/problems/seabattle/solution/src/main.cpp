#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {

        if(  my_initiative  )
        {
            while( !IsGameEnded() && processMyMove( socket ) );
        }

        while( !IsGameEnded() )
        {
            while( !IsGameEnded() && processEnemiesMove( socket ) );

            while( !IsGameEnded() && processMyMove( socket )  );
        }
        std::cout<< "Game ended"<< std::endl;
        if( my_field_.IsLoser() )
        {
            std::cout<< "You lose" << std::endl;
        }
        else
        {
            std::cout << "You win" << std::endl;
        }
    }

private:
    bool  processEnemiesMove( tcp::socket& socket )
    {
        PrintFieldPair( my_field_, other_field_ );
        std::cout << "Waiting enemies move" << std::endl;
        //принять ход противника
        std::optional<std::string> recvStr = ReadExact<2>(socket);
        if( !recvStr )
        {
            std::cout<< "Can't read packet";
            exit(1);
        }
        auto pairMove = ParseMove( *recvStr );
        if( !pairMove )
        {
            std::cout << "Wrong enemies move" << std::endl;
            std::cout << *recvStr;
            exit(1);
        }

        //вычислить и отправить результат попадание/промах
        auto shotRes = my_field_.Shoot( (*pairMove).second, (*pairMove).first );
        std::string resStr = std::to_string( static_cast<int> ( shotRes ) );
        if( !WriteExact( socket, resStr ) )
        {
            std::cout<<"connection lost";
            exit(1);
        }

        //вывести результат попадания
        if( shotRes == SeabattleField::ShotResult::HIT )
        {
            std::cout << "Enemy hit your ship " << *recvStr << std::endl;
            return true;
        }
        else if ( shotRes == SeabattleField::ShotResult::KILL )
        {
            std::cout << "Enemy killed your ship " << *recvStr << std::endl;
            return true;
        }
        else if ( shotRes == SeabattleField::ShotResult::MISS )
        {
            std::cout << "Enemy missed " << *recvStr << std::endl;
            return false;
        }

    }
    bool processMyMove( tcp::socket& socket )
    {
        PrintFieldPair( my_field_, other_field_ );
        std::cout << "Your turn" << std::endl;

        //отправить свой ход противнку
        std::string strMove = getMoveFromUser();

        if( !WriteExact( socket, strMove ) )
        {
            std::cout<<"connection lost";
            exit(1);
        }

        //принять ответ от противника попадание/промах
        std::optional<std::string> recvStr = ReadExact<1>(socket);
        if( !recvStr )
        {
            std::cout<< "Can't read packet";
            exit(1);
        }
        auto pairMove = ParseMove( strMove );
        int res = stoi ( *recvStr );
        if( res == static_cast<int> ( SeabattleField::ShotResult::HIT ) )
        {
            std::cout<< "You hit enemies ship" << std::endl;
            other_field_.MarkHit( (*pairMove).second, (*pairMove).first );
            return true;
        }
        else if( res == static_cast<int> ( SeabattleField::ShotResult::MISS ) )
        {
            std::cout<< "You missed" << std::endl;
            other_field_.MarkMiss( (*pairMove).second, (*pairMove).first );
            return false;
        }
        else if( res == static_cast<int> ( SeabattleField::ShotResult::KILL ) )
        {
            std::cout<< "You killed enemies ship" << std::endl;
            other_field_.MarkKill( (*pairMove).second, (*pairMove).first );
            return true;
        }
    }
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию
    std::string  getMoveFromUser()
    {
        std::string inStr;
        do
        {
        std::cout << "Enter a cell" << std::endl;
        std::cin >> inStr;
        } while ( !ParseMove ( inStr ) );

        return inStr;

    }


private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);
    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;
    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);

    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        exit(1);
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        std::cout<< ec.what() << std::endl;
        std::cout << ip_str << std::endl;
        //exit(1);
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
        std::cout << ec.what() << std::endl;
        exit(1);
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
