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
  \file db_utils.hpp
  \brief Utility functions for creating the DAGs, costs, and final AQFP databases

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <atomic>
#include <iostream>
#include <thread>

#include <kitty/kitty.hpp>

#include "dag.hpp"
#include "dag_cost.hpp"
#include "dag_gen.hpp"
#include "db_builder.hpp"

namespace mockturtle
{

inline void generate_aqfp_dags( const mockturtle::dag_generator_params& params, const std::string& file_prefix, uint32_t num_threads )
{
  auto t0 = std::chrono::high_resolution_clock::now();

  std::vector<std::ofstream> os;
  for ( auto i = 0u; i < num_threads; i++ )
  {
    auto file_path = fmt::format( "{}_{:02d}.txt", file_prefix, i );
    os.emplace_back( file_path );
    assert( os[i].is_open() );
  }

  auto gen = mockturtle::dag_generator<int>( params, num_threads );

  std::atomic<uint32_t> count = 0u;
  std::vector<std::atomic<uint64_t>> counts_inp( 6u );

  gen.for_each_dag( [&]( const auto& net, uint32_t thread_id ) {
    counts_inp[net.input_slots.size()]++;

    os[thread_id] << fmt::format( "{}\n", net.encode_as_string() );

    if ( (++count) % 100000 == 0u )
    {
      auto t1 = std::chrono::high_resolution_clock::now();
      auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 );

      std::cerr << fmt::format( "Number of DAGs generated {:10d}\nTime so far in seconds {:9.3f}\n", count, d1.count() / 1000.0 );
    } } );

  for ( auto& file : os )
  {
    file.close();
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t0 );
  std::cerr << fmt::format( "Number of DAGs generated {:10d}\nTime elapsed in seconds {:9.3f}\n", count, d2.count() / 1000.0 );

  std::cerr << fmt::format( "Number of DAGs of different input counts: [3 -> {},  4 -> {}, 5 -> {}]\n", counts_inp[3u], counts_inp[4u], counts_inp[5u] );
}

inline void compute_aqfp_dag_costs( const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters,
                                    const std::string& dag_file_prefix, const std::string& cost_file_prefix, uint32_t num_threads )
{
  auto t0 = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;

  std::atomic<uint64_t> count = 0u;

  for ( auto i = 0u; i < num_threads; i++ )
  {
    threads.emplace_back(
        [&]( auto id ) {
          std::ifstream is( fmt::format( "{}_{:02d}.txt", dag_file_prefix, id ) );
          assert( is.is_open() );

          std::ofstream os( fmt::format( "{}_{:02d}.txt", cost_file_prefix, id ) );
          assert( os.is_open() );
          mockturtle::dag_aqfp_cost_all_configs<mockturtle::aqfp_dag<>> cc( gate_costs, splitters );

          std::string temp;
          while ( getline( is, temp ) )
          {
            if ( temp.length() > 0 )
            {
              mockturtle::aqfp_dag<> net( temp );
              auto costs = cc( net );

              os << costs.size() << std::endl;
              for ( auto it = costs.begin(); it != costs.end(); it++ )
              {
                os << fmt::format( "{:08x} {}\n", it->first, it->second );
              }

              if ( ( ++count ) % 100000u == 0u )
              {
                auto t1 = std::chrono::high_resolution_clock::now();
                auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 );

                std::cerr << fmt::format( "Number of DAGs processed {:10d}\nTime so far in seconds {:9.3f}\n", count, d1.count() / 1000.0 );
              }
            }
          }

          is.close();
          os.close();
        },
        i );
  }

  for ( auto& t : threads )
  {
    t.join();
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t0 );
  std::cerr << fmt::format( "Number of DAGs processed {:10d}\nTime elapsed in seconds {:9.3f}\n", count, d2.count() / 1000.0 );
}

