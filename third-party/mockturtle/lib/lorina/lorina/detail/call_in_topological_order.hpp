/* lorina: C++ parsing library
 * Copyright (C) 2018-2021  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*! \cond PRIVATE */

#pragma once

#include <iostream>

namespace lorina
{

namespace detail
{

/* std::apply in C++14 taken from https://stackoverflow.com/a/36656413 */
template<typename Function, typename Tuple, size_t ... I>
auto apply( Function f, Tuple t, std::index_sequence<I ...> )
{
  return f( std::get<I>(t)... );
}

template<typename Function, typename Tuple>
auto apply( Function f, Tuple t )
{
  static constexpr auto size = std::tuple_size<Tuple>::value;
  return apply( f, t, std::make_index_sequence<size>{} );
}

/* \brief Parameter pack
 *
 * A pack of parameters of different types
 *
 * Example:
 *   using PackedParams = ParamPack<int, std::string, float>;
 *
 *   PackedParams params;
 *   params.set( 10, "a", 10.5f );
 */
template<typename... Args>
class ParamPack
{
public:
  using Tuple = std::tuple<Args...>;

public:
  explicit ParamPack() {}

  explicit ParamPack( Args... params )
    : params_( std::make_tuple( params... ) )
  {}

  explicit ParamPack( const std::tuple<Args...>& tup )
    : params_( tup )
  {}

  void set( Args... params )
  {
    params_ = std::make_tuple( params... );
  }

  auto get() const
  {
    return params_;
  }

private:
  std::tuple<Args...> params_;
}; // ParamPack

/* \brief Multple parameter packs
 *
 * A vector of N parameter packs
 *
 * Example:
 *   using PackedParamsA = ParamPack<int, std::string, int>;
 *   using PackedParamsB = ParamPack<double, int>;
 *
 *   PackedParamsA params_a( 10, "a", 4 );
 *   PackedParamsB params_b( 2.0, 3 );
 *   ParamPackN<PackedParamsA, PackedParamsB>
 *     params( params_a, params_b );
 */
template<typename... ParamPacks>
class ParamPackN
{
public:
  explicit ParamPackN()
  {}

  explicit ParamPackN( ParamPacks... packs )
    : packs_( std::make_tuple( packs... ) )
  {}

  template<int I>
  void set( typename std::tuple_element<I, std::tuple<ParamPacks...>>::type const& pack )
  {
    static_assert( I < sizeof...( ParamPacks ) );
    std::get<I>( packs_ ) = pack;
  }

  template<int I>
  auto get() const
  {
    static_assert( I < sizeof...( ParamPacks ) );
    return std::get<I>( packs_ );
  }

private:
  std::tuple<ParamPacks...> packs_;
}; // ParamPackN

/* \brief Parameter pack map
 *
 * A map from a KeyType to a parameter pack
 *
 * Example:
 *   ParamPackMap<std::string, int, std::string, int> param_map;
 *   param_map["a"] = ParamPack<int, std::string, int>( 10, "a", 10 );
 *   param_map["b"] = ParamPack<int, std::string, int>( 10, "b", 10 );
 */
template<typename KeyType, typename... Args>
class ParamPackMap
{
public:
  using ValueType = typename detail::ParamPack<Args...>::Tuple;

public:
  explicit ParamPackMap() {}

  auto& operator[]( const KeyType& key )
  {
    return map_[key];
  }

  const auto& operator[]( const KeyType& key ) const
  {
    return map_[key];
  }

  void insert( const KeyType& key, const std::tuple<Args...>& tup )
  {
    map_[key] = ParamPack<Args...>( tup );
  }

  void insert( const KeyType& key, Args... args )
  {
    map_[key] = ParamPack<Args...>( std::make_tuple( args... ) );
  }

private:
  std::unordered_map<KeyType, ParamPack<Args...>> map_;
};

/* \brief Multiple parameter pack maps
 *
 * A vector of N parameter pack maps
 *
 * Example:
 *   using StringToStringMap = ParamPackMap<std::string, std::string>;
 *   using StringToIntMap = ParamPackMap<std::string, int>;
 *
 *   ParamPackMapN<StringToStringMap, StringToIntMap> maps;
 *   maps.get<0>()["a"] = ParamPack<std::string>( std::string{"a"} );
 *   maps.get<1>()["b"] = ParamPack<int>( 10 );
 */
template<typename... ParamMaps>
class ParamPackMapN
{
public:
  explicit ParamPackMapN() {}

  template<int I>
  auto& get()
  {
    static_assert( I < sizeof...( ParamMaps ) );
    return std::get<I>( maps_ );
  }

  template<int I>
  auto& get() const
  {
    static_assert( I < sizeof...( ParamMaps ) );
    return std::get<I>( maps_ );
  }

private:
  std::tuple<ParamMaps...> maps_;
};

/* \brief A callable function
 *
 * Example:
 *   Func<int, std::string> f;
 *   f( std::tuple( 10, "a" ) );
 *   f( std::tuple( 12, "b" ) );
 */
template<typename... Args>
class Func
{
public:
  using Tuple = std::tuple<Args...>;

public:
  Func( std::function<void(Args...)> fn )
    : fn_( fn )
  {}

