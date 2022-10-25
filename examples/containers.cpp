/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <map>
#include <vector>
#include <iostream>

#include <boost/asio.hpp>
#if defined(BOOST_ASIO_HAS_CO_AWAIT)
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <aedis.hpp>
#include "print.hpp"

// Include this in no more than one .cpp file.
#include <aedis/src.hpp>

namespace net = boost::asio;
using namespace net::experimental::awaitable_operators;
using aedis::adapt;
using aedis::resp3::request;
using aedis::endpoint;
using tcp_socket = net::use_awaitable_t<>::as_default_on_t<net::ip::tcp::socket>;
using connection = aedis::connection<tcp_socket>;

// Sends some containers.
net::awaitable<void> send()
{
   auto ex = co_await net::this_coro::executor;

   std::vector<int> vec
      {1, 2, 3, 4, 5, 6};

   std::map<std::string, std::string> map
      {{"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}};

   request req;
   req.push_range("RPUSH", "rpush-key", vec); // Sends
   req.push_range("HSET", "hset-key", map); // Sends
   req.push("QUIT");

   connection conn{ex};
   endpoint ep{"127.0.0.1", "6379"};
   boost::system::error_code ec1, ec2;
   co_await (
      conn.async_run(ep, {}, net::redirect_error(net::use_awaitable, ec1)) ||
      conn.async_exec(req, adapt(), net::redirect_error(net::use_awaitable, ec2))
   );
}

net::awaitable<void> async_main()
{
   auto ex = co_await net::this_coro::executor;

   request req;
   req.push("MULTI");
   req.push("LRANGE", "rpush-key", 0, -1); // Retrieves
   req.push("HGETALL", "hset-key"); // Retrieves
   req.push("EXEC");
   req.push("QUIT");

   std::tuple<
      aedis::ignore, // multi
      aedis::ignore, // lrange
      aedis::ignore, // hgetall
      std::tuple<std::optional<std::vector<int>>, std::optional<std::map<std::string, std::string>>>, // exec
      aedis::ignore  // quit
   > resp;

   co_await send();
   connection conn{ex};
   endpoint ep{"127.0.0.1", "6379"};
   boost::system::error_code ec1, ec2;
   co_await (
      conn.async_run(ep, {}, net::redirect_error(net::use_awaitable, ec1)) ||
      conn.async_exec(req, adapt(resp), net::redirect_error(net::use_awaitable, ec2))
   );

   print(std::get<0>(std::get<3>(resp)).value());
   print(std::get<1>(std::get<3>(resp)).value());
}

auto main() -> int
{
   try {
      net::io_context ioc;
      net::co_spawn(ioc, async_main(), net::detached);
      ioc.run();
   } catch (...) {
      std::cerr << "Error." << std::endl;
   }
}

#else // defined(BOOST_ASIO_HAS_CO_AWAIT)
auto main() -> int {std::cout << "Requires coroutine support." << std::endl; return 1;}
#endif // defined(BOOST_ASIO_HAS_CO_AWAIT)
