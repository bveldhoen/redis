/* Copyright (c) 2019 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <map>
#include <vector>
#include <iostream>

#include <aedis/aedis.hpp>
#include <aedis/src.hpp>

namespace net = aedis::net;
namespace redis = aedis::redis;
using aedis::redis::command;
using aedis::redis::receiver;
using aedis::resp3::node;
using client_type = redis::client<net::detached_t::as_default_on_t<aedis::net::ip::tcp::socket>>;

// Helper function.
template <class Container>
void print_and_clear(Container& cont)
{
   std::cout << "\n";
   for (auto const& e: cont) std::cout << e << " ";
   std::cout << "\n";
   cont.clear();
}

using receiver_type =
   receiver<
      std::list<int>,
      std::set<std::string>,
      std::vector<node<std::string>>
   >;

struct myreceiver : receiver_type {
public:
   myreceiver(client_type& db) : db_{&db} {}

private:
   std::shared_ptr<client_type> db_;

   int to_tuple_idx_impl(command cmd) override
   {
      switch (cmd) {
         case command::lrange:   return index_of<std::list<int>>();
         case command::smembers: return index_of<std::set<std::string>>();
         default: return -1;
      }
   }

   void on_read_impl(command cmd) override
   {
      switch (cmd) {
         case command::hello:
         {
            std::map<std::string, std::string> map
               { {"key1", "value1"}
               , {"key2", "value2"}
               , {"key3", "value3"}
               };

            std::vector<int> vec
               {1, 2, 3, 4, 5, 6};

            std::set<std::string> set
               {"one", "two", "three", "four"};

            // Sends the stl containers.
            db_->send_range(command::hset, "hset-key", std::cbegin(map), std::cend(map));
            db_->send_range(command::rpush, "rpush-key", std::cbegin(vec), std::cend(vec));
            db_->send_range(command::sadd, "sadd-key", std::cbegin(set), std::cend(set));

            //_ Retrieves the containers.
            db_->send(command::hgetall, "hset-key");
            db_->send(command::lrange, "rpush-key", 0, -1);
            db_->send(command::smembers, "sadd-key");
            db_->send(command::quit);
         } break;

         case command::lrange:
         print_and_clear(get<std::list<int>>());
         break;

         case command::smembers:
         print_and_clear(get<std::set<std::string>>());
         break;

         default:;
      }
   }
};

int main()
{
   net::io_context ioc;
   client_type db{ioc.get_executor()};
   myreceiver recv{db};

   db.async_run(
       recv,
       {net::ip::make_address("127.0.0.1"), 6379},
       [](auto ec){ std::cout << ec.message() << std::endl;});

   ioc.run();
}

