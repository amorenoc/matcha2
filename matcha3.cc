#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <array>
#include <map>
#include <initializer_list>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <valarray>
#include <typeinfo>

#include "prettyprint.hpp"

namespace matcha {

  template<typename T>
  struct is_container : pretty_print::is_container<T> 
  { };

  template<typename T, typename... Rest>
  struct is_same : std::false_type
  { };

  template<typename T, typename First>
  struct is_same<T, First> : std::is_same<T, First>
  { };

  template<typename T, typename First, typename... Rest>
  struct is_same<T, First, Rest...>
    : std::integral_constant<
        bool,
        std::is_same<T, First>::value && is_same<T, Rest...>::value
      >
  { };

  template<template <class...> class Predicate, class ... Ts>
  class Matcher
  {
  public:
    Matcher(Ts&& ... args) : pred(), args(std::forward<Ts>(args)...) { }
    template<class T> bool matches(const T &);

  private:
    template <class T, std::size_t... Is>
    bool matches_impl(const T & actual, std::index_sequence<Is...>);

  private:
    Predicate<std::decay_t<Ts>...> pred;
    std::tuple<Ts...> args;
  };

  template<template <class...> class Predicate, class ... Ts>
  template <class T, std::size_t... Is>
  bool Matcher<Predicate,Ts...>::matches_impl(const T & actual, std::index_sequence<Is...>)
  {
    return pred.matches(actual, std::get<Is>(args)...);
  }

  template<template <class...> class Predicate, class ... Ts>
  template<class T>
  bool Matcher<Predicate,Ts...>::matches(const T & actual)
  {
    return matches_impl(actual, std::index_sequence_for<Ts...>{});
  }

  /*
   * factory functions
   */
  template<template <class...> class Predicate, class ... T>
  auto make_matcher(T && ... val) 
  {
    return Matcher<Predicate, T...>(std::forward<T>(val)...);
  }


  template<typename T, class = void>
  struct IsEqual
  {
    bool matches(const T & actual, const T & expected)
    {
      std::cout << "not container" << std::endl;
      return actual == expected;
    }
  };

  template <typename T>
  struct IsEqual<T, std::enable_if_t<is_container<T>::value>> 
  {
    template<typename U>
    bool matches(const U & actual, const T & expected)
    {
      using std::begin;
      using std::end;

      std::cout << "container" << std::endl;
      return std::equal(begin(actual),   end(actual),
                        begin(expected), end(expected));
    }
  };

  template<class...>
  struct IsContaining;

  template<class T>
  struct IsContaining<T>
  {
    template<class C>
    bool matches(const C & actual, const T & expected)
    {
      static_assert(is_container<C>::value, "expects a Container");

      std::for_each(std::begin(actual), std::end(actual), [](auto &n) { 
        std::cout << "val is " << n << '\n';
      });
      return false;
    }
  };

  template<class Key, class T>
  struct IsContaining<Key,T>
  {
    template<class C>
    bool matches(const C & actual, const Key key, const T & value)
    {
      return false;
    }

  };

  template<typename T>
  struct EndsWith;

  template<>
  struct EndsWith<std::string>
  {
    bool matches(std::string actual, std::string expected) {
      std::cout << "actual is " << actual << "expected " << expected << '\n';
      return false;
    }
  };

  template<class ... Ts>
  struct AnyOf
  {
    static_assert(is_same<Ts...>::value, 
      "IsNot matcher requires a Matcher parameter");

    template<class T>
    bool matches(const T & actual, const Ts & ... args) {
      for (auto&& x : { args... }) {
        std::cout << "anyof is " << x << '\n';
  //            if (x.matches(actual))
  //                return true;
      }
      return false;
    }
  };

  template<class T, class ... Ts>
  struct OneOf
  {
    bool matches(const T & actual, const T & first, const Ts & ... rest) 
    {
      for (auto&& x : { first, rest... }) {
        std::cout << "x is " << x << '\n';
      }
      return false;
    }
  };

  template<typename...>
  struct IsNull;

  template<>
  struct IsNull<>
  {
    template<typename T>
    bool matches(const T & actual)
    {
      std::cout << "actual is null? " << actual << '\n';
      return false;
    }
  };

}; // end matcha

auto endWith = [](const std::string & value) {
  return matcha::make_matcher<matcha::EndsWith>(value);
};
auto endsWith = endWith;

auto equal = [](auto && value) {
  return matcha::make_matcher<matcha::IsEqual>
    (std::forward<decltype(value)>(value));
};
auto equals = equal;

template <class T, class ... Ts>
auto anyOf(T && first, Ts && ... rest)
{
  return matcha::make_matcher<matcha::AnyOf>
    (std::forward<T>(first), std::forward<Ts>(rest)...);
}

template <class T, class ... Ts>
auto oneOf(T && first, Ts && ... rest)
{
  static_assert(matcha::is_same<T,Ts...>::value, 
  "oneOf args must be all same type");

  return matcha::make_matcher<matcha::OneOf>
    (std::forward<T>(first), std::forward<Ts>(rest)...);
}

auto null = [] {
  return matcha::make_matcher<matcha::IsNull>();
};

template <class T, class ... Ts>
auto contain(T && first, Ts && ... rest) {
  return matcha::make_matcher<matcha::IsContaining>
    (std::forward<T>(first), std::forward<Ts>(rest)...);
};

template<class T, class Matcher>
bool expect(const T & val, Matcher && matcher)
{
  std::cout << "expect returns" << matcher.matches(val) << '\n';
  return false;
}


// expect({1,2,3}, to(not(contain(2))));
// expect("kayak", to(not(be(palindrome()))));
// expect(std::begin(v), std::end(v), to(have(everyItem(equal(3)))));

int main()
{
  expect("foo", endsWith("foo"));
  expect(3, equals(4));

  int b[] = {3,2,3,4};
  expect(b, contain(3));

  std::array<int,3> foo = {5,2,3};
  expect(foo, contain(3));
  int xx[] = {1,2,3};
  expect(xx, equals(foo));

  std::map<std::string, int> bar {
    {"this",  100},
    {"can",   100},
    {"be",    100},
    {"const", 100},
  };

  expect(bar, equals(bar));

  expect(bar, contain("string", 100));
  //expect(std::begin(foo), std::end(foo), contains(3));

  expect(4, anyOf(1,2,3,4,5,6));
  expect(1, oneOf(1,2,3,4,5));

  expect("foo", null());

}

//template<class Iter, class Matcher>
//bool expect(Iter first, Iter last, Matcher && matcher)
//{
//  const char *delim = "[";
//  std::for_each(first, last, [&delim](auto & val) { 
//    std::cout << delim << val;
//    delim = ", ";
//  });
//  std::cout << ']' << '\n';
//  return matcher.matches(first, last);
//}

