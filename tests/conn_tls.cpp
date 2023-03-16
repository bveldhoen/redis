/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#define BOOST_TEST_MODULE conn-tls
#include <boost/test/included/unit_test.hpp>

#include <boost/redis.hpp>
#include <boost/redis/ssl/connection.hpp>
#include <boost/redis/src.hpp>
#include "common.hpp"

namespace net = boost::asio;

using connection = boost::redis::ssl::connection;
using boost::redis::request;
using boost::redis::response;
using boost::redis::ignore_t;

using endpoints = net::ip::tcp::resolver::results_type;

auto
resolve(
   std::string const& host = "127.0.0.1",
   std::string const& port = "6379") -> endpoints
{
   net::io_context ioc;
   net::ip::tcp::resolver resv{ioc};
   return resv.resolve(host, port);
}

struct endpoint {
   std::string host;
   std::string port;
};

bool verify_certificate(bool, net::ssl::verify_context&)
{
   std::cout << "set_verify_callback" << std::endl;
   return true;
}

BOOST_AUTO_TEST_CASE(ping)
{
   std::string const in = "Kabuf";

   request req;
   req.get_config().cancel_on_connection_lost = true;
   req.push("HELLO", 3, "AUTH", "aedis", "aedis");
   req.push("PING", in);
   req.push("QUIT");

   response<ignore_t, std::string, ignore_t> resp;

   auto const endpoints = resolve("db.occase.de", "6380");

   net::io_context ioc;
   net::ssl::context ctx{net::ssl::context::sslv23};
   connection conn{ioc, ctx};
   conn.next_layer().set_verify_mode(net::ssl::verify_peer);
   conn.next_layer().set_verify_callback(verify_certificate);

   net::connect(conn.lowest_layer(), endpoints);
   conn.next_layer().handshake(net::ssl::stream_base::client);

   conn.async_exec(req, resp, [](auto ec, auto) {
      BOOST_TEST(!ec);
   });

   conn.async_run([](auto ec) {
      BOOST_TEST(!ec);
   });

   ioc.run();

   BOOST_CHECK_EQUAL(in, std::get<1>(resp).value());
}

