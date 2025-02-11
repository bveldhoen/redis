/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <boost/asio.hpp>
#if defined(BOOST_ASIO_HAS_CO_AWAIT)
#define BOOST_JSON_NO_LIB
#define BOOST_CONTAINER_NO_LIB
#include <boost/redis.hpp>
#include <boost/describe.hpp>
#include <string>
#include <iostream>
#include "common/common.hpp"
#include "json.hpp"

// Include this in no more than one .cpp file.
#include <boost/json/src.hpp>

namespace net = boost::asio;
namespace redis = boost::redis;
using namespace boost::describe;
using boost::redis::request;
using boost::redis::response;
using boost::redis::operation;
using boost::redis::ignore_t;

// Struct that will be stored in Redis using json serialization. 
struct user {
   std::string name;
   std::string age;
   std::string country;
};

// The type must be described for serialization to work.
BOOST_DESCRIBE_STRUCT(user, (), (name, age, country))

// Boost.Redis customization points (examples/json.hpp)
void boost_redis_to_bulk(std::string& to, user const& u)
   { boost::redis::json::to_bulk(to, u); }

void boost_redis_from_bulk(user& u, std::string_view sv, boost::system::error_code& ec)
   { boost::redis::json::from_bulk(u, sv, ec); }

auto run(std::shared_ptr<connection> conn, std::string host, std::string port) -> net::awaitable<void>
{
   co_await connect(conn, host, port);
   co_await conn->async_run();
}

net::awaitable<void> co_main(std::string host, std::string port)
{
   auto ex = co_await net::this_coro::executor;
   auto conn = std::make_shared<connection>(ex);
   net::co_spawn(ex, run(conn, host, port), net::detached);

   // user object that will be stored in Redis in json format.
   user const u{"Joao", "58", "Brazil"};

   // Stores and retrieves in the same request.
   request req;
   req.push("HELLO", 3);
   req.push("SET", "json-key", u); // Stores in Redis.
   req.push("GET", "json-key"); // Retrieves from Redis.

   response<ignore_t, ignore_t, user> resp;

   co_await conn->async_exec(req, resp);

   // Prints the first ping
   std::cout
      << "Name: " << std::get<2>(resp).value().name << "\n"
      << "Age: " << std::get<2>(resp).value().age << "\n"
      << "Country: " << std::get<2>(resp).value().country << "\n";

   conn->cancel(operation::run);
}

#endif // defined(BOOST_ASIO_HAS_CO_AWAIT)
