#define BOOST_TEST_MODULE TestScanArchitect
#include <boost/test/included/unit_test.hpp>
#include <limits>
#include <random>
#include <sstream>

#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"

namespace {

BOOST_AUTO_TEST_SUITE(test_suite)

using namespace dft;

bool operator!=(const ScanArchitect::HashDomainLimits& lhs,
                const ScanArchitect::HashDomainLimits& rhs)
{
  return lhs.chain_count != rhs.chain_count || lhs.max_length != rhs.max_length;
}

template <typename T>
bool CompareUnordered(const T& u1, const T& u2)
{
  for (const auto& [key, value] : u1) {
    auto it = u2.find(key);
    if (it == u2.end()) {
      return false;
    }
    if (it->second != value) {
      return false;
    }
  }
  return true;
}

ScanArchitect::HashDomainLimits CreateTestHashDomainLimits(uint64_t chain_count,
                                                           uint64_t max_length)
{
  ScanArchitect::HashDomainLimits limits;
  limits.chain_count = chain_count;
  limits.max_length = max_length;
  return limits;
}

// Check if we are correctly calculating the limits of the hash domains
BOOST_AUTO_TEST_CASE(test_calculate_chain_count_and_max_length)
{
  // For 1 hash domain (1) with 10 bits and a max length of 5, we need to have a
  // chain count of 2 in that hash domain (1)
  BOOST_TEST(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}}, 5),
      {{1, CreateTestHashDomainLimits(2, 5)}}));
  // Unbalance cases
  // We create 2 chains with max_length of 4
  BOOST_TEST(
      CompareUnordered(ScanArchitect::inferChainCountFromMaxLength({{1, 9}}, 5),
                       {{1, CreateTestHashDomainLimits(2, 5)}}));
  // We create 2 chains with max_length of 3
  BOOST_TEST(
      CompareUnordered(ScanArchitect::inferChainCountFromMaxLength({{1, 6}}, 5),
                       {{1, CreateTestHashDomainLimits(2, 3)}}));

  // Two hash domains, balance case
  BOOST_TEST(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 10}}, 5),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(2, 5)}}));
  // Unbalance
  BOOST_TEST(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 9}}, 5),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(2, 5)}}));

  //// Two hash domains with different number of cells
  BOOST_TEST(CompareUnordered(
      ScanArchitect::inferChainCountFromMaxLength({{1, 10}, {2, 15}}, 5),
      {{1, CreateTestHashDomainLimits(2, 5)},
       {2, CreateTestHashDomainLimits(3, 5)}}));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
