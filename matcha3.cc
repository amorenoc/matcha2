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

  template<template <class...> class Predicate, class ... Ts>
  class Matcher
  {
  public:
    Matcher(Ts&& ... args)
      : pred()
      , args(std::forward<Ts>(args)...) 
    { }

    template<class T>
    bool matches(const T &);
    void describe(std::ostream& o);

    friend std::ostream& operator<<(std::ostream& o, 
        Matcher & matcher) 
    {
        matcher.describe(o);
        return o;
    }

  private:
    template <class T, std::size_t... Is>
    bool matches_impl(const T & actual, std::index_sequence<Is...>);

    template <std::size_t... Is>
    void describe_impl(std::ostream& o, std::index_sequence<Is...>);

    Predicate<std::decay_t<Ts>...> pred;
    std::tuple<Ts...> args;
  };

  template<template <class...> class Predicate, class ... Ts>
  template <class T, std::size_t... Is>
  bool Matcher<Predicate,Ts...>::matches_impl(const T & actual, 
      std::index_sequence<Is...>)
  {
    return pred.matches(actual, std::get<Is>(args)...);
  }

  template<template <class...> class Predicate, class ... Ts>
  template<class T>
  bool Matcher<Predicate,Ts...>::matches(const T & actual)
  {
    return matches_impl(actual, std::index_sequence_for<Ts...>{});
  }

  template<template <class...> class Predicate, class ... Ts>
  template <std::size_t... Is>
  void Matcher<Predicate,Ts...>::describe_impl(std::ostream& o, 
      std::index_sequence<Is...>)
  {
    return pred.describe(o, std::get<Is>(args)...);
  }

  template<template <class...> class Predicate, class ... Ts>
  void Matcher<Predicate,Ts...>::describe(std::ostream& o)
  {
    return describe_impl(o, std::index_sequence_for<Ts...>{});
  }

  template<template <class...> class Predicate, class ... T>
  auto make_matcher(T && ... val) 
  {
    return Matcher<Predicate, T...>(std::forward<T>(val)...);
  }

  template<typename T>
  struct is_container : pretty_print::is_container<T> { };

  template<typename T, typename... Rest>
  struct is_same : std::false_type { };

  template<typename T, typename First>
  struct is_same<T, First> : std::is_same<T, First> { };

  template<typename T, typename First, typename... Rest>
  struct is_same<T, First, Rest...>
    : std::integral_constant<
        bool,
        std::is_same<T, First>::value && is_same<T, Rest...>::value
      >
  { };

  template<typename>
  struct is_matcher : std::false_type { };

  template<template <class...> class Predicate, class ... Ts>
  struct is_matcher<Matcher<Predicate,Ts...>>: std::true_type { };

  template<typename T>
  struct To 
  {
    static_assert(is_matcher<T>::value, "expects a Matcher argument");

    template<typename U>
    bool matches(const U & actual, T & expected)
    {
      return expected.matches(actual);
    }

    void describe(std::ostream& o, T & expected) {
      o << "to " << expected;
    }
  };

  template<typename T>
  struct Not 
  {
    static_assert(is_matcher<T>::value, "expects a Matcher argument");

    template<typename U>
    bool matches(const U & actual, T & expected) {
      return !expected.matches(actual);
    }

    void describe(std::ostream& o, T & expected) {
      o << "not " << expected;
    }
  };

  template<typename T, class = void>
  struct IsEqual
  {
    bool matches(const T & actual, const T & expected) {
      std::cout << "not container" << std::endl;
      return actual == expected;
    }

    void describe(std::ostream& o, T const& expected) {
       o << "equal " << expected;
    }
  };

  template <typename T>
  struct IsEqual<T, std::enable_if_t<is_container<T>::value>> 
  {
    template<typename U>
    bool matches(const U & actual, const T & expected) {
      using std::begin;
      using std::end;

      std::cout << "container" << std::endl;
      return std::equal(begin(actual),   end(actual),
                        begin(expected), end(expected));
    }

    void describe(std::ostream& o, T const& expected) {
       o << "equal " << expected;
    }
  };

  template<class...> struct IsContaining;

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

    void describe(std::ostream& o, const T & expected) {
      o << "contain " << expected;
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

    void describe(std::ostream& o, const Key key, const T & value) {
      o << "contain key " << key << " and value " <<  value;
    }

  };

  template<typename T>
  struct EndsWith;

  template<>
  struct EndsWith<std::string>
  {
    bool matches(std::string actual, std::string expected) {
      return true;
    }

    void describe(std::ostream& o, std::string expected) {
      o << "end with " << expected;
    }

  };

  template<class ... Ts>
  struct AnyOf
  {
    static_assert(is_same<Ts...>::value,
        "IsNot matcher requires a Matcher parameter");

    template<class T>
    bool matches(const T & actual, const Ts & ... args) 
    {
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
    static_assert(is_same<T,Ts...>::value,
        "oneOf args must be all same type");

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

  template <typename T>
  std::string to_string(T & val)
  {
    std::ostringstream out;
    out << val;
    return out.str();
  }

  template<typename T>
  struct output_traits;

  template<>
  struct output_traits<bool>
  {
    typedef bool result_type;
    static constexpr bool success = true;
    static constexpr bool failure = false;

    static std::ostream & ostream(bool & result) {
      return std::cout;
    }
  };


  template<class Result, class T, class U>
  auto assertResult(T const& actual, U && matcher) 
  {
    Result result = output_traits<Result>::failure;
    //const std::string red("\033[0;31m");
    //const std::string green("\033[1;32m");

    if (matcher.matches(actual))
      return output_traits<Result>::success;

    output_traits<Result>::ostream(result)
      << "expected " 
      << to_string(actual) << ' '
      << to_string(matcher)
      << '\n'; 

    return result;
  }

  template<class T, class Matcher>
  auto expect(T const& actual, Matcher && matcher) {
    return assertResult<bool>(actual, matcher);
  }

  namespace predicates {

    template <typename T>
    auto to(T && matcher) 
    {
      return make_matcher<To>(std::forward<T>(matcher));
    }

    template <typename T>
    auto operator!(T && matcher)
    {
      return make_matcher<Not>(std::forward<T>(matcher));
    }

    auto null() {
      return make_matcher<IsNull>();
    }

    template <class T, class ... Ts>
    auto oneOf(T && first, Ts && ... rest) 
    {
      return make_matcher<OneOf>(std::forward<T>(first),
        std::forward<Ts>(rest)...);
    }

    template <class T, class ... Ts> 
    auto anyOf(T && first, Ts && ... rest)
    {
      return make_matcher<AnyOf>(std::forward<T>(first), 
        std::forward<Ts>(rest)...);
    }

    auto endWith = [](const std::string & value) 
    {
      return make_matcher<EndsWith>(value);
    };

    auto endsWith = endWith;

    template <class T, class ... Ts>
    auto contain(T && first, Ts && ... rest) 
    {
      return make_matcher<IsContaining>(std::forward<T>(first), 
        std::forward<Ts>(rest)...);
    }

    auto equal = [](auto && value)
    {
      return make_matcher<IsEqual>(std::forward<decltype(value)>(value));
    };

    auto equals = equal;
  }; // end predicates

}; // end matcha


using namespace matcha::predicates;
using matcha::expect;

int main()
{
  expect("foo", to(not(endWith("foo"))));
  expect(3, to(equal(4)));

  int b[] = {3,2,3,4};
  expect(b, to(contain(3)));

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

  //expect(4, to(be(anyOf(1,2,3,4,5,6))));
  //expect(1, oneOf(1,2,3,4,5));

  //expect("foo", null());

}

// expect({1,2,3}, to(not(contain(2))));
// expect("kayak", to(not(be(palindrome()))));
// expect(std::begin(v), std::end(v), to(have(everyItem(equal(3)))));


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
