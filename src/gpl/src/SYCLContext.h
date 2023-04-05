#pragma once

#include <complex>
#include <SYCL/sycl.hpp>
#include <bbfft/configuration.hpp>
#include <bbfft/plan.hpp>
#include "utl/Logger.h"

namespace gpl {

class SYCLContext {
    public: 


        
    SYCLContext(utl::Logger* logger);

    sycl::buffer<float, 2> performDCT2D(sycl::buffer<float, 2> buf);
    sycl::buffer<float, 2> performIDCT2D(sycl::buffer<float, 2> buf);
    // Discrete Cosine-Sine Transform
    sycl::buffer<float, 2> performIDCST2D(sycl::buffer<float, 2> buf);
    // Discrete Sine-Cosine Transform
    sycl::buffer<float, 2> performIDSCT2D();

    private:
        utl::Logger* logger_;

        sycl::queue queue_;
        bbfft:plan<sycl::event> fft_idct1d_;
        bbfft:plan<sycl::event> fft_idst1d_;

}