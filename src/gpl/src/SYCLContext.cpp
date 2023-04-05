#include "syclContext.h"

namespace gpl {

SYCLContext::SYCLContext(utl::Logger* logger) : 
    logger_(logger) 
{
    try{ 
        this.queue_ = sycl::queue(gpu_selector {}); //or accelerator_selector {} for GPU/FPGA
    } catch(sycl::runtime_error e) {
        logger_.error("Could not find or initialize a valid SYCL device.");
        throw e; // At some point, change to fall back to CPU.
    }

    logger_.info("Initialized SYCL device: {}.", this.queue.get_device().get_info<info::device::name>());
}

sycl::

sycl::buffer<float, 2> SYCLContext::performDCT2D(int cnt_x, int cnt_y, sycl::buffer<float, 2> input){
    // Based on https://dsp.stackexchange.com/questions/2807/fast-cosine-transform-via-fft

    bbft::configuration horiz_dct = {
        1,
        {cnt_x * 2, 1, 1},
        bbfft::precision::f32,
        direction::forward,
        bbfft::transform_type::r2c
    };

    bbfft::plan plan_horiz = bbfft::make_plan(horiz_dct, this.queue_);
    float* proc_input = sycl::malloc_device<float>(cnt_y * cnt_x * 2);
    sycl::buffer<float, 2> proc_input_buf(sycl::range<2>{cnt_y, cnt_x * 2}, proc_input);

    auto input_preproc_ev = this.queue_.submit([&](handler& cgh) {
        sycl::accessor raw_acc {input, cgh, read_only};
        sycl::accessor proc_acc {proc_input, cgh, write_only};

        cgh.parallel_for(Range<2>{cnt_y, cnt_x * 2}, [=](id<2> idx) {
            if(idx[1] < cnt_x)
                proc_acc[idx] = raw_acc[idx];
            else 
                proc_acc[idx] = 0;
        });

    });

    // Perform horizontal DFT (Note the output is complex)
    std::complex<float>* dct_step1 = sycl::malloc_device<std::complex<float>>(cnt_y * cnt_x * 2);
    sycl::buffer<std::complex<float>, 2> dct_step1_buf(syc::range<2>{cnt_y, cnt_x * 2}, dct_step1);
    auto dct1_ev;
    for(int row = 0; row < cnt_y; row++) {  
        dct1_ev = plan_horiz.execute(proc_input, reinterpret_cast<float**>(dct_step1), input_preproc_ev);
    }
    this.queue_.wait();

    std::complex<float>* dct1_stripped = sycl::malloc_device<std::complex<float>>(cnt_y * cnt_x * 2);
    sycl::buffer<std::complex<float>, 2> dct1_stripped_buf(syc::range<2>{cnt_y, cnt_x}, dct1_stripped);
    // Remove all conjugates (keeping half of each row).
    auto cong_strip_ev = this.queue_.submit([&](handler& cgh) {
        sycl::accessor original_acc {dct_step1_buf, cgh, read_only};
        sycl::accessor stripped_acc {dct1_stripped_buf, cgh, write_only};

        cgh.parallel_for(Range<2>{cnt_y, cnt_x}, [=](id<2> idx) {
            std::complex<float> pwr (0, (-1*M_1_PI*idx[1])/(2*cnt_x));
            stripped_acc[idx] = 2 * original_acc[idx] * std::exp(pwr);
        });
    });

    float* transposed = sycl::malloc_device<std::complex<>>(cnt_y * cnt_x);
    sycl::buffer<float, 2> transposed_buf(syc::range<2>{cnt_x, cnt_y}, transposed);
    auto transpose_ev = this.queue_.submit([&](handler& cgh) {
        cgh.depends_on(cong_strip_ev);
        sycl::accessor in_acc {stripped_dct_buf, cgh, read_only};
        sycl::accessor trans_acc {transposed_buf, cgh, write_only};

        cgh.parallel_for(Range<2>{cnt_y, cnt_x}, [=](id<2> idx) {
            trans_acc[idx[1]][idx[0]] = in_acc[idx];
        });
    });


}

void SYCLContext::planFFTs() {


}

}