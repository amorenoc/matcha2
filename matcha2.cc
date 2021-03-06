#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <utility>
#include <array>
#include <initializer_list>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <valarray>


namespace matcha {

// SFINAE type trait to detect whether T::const_iterator exists.

template<typename T>
struct has_const_iterator
{
private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template <typename C> static yes test(typename C::const_iterator*);
    template <typename C> static no  test(...);
public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
    typedef T type;
};

// SFINAE type trait to detect whether "T::const_iterator T::begin/end() const" exist.

template <typename T>
struct has_begin_end_OLD
{
    struct Dummy { typedef void const_iterator; };
    typedef typename std::conditional<has_const_iterator<T>::value, T, Dummy>::type TType;
    typedef typename TType::const_iterator iter;

    struct Fallback { iter begin() const; iter end() const; };
    struct Derived : TType, Fallback { };

    template<typename C, C> struct ChT;

    template<typename C> static char (&f(ChT<iter (Fallback::*)() const, &C::begin>*))[1];
    template<typename C> static char (&f(...))[2];
    template<typename C> static char (&g(ChT<iter (Fallback::*)() const, &C::end>*))[1];
    template<typename C> static char (&g(...))[2];

    static bool const beg_value = sizeof(f<Derived>(0)) == 2;
    static bool const end_value = sizeof(g<Derived>(0)) == 2;
};

template <typename T>
struct has_begin_end
{
    template<typename C> static char (&f(typename std::enable_if<
      std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::begin)),
      typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&f(...))[2];

    template<typename C> static char (&g(typename std::enable_if<
      std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::end)),
      typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&g(...))[2];

    static bool const beg_value = sizeof(f<T>(0)) == 1;
    static bool const end_value = sizeof(g<T>(0)) == 1;
};

// Basic is_container template; specialize to derive from std::true_type for all desired container types

template<typename T> struct is_container : public ::std::integral_constant<bool,
  has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value> { };

template<typename T, std::size_t N> struct is_container<T[N]> : public ::std::true_type { };

template<std::size_t N> struct is_container<char[N]> : public ::std::false_type { };

template <typename T> struct is_container< ::std::valarray<T>> : public ::std::true_type { };

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

template<typename T>
struct IsEqual
{
  bool matches(const T & actual, const T & expected) {
    std::cout << "equal to " << expected << '\n';
    return false;
  }

};

template<class T>
struct IsContaining
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

struct EndsWith
{
  bool matches(const std::string & actual, const std::string & expected) {
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

struct IsNull
{
  template<typename T>
  bool matches(const T & actual)
  {
    std::cout << "actual is null? " << actual << '\n';
    return false;
  }
};

template<class Predicate, class ... Ts>
class Matcher
{
public:
  Matcher(Ts&& ... args)
  : predicate()
  , args(std::forward<Ts>(args)...)
  { }

  template<class T, class indices = std::index_sequence_for<Ts...>>
  bool matches(const T & actual);
  bool matches(const std::string & val);

private:
  template <class T, std::size_t... I>
  bool matches(const T & actual, std::index_sequence<I...>);

private:
  Predicate predicate;
  std::tuple<Ts...> args;
};

template<class Predicate, class ... Ts>
template <class T, std::size_t... I>
bool Matcher<Predicate,Ts...>::matches(const T & actual, std::index_sequence<I...>)
{
  return predicate.matches(actual, std::get<I>(args)...);
}

template<class Predicate, class ... Ts>
template<class T, class indices>
bool Matcher<Predicate,Ts...>::matches(const T & actual)
{
  return matches(actual, indices());
}

template<class Predicate, class ... Ts>
bool Matcher<Predicate,Ts...>::matches(const std::string & val)
{
  return matches(val);
}

/*
 * factory functions
 */
template<class Predicate, class ... T>
auto make_matcher(T && ... val) 
{ 
  return Matcher<Predicate, T...>
    (std::forward<T>(val)...);
}

template<template <class...> class Predicate, class ... T>
auto make_matcher(T && ... val) 
{
  return Matcher<Predicate<T...>, T...>
    (std::forward<T>(val)...);
}

template<class Predicate>
auto make_matcher()
{
  return Matcher<Predicate>();
}


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
    (std::forward<T>(first), std::forward<T>(rest)...);
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

auto contain = [](auto && value) {
  return matcha::make_matcher<matcha::IsContaining>
    (std::forward<decltype(value)>(value));
};
auto contains = contain;

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
  expect(foo, contains(3));
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