  void operator()( Tuple const& tup )
  {
    detail::apply( fn_, tup );
  }

private:
  std::function<void(Args...)> fn_;
}; // FuncPack

/* \brief Multiple packed functions
 *
 * A vector of N packed functions
 *
 * Example:
 *   using Fn_A = Func<int, std::string>;
 *   using Fn_B = Func<double, int>;
 *
 *   FuncPackN<Fn_A, Fn_B> functions(
 *     Fn_A( []( int a, std::string b ){ std::cout << a << ' ' << b << std::endl; } ),
 *     Fn_B( []( double a, int b ){ std::cout << a << ' ' << b << std::endl; } )
 *   );
 *
 *   functions.apply<0>( std::tuple( 10, "foo" ) );
 *   functions.apply<0>( std::tuple( 12, "bar" ) );
 *   functions.apply<1>( std::tuple( 3.41, 0 ) );
 *   functions.apply<1>( std::tuple( 2.58, 1 ) );
 */
template<typename... Fns>
class FuncPackN
{
public:
  explicit FuncPackN( std::tuple<Fns...> fns )
    : fns_( fns )
  {}

  explicit FuncPackN( Fns... fns )
    : fns_( std::make_tuple( fns... ) )
  {}

  template<int I>
  void apply( typename std::tuple_element<I, std::tuple<Fns...>>::type::Tuple const& tup )
  {
    std::get<I>( fns_ )( tup );
  }

private:
  std::tuple<Fns...> fns_;
}; // FuncPackN

template<typename TypeListOne, typename TypeListTwo>
struct call_in_topological_order;

template<typename... Fns, typename... Params>
struct call_in_topological_order<FuncPackN<Fns...>, ParamPackMapN<Params...>>
{
public:
  using dependency_type = std::pair<std::string, std::string>;

public:
  explicit call_in_topological_order( FuncPackN<Fns...> fns )
    : fns_( fns )
  {}

  void declare_known( const std::string& name )
  {
    known_.emplace( name );
  }

  template<int I>
  void call_deferred( const std::vector<std::string>& inputs,
		      const std::vector<std::string>& outputs,
		      const typename std::tuple_element<I, std::tuple<Params...>>::type::ValueType& tup )
  {
    /* do we have all inputs */
    std::unordered_set<std::string> unknown;
    for ( const auto& input : inputs )
    {
      if ( known_.find( input ) != std::end( known_ ) )
	continue;

      const auto it = waits_for_.find( input );
      if ( it == std::end( waits_for_ ) || !it->second.empty() )
      {
	unknown.insert( input );
      }
    }

    /* store the parameters */
    for ( const auto& output : outputs )
    {
      param_maps_.template get<I>()[output] = ParamPack( tup );
    }

    if ( !unknown.empty() )
    {
      /* defer computation */
      for ( const auto& input : unknown )
      {
	for ( const auto& output : outputs )
	{
	  triggers_[input].insert( output );
	  waits_for_[output].insert( input );
	}
      }
      return;
    }

    /* trigger dependency computation */
    for ( const auto& output : outputs )
    {
      compute_dependencies<I>( output );
    }
  }

  template<int I>
  void compute_dependencies( const std::string& output )
  {
    /* init empty, makes sure nothing is waiting for this output */
    waits_for_[output];

    std::stack<std::string> computed;
    computed.push( output );

    while ( !computed.empty() )
    {
      auto const next = computed.top();
      computed.pop();

      // C++17: std::apply( f, _stored_params[next] );
      // detail::apply( f_, stored_params_[next] );

      fns_.template apply<I>( param_maps_.template get<I>()[next].get() );

      /* activate all the triggers */
      for ( const auto& other : triggers_[next] )
      {
	waits_for_[other].erase( next );
	if ( waits_for_[other].empty() )
	{
	  computed.push( other );
	}
      }
      triggers_[next].clear();
    }
  }

  std::vector<dependency_type> unresolved_dependencies()
  {
    std::vector<dependency_type> deps;
    for ( const auto& item : waits_for_ )
    {
      auto const& key = item.first;
      auto const& wait_list = item.second;

      if ( wait_list.empty() )
	continue;

      /* collect all keys that are still waiting for an item */
      for ( const auto& entry : wait_list )
      {
	deps.emplace_back( key, entry );
      }
    }
    return deps;
  }

private:
  FuncPackN<Fns...> fns_;
  ParamPackMapN<Params...> param_maps_;

  std::unordered_set<std::string> known_;
  std::unordered_map<std::string, std::unordered_set<std::string>> waits_for_;
  std::unordered_map<std::string, std::unordered_set<std::string>> triggers_;
}; // call_in_topological_order

} // namespace detail

} // namespace lorina