inline void generate_aqfp_db( const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters,
                              const std::string& dag_file_prefix, const std::string& cost_file_prefix, const std::string& db_file_prefix, uint32_t num_threads )
{
  auto t0 = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;

  std::atomic<uint64_t> count = 0u;

  for ( auto i = 0u; i < num_threads; i++ )
  {
    threads.emplace_back(
        [&]( auto id ) {
          std::ifstream ds( fmt::format( "{}_{:02d}.txt", dag_file_prefix, id ) );
          std::ifstream cs( fmt::format( "{}_{:02d}.txt", cost_file_prefix, id ) );

          mockturtle::aqfp_db_builder<> db( gate_costs, splitters );
          uint64_t local_count = 0u;

          std::string dag;
          while ( std::getline( ds, dag ) )
          {
            uint32_t num_configs;
            cs >> num_configs;

            std::unordered_map<uint64_t, double> configs;

            std::string config_str;
            uint64_t config;
            double cost;

            for ( auto j = 0u; j < num_configs; j++ )
            {
              cs >> config_str;
              cs >> cost;
              config = std::stoul( config_str, 0, 16 );
              configs[config] = cost;
            }

            mockturtle::aqfp_dag<> ntk( dag );
            if ( ntk.input_slots.size() < 5u || ( ntk.input_slots.size() == 5u && ntk.zero_input != 0 ) )
            {
              db.update( ntk, configs );
            }

            if ( ( ++count ) % 10000 == 0u )
            {
              auto t1 = std::chrono::high_resolution_clock::now();
              auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>( t1 - t0 );

              std::cerr << fmt::format( "Number of DAGs processed {:10d}\nTime so far in seconds {:9.3f}\n", count, d1.count() / 1000.0 );
            }

            if ( ( ++local_count ) % 10000 == 0u )
            {
              db.remove_redundant();

              std::ofstream os_tmp( fmt::format( "{}_{:02d}.txt", db_file_prefix, id ) );
              assert( os_tmp.is_open() );
              db.save_db_to_file( os_tmp );
              os_tmp.close();
            }
          }

          db.remove_redundant();

          std::ofstream os( fmt::format( "{}_{:02d}.txt", db_file_prefix, id ) );
          assert( os.is_open() );
          db.save_db_to_file( os );
          os.close();
        },
        i );
  }

  for ( auto& t : threads )
  {
    t.join();
  }

  mockturtle::aqfp_db_builder<> db( gate_costs, splitters );
  for ( auto i = 0u; i < num_threads; i++ )
  {
    std::ifstream is( fmt::format( "{}_{:02d}.txt", db_file_prefix, i ) );
    assert( is.is_open() );
    db.load_db( is );
    is.close();
  }

  db.remove_redundant();

  std::ofstream os_final( fmt::format( "{}.txt", db_file_prefix ) );
  assert( os_final.is_open() );
  db.save_db_to_file( os_final );
  os_final.close();

  std::ofstream os_final_init_list( fmt::format( "{}_as_initializer_list.txt", db_file_prefix ) );
  assert( os_final_init_list.is_open() );
  db.save_db_to_file( os_final_init_list, true );
  os_final_init_list.close();

  auto t2 = std::chrono::high_resolution_clock::now();
  auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t0 );
  std::cerr << fmt::format( "Number of DAGs processed {:10d}\nTime elapsed in seconds {:9.3f}\n", count, d2.count() / 1000.0 );
}

inline void generate_aqfp_db(
    const mockturtle::dag_generator_params& params,
    const std::unordered_map<uint32_t, double>& gate_costs,
    const std::unordered_map<uint32_t, double>& splitters,
    const std::string& file_prefix,
    uint32_t num_threads )
{
  std::cerr << "Generating DAGs ...\n";
  auto dag_file_prefix = fmt::format( "{}_dags", file_prefix );
  generate_aqfp_dags( params, dag_file_prefix, num_threads );

  std::cerr << "Computing costs ...\n";
  auto cost_file_prefix = fmt::format( "{}_costs", file_prefix );
  compute_aqfp_dag_costs( gate_costs, splitters, dag_file_prefix, cost_file_prefix, num_threads );

  std::cerr << "Generating the database ...\n";
  auto db_file_prefix = fmt::format( "{}_db", file_prefix );
  generate_aqfp_db( gate_costs, splitters, dag_file_prefix, cost_file_prefix, db_file_prefix, num_threads );

  std::cerr << "Generation completed!";
}

} // namespace mockturtle