/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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

/*!
  \file color_view.hpp
  \brief Manager view for traversal IDs, called colors

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

namespace mockturtle
{

/*!\brief Manager view for traversal IDs (in-place storage).
 *
 * Traversal IDs, called colors, are unsigned integers that can be
 * assigned to nodes.  The corresponding values are stored in-place in
 * the flags of the underlying of the network.
 */
template<typename Ntk>
class color_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit color_view( Ntk const& ntk )
      : Ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  }

  /*! \brief Returns a new color and increases the current color */
  uint32_t new_color() const
  {
    return ++this->_storage->trav_id;
    // return ++value;
  }

  /*! \brief Returns the current color */
  uint32_t current_color() const
  {
    return this->_storage->trav_id;
    // return value;
  }

  /*! \brief Assigns all nodes to `color` */
  void clear_colors( uint32_t color = 0 ) const
  {
    std::for_each( this->_storage->nodes.begin(), this->_storage->nodes.end(),
                   [color]( auto& n ) { n.data[1].h1 = color; } );
  }

  /*! \brief Returns the color of a node */
  auto color( node const& n ) const
  {
    return this->_storage->nodes[n].data[1].h1;
  }

  /*! \brief Returns the color of a node */
  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  auto color( signal const& n ) const
  {
    return this->_storage->nodes[this->get_node( n )].data[1].h1;
  }

  /*! \brief Assigns the current color to a node */
  void paint( node const& n ) const
  {
    this->_storage->nodes[n].data[1].h1 = current_color();
  }

  /*! \brief Assigns `color` to a node */
  void paint( node const& n, uint32_t color ) const
  {
    this->_storage->nodes[n].data[1].h1 = color;
  }

  /*! \brief Copies the color from `other` to `n` */
  void paint( node const& n, node const& other ) const
  {
    this->_storage->nodes[n].data[1].h1 = color( other );
  }

  /*! \brief Evaluates a predicate on the color of a node */
  template<typename Pred>
  bool eval_color( node const& n, Pred&& pred ) const
  {
    return pred( color( n ) );
  }

  /*! \brief Evaluates a predicate on the colors of two nodes */
  template<typename Pred>
  bool eval_color( node const& a, node const& b, Pred&& pred ) const
  {
    return pred( color( a ), color( b ) );
  }

  /*! \brief Evaluates a predicate on the colors of the fanins of a node */
  template<typename Pred>
  bool eval_fanins_color( node const& n, Pred&& pred ) const
  {
    bool result = true;
    this->foreach_fanin( n, [&]( signal const& fi ) {
      if ( !pred( color( this->get_node( fi ) ) ) )
      {
        result = false;
        return false;
      }
      return true;
    } );
    return result;
  }

protected:
  // mutable uint32_t value{0};
}; /* color_view */

/*!\brief Manager view for traversal IDs (out-of-place storage).
 *
 * Traversal IDs, called colors, are unsigned integers that can be
 * assigned to nodes.  The corresponding values are stored
 * out-of-place in this view.
 */
template<typename Ntk>
class out_of_place_color_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit out_of_place_color_view( Ntk const& ntk )
      : Ntk( ntk ), values( ntk.size() )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  }

  uint32_t new_color() const
  {
    return ++value;
  }

  uint32_t current_color() const
  {
    return value;
  }

  void clear_colors( uint32_t color = 0 ) const
  {
    std::for_each( std::begin( values ), std::end( values ),
                   [color]( auto& v ) { v = color; } );
  }

  auto color( node const& n ) const
  {
    return values[n];
  }

  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  auto color( signal const& n ) const
  {
    return values[this->get_node( n )];
  }

  void paint( node const& n ) const
  {
    values[n] = value;
  }

  void paint( node const& n, uint32_t color ) const
  {
    values[n] = color;
  }

  void paint( node const& n, node const& other ) const
  {
    values[n] = values[other];
  }

  /*! \brief Evaluates a predicate on the color of a node */
  template<typename Pred>
  bool eval_color( node const& n, Pred&& pred ) const
  {
    return pred( color( n ) );
  }

  /*! \brief Evaluates a predicate on the colors of two nodes */
  template<typename Pred>
  bool eval_color( node const& a, node const& b, Pred&& pred ) const
  {
    return pred( color( a ), color( b ) );
  }

  /*! \brief Evaluates a predicate on the colors of the fanins of a node */
  template<typename Pred>
  bool eval_fanins_color( node const& n, Pred&& pred ) const
  {
    bool result = true;
    this->foreach_fanin( n, [&]( signal const& fi ) {
      if ( !pred( color( this->get_node( fi ) ) ) )
      {
        result = false;
        return false;
      }
      return true;
    } );
    return result;
  }

protected:
  mutable std::vector<uint32_t> values;
  mutable uint32_t value{ 0 };
}; /* out_of_place_color_view */

} // namespace mockturtle