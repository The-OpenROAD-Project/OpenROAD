#include "src/gpl/src/fft.h"

#include <iostream>
#include <memory>
#include <sstream>

#include "gtest/gtest.h"
#include "spdlog/fmt/fmt.h"

namespace {

const int X_MAX = 4;
const int Y_MAX = 4;

float input_data[X_MAX * Y_MAX] = {
    0.0,     // 0, 0
    1.0,     // 1, 0
    2.0,     // 2, 0
    3.0,     // 3, 0
    512.0,   // 0, 1
    513.0,   // 1, 1
    514.0,   // 2, 1
    515.0,   // 3, 1
    1024.0,  // 0, 2
    1025.0,  // 1, 2
    1026.0,  // 2, 2
    1027.0,  // 3, 2
    1536.0,  // 0, 3
    1537.0,  // 1, 3
    1538.0,  // 2, 3
    1539.0,  // 3, 3
};

// clang-format off
float output_data_eForce_first[X_MAX * Y_MAX] = {
       -0.81241744756698608398, // 0, 0
       -1.83704113960266113281, // 1, 0
       -1.83704113960266113281, // 2, 0
       -0.81241744756698608398, // 3, 0
       -0.81241744756698608398, // 0, 1
       -1.83704113960266113281, // 1, 1
       -1.83704113960266113281, // 2, 1
       -0.81241744756698608398, // 3, 1
       -0.81241744756698608398, // 0, 2
       -1.83704113960266113281, // 1, 2
       -1.83704113960266113281, // 2, 2
       -0.81241744756698608398, // 3, 2
       -0.81241744756698608398, // 0, 3
       -1.83704113960266113281, // 1, 3
       -1.83704113960266113281, // 2, 3
       -0.81241744756698608398, // 3, 3
};

float output_data_eForce_second[X_MAX * Y_MAX] = {
     -415.95773315429687500000, // 0, 0
     -415.95773315429687500000, // 1, 0
     -415.95773315429687500000, // 2, 0
     -415.95773315429687500000, // 3, 0
     -940.56506347656250000000, // 0, 1
     -940.56506347656250000000, // 1, 1
     -940.56506347656250000000, // 2, 1
     -940.56506347656250000000, // 3, 1
     -940.56506347656250000000, // 0, 2
     -940.56506347656250000000, // 1, 2
     -940.56506347656250000000, // 2, 2
     -940.56506347656250000000, // 3, 2
     -415.95773315429687500000, // 0, 3
     -415.95773315429687500000, // 1, 3
     -415.95773315429687500000, // 2, 3
     -415.95773315429687500000, // 3, 3
};

float output_data_electroPhi[X_MAX * Y_MAX] = {
    -1215.75781250000000000000, // 0, 0
    -1214.34777832031250000000, // 1, 0
    -1212.42810058593750000000, // 2, 0
    -1211.01806640625000000000, // 3, 0
     -493.78292846679687500000, // 0, 1
     -492.37280273437500000000, // 1, 1
     -490.45324707031250000000, // 2, 1
     -489.04312133789062500000, // 3, 1
      489.04312133789062500000, // 0, 2
      490.45324707031250000000, // 1, 2
      492.37280273437500000000, // 2, 2
      493.78292846679687500000, // 3, 2
     1211.01806640625000000000, // 0, 3
     1212.42810058593750000000, // 1, 3
     1214.34777832031250000000, // 2, 3
     1215.75781250000000000000, // 3, 3
};
// clang-format on

void print_tables(std::unique_ptr<gpl::FFT>& fft)
{
  std::stringstream out_ef_1st;
  std::stringstream out_ef_2nd;
  std::stringstream out_el_phi;

  out_ef_1st << "float output_data_eForce_first[X_MAX * Y_MAX] = {\n";
  out_ef_2nd << "float output_data_eForce_second[X_MAX * Y_MAX] = {\n";
  out_el_phi << "float output_data_electroPhi[X_MAX * Y_MAX] = {\n";
  for (int y = 0; y < Y_MAX; y++) {
    for (int x = 0; x < X_MAX; x++) {
      auto eForce = fft->getElectroForce(x, y);
      auto electroPhi = fft->getElectroPhi(x, y);
      out_ef_1st << fmt::format(
          "    {: 26.20f}, // {:d}, {:d}\n", eForce.first, x, y);
      out_ef_2nd << fmt::format(
          "    {: 26.20f}, // {:d}, {:d}\n", eForce.second, x, y);
      out_el_phi << fmt::format(
          "    {: 26.20f}, // {:d}, {:d}\n", electroPhi, x, y);
    }
  }
  out_ef_1st << "};\n\n";
  out_ef_2nd << "};\n\n";
  out_el_phi << "};\n\n";

  std::cout << out_ef_1st.str();
  std::cout << out_ef_2nd.str();
  std::cout << out_el_phi.str();
}

TEST(FloatFFTTest, Basic)
{
  std::unique_ptr<gpl::FFT> fft(new gpl::FFT(X_MAX, Y_MAX, X_MAX, Y_MAX));

  for (int y = 0; y < Y_MAX; y++) {
    for (int x = 0; x < X_MAX; x++) {
      fft->updateDensity(x, y, input_data[x + y * Y_MAX]);
    }
  }

  fft->doFFT();

  print_tables(fft);

  for (int y = 0; y < Y_MAX; y++) {
    for (int x = 0; x < X_MAX; x++) {
      auto eForce = fft->getElectroForce(x, y);
      auto electroPhi = fft->getElectroPhi(x, y);

      EXPECT_EQ(eForce.first, output_data_eForce_first[x + y * Y_MAX]);
      EXPECT_EQ(eForce.second, output_data_eForce_second[x + y * Y_MAX]);
      EXPECT_EQ(electroPhi, output_data_electroPhi[x + y * Y_MAX]);
    }
  }
}

}  // namespace
