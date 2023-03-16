/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef BOOST_REDIS_JSON_HPP
#define BOOST_REDIS_JSON_HPP

#include <boost/json/serialize.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value_from.hpp>
#include <boost/redis/resp3/serialization.hpp>

namespace boost::redis::json
{

template <class T>
void to_bulk(std::string& to, T const& u)
{
   redis::resp3::boost_redis_to_bulk(to, boost::json::serialize(boost::json::value_from(u)));
}

template <class T>
void from_bulk(T& u, std::string_view sv, system::error_code&)
{
   auto const jv = boost::json::parse(sv);
   u = boost::json::value_to<T>(jv);
}

} // boost::redis::json

#endif // BOOST_REDIS_JSON_HPP
