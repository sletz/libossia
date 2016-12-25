#pragma once
#include <ossia/detail/optional.hpp>
#include <ossia_export.h>

#include <boost/any.hpp>
#include <hopscotch_map.h>
#include <vector>
#include <string>

/**
 * \file node_attributes.hpp
 *
 * This file defines default types and accessors for common extended
 * attributes.
 */
namespace ossia
{
namespace net
{
using any_map = tsl::hopscotch_map<std::string, boost::any>;
using extended_attributes = any_map;

struct OSSIA_EXPORT instance_bounds
{
  ossia::optional<uint32_t> min_instances;
  ossia::optional<uint32_t> max_instances;
};

using tags = std::vector<std::string>;
using description = std::string;
using priority = int32_t;
using refresh_rate = int32_t;

// The following attributes are just accessors to the
// extended attributes container.
OSSIA_EXPORT optional<instance_bounds> get_dynamic_instances(const extended_attributes& n);
OSSIA_EXPORT void set_dynamic_instances(extended_attributes& n, optional<instance_bounds>);

OSSIA_EXPORT optional<tags> get_tags(const extended_attributes& n);
OSSIA_EXPORT void set_tags(extended_attributes& n, const optional<tags>& v);

OSSIA_EXPORT optional<description> get_description(const extended_attributes& n);
OSSIA_EXPORT void set_description(extended_attributes& n, const optional<description>& v);

OSSIA_EXPORT optional<priority> get_priority(const extended_attributes& n);
OSSIA_EXPORT void set_priority(extended_attributes& n, const optional<priority>& v);

OSSIA_EXPORT optional<refresh_rate> get_refresh_rate(const extended_attributes& n);
OSSIA_EXPORT void set_refresh_rate(extended_attributes& n, const optional<refresh_rate>& v);


template<typename T>
auto get_attribute(
    const extended_attributes& e,
    const std::string& name)
{
  auto it = e.find(name);
  if(it != e.cend())
  {
    auto val = boost::any_cast<T*>(it.value());
    if(val)
      return *val;
  }

  return T{};
}

template<typename T>
optional<T> get_optional_attribute(
    const extended_attributes& e,
    const std::string& name)
{
  auto it = e.find(name);
  if(it != e.cend())
  {
    auto val = boost::any_cast<T*>(it.value());
    if(val)
      return *val;
  }

  return ossia::none;
}

template<typename T>
void set_attribute(extended_attributes& e, const std::string& str, const T& val)
{ e[str] = val; }
template<typename T>
void set_attribute(extended_attributes& e, const std::string& str, T&& val)
{ e[str] = std::move(val); }

inline OSSIA_EXPORT void set_attribute(extended_attributes &e, const std::string& str, ossia::none_t)
{
  e.erase(str);
}

template<typename T>
OSSIA_EXPORT void set_optional_attribute(
    extended_attributes &e,
    const std::string& str, const optional<T>& opt)
{
  if(opt)
    set_attribute(e, str, *opt);
  else
    set_attribute(e, str, ossia::none);
}

}}