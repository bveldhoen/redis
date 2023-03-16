/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <boost/redis/run.hpp>
#include <boost/redis/address.hpp>
#include <boost/system/errc.hpp>
#define BOOST_TEST_MODULE conn-exec
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "common.hpp"
#include <boost/redis/src.hpp>

// TODO: Test whether HELLO won't be inserted passt commands that have
// been already writen.
// TODO: Test async_exec with empty request e.g. hgetall with an empty
// container.

namespace net = boost::asio;
using error_code = boost::system::error_code;
using connection = boost::redis::connection;
using boost::redis::request;
using boost::redis::response;
using boost::redis::ignore;
using boost::redis::ignore_t;
using boost::redis::async_run;
using boost::redis::address;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_CASE(hello_priority)
{
   request req1;
   req1.push("PING", "req1");

   request req2;
   req2.get_config().hello_with_priority = false;
   req2.push("HELLO", 3);
   req2.push("PING", "req2");
   req2.push("QUIT");

   request req3;
   req3.get_config().hello_with_priority = true;
   req3.push("HELLO", 3);
   req3.push("PING", "req3");

   net::io_context ioc;

   connection conn{ioc};

   bool seen1 = false;
   bool seen2 = false;
   bool seen3 = false;

   conn.async_exec(req1, ignore, [&](auto ec, auto){
      std::cout << "bbb" << std::endl;
      BOOST_TEST(!ec);
      BOOST_TEST(!seen2);
      BOOST_TEST(seen3);
      seen1 = true;
   });
   conn.async_exec(req2, ignore, [&](auto ec, auto){
      std::cout << "ccc" << std::endl;
      BOOST_TEST(!ec);
      BOOST_TEST(seen1);
      BOOST_TEST(seen3);
      seen2 = true;
   });
   conn.async_exec(req3, ignore, [&](auto ec, auto){
      std::cout << "ddd" << std::endl;
      BOOST_TEST(!ec);
      BOOST_TEST(!seen1);
      BOOST_TEST(!seen2);
      seen3 = true;
   });

   async_run(conn, address{}, 10s, 10s, [](auto ec){
      BOOST_TEST(!ec);
   });

   ioc.run();
}

BOOST_AUTO_TEST_CASE(wrong_response_data_type)
{
   request req;
   req.push("HELLO", 3);
   req.push("QUIT");

   // Wrong data type.
   response<ignore_t, int> resp;
   net::io_context ioc;

   connection conn{ioc};

   conn.async_exec(req, resp, [](auto ec, auto){
      BOOST_CHECK_EQUAL(ec, boost::redis::error::not_a_number);
   });
   async_run(conn, address{}, 10s, 10s, [](auto ec){
      BOOST_CHECK_EQUAL(ec, boost::asio::error::basic_errors::operation_aborted);
   });

   ioc.run();
}

BOOST_AUTO_TEST_CASE(cancel_request_if_not_connected)
{
   request req;
   req.get_config().cancel_if_not_connected = true;
   req.push("HELLO", 3);
   req.push("PING");

   net::io_context ioc;
   auto conn = std::make_shared<connection>(ioc);
   conn->async_exec(req, ignore, [](auto ec, auto){
      BOOST_CHECK_EQUAL(ec, boost::redis::error::not_connected);
   });

   ioc.run();
}
