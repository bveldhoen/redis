/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <boost/redis/run.hpp>
#include <boost/redis/check_health.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

#if defined(BOOST_ASIO_HAS_CO_AWAIT)

namespace net = boost::asio;
using namespace net::experimental::awaitable_operators;
using tcp_socket = net::use_awaitable_t<>::as_default_on_t<net::ip::tcp::socket>;
using tcp_acceptor = net::use_awaitable_t<>::as_default_on_t<net::ip::tcp::acceptor>;
using signal_set = net::use_awaitable_t<>::as_default_on_t<net::signal_set>;
using boost::redis::request;
using boost::redis::response;
using boost::redis::async_check_health;
using boost::redis::async_run;
using connection = boost::asio::use_awaitable_t<>::as_default_on_t<boost::redis::connection>;

auto echo_server_session(tcp_socket socket, std::shared_ptr<connection> conn) -> net::awaitable<void>
{
   request req;
   response<std::string> resp;

   for (std::string buffer;;) {
      auto n = co_await net::async_read_until(socket, net::dynamic_buffer(buffer, 1024), "\n");
      req.push("PING", buffer);
      co_await conn->async_exec(req, resp);
      co_await net::async_write(socket, net::buffer(std::get<0>(resp).value()));
      std::get<0>(resp).value().clear();
      req.clear();
      buffer.erase(0, n);
   }
}

// Listens for tcp connections.
auto listener(std::shared_ptr<connection> conn) -> net::awaitable<void>
{
   auto ex = co_await net::this_coro::executor;
   tcp_acceptor acc(ex, {net::ip::tcp::v4(), 55555});
   for (;;)
      net::co_spawn(ex, echo_server_session(co_await acc.async_accept(), conn), net::detached);
}

// Called from the main function (see main.cpp)
auto co_main(std::string host, std::string port) -> net::awaitable<void>
{
   auto ex = co_await net::this_coro::executor;
   auto conn = std::make_shared<connection>(ex);
   signal_set sig{ex, SIGINT, SIGTERM};

   request req;
   req.push("HELLO", 3);

   co_await ((async_run(*conn, host, port) || listener(conn) || async_check_health(*conn) ||
            sig.async_wait()) && conn->async_exec(req));
}

#endif // defined(BOOST_ASIO_HAS_CO_AWAIT)
