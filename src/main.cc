#include "audio/audio_core/audio_source_loader.h"
#include "audio/audio_core/audio_buffer_manager.h"
#include "audio/file_io_codecs/codec_ffmpeg_encode_helper.h"
#include "flux/terminal/terminal_output_renderer.h"
#include "flux/terminal/terminal_colors.h"
#include "flux/terminal/terminal_styles.h"
#include "flux/input/input_manager.h"
#include "async_io/hardware_abstraction/gpu_info.h"
#include "async_io/hardware_abstraction/cpu_info.h"
#include "async_io/io_thread_pool.h"
#include "async_io/dnn_backends/cudnn_convolution_engine.h"
#include "audio/effects_rack/custom_effect.h"
#include "audio/dsp_algorithms/dsp_pitch_shifter_granular.h"

#include <absl/strings/str_cat.h>

int main(int argc, char* argv[]) {
    AsyncIO::IO::Hardware::GPUInfoProvider gpu_info_provider;
    AsyncIO::IO::Hardware::CPUInfoProvider cpu_info_provider;

    if (gpu_info_provider.Initialize()) {
        int gpu_count = gpu_info_provider.GetDeviceCount(); 
        
        flux::terminal::WriteLine(
            flux::terminal::OutputStream::kStdout,
            absl::StrCat( 
                flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kCyan),
                flux::terminal::BeginStyleSequence({.bold = true}),
                absl::StrCat("Detected ", gpu_count, " GPU(s):") 
            )
        );

        for (int i = 0; i < gpu_count; ++i) {
            std::string name = gpu_info_provider.GetDeviceName(i);
            uint64_t total_mem = gpu_info_provider.GetTotalMemory(i);
            uint64_t free_mem = gpu_info_provider.GetFreeMemory(i);
            std::string description = gpu_info_provider.GetDeviceDescription(i);

            flux::terminal::WriteLine(
                flux::terminal::OutputStream::kStdout,
                absl::StrCat(
                    "GPU ", i, ": ", name, " - Total Memory: ", total_mem / (1024 * 1024), " MB, Free Memory: ", free_mem / (1024 * 1024), " MB\n",
                    "Description: ", description
                )
            );
        }
         
    }
    
    else {
        flux::terminal::WriteLine(
            flux::terminal::OutputStream::kStderr,
            absl::StrCat(
                flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kRed),
                flux::terminal::BeginStyleSequence({.bold = true}),
                "Failed to initialize GPU info provider."
            )
        );
    }

    if (cpu_info_provider.Initialize()) {
        auto cpu_info = cpu_info_provider.GetInfo();
        auto cores = cpu_info_provider.GetCores();
        uint64_t total_mem = cpu_info_provider.GetTotalMemoryMB();
        uint64_t avail_mem = cpu_info_provider.GetAvailableMemoryMB();

        flux::terminal::WriteLine(
            flux::terminal::OutputStream::kStdout,
            absl::StrCat(
                flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kGreen),
                flux::terminal::BeginStyleSequence({.bold = true}),
                "CPU Information:\n",
                "Vendor: ", cpu_info.vendor, "\n", 
                "Cores: ", cores.size(), "\n",
                "Total Memory: ", total_mem, " MB\n",
                "Available Memory: ", avail_mem, " MB\n"
            )
        );
    }
    else {
        flux::terminal::WriteLine(
            flux::terminal::OutputStream::kStderr,
            absl::StrCat(
                flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kRed),
                flux::terminal::BeginStyleSequence({.bold = true}),
                "Failed to initialize CPU info provider."
            )
        );
    }

    AsyncIO::IO::DNNBackends::CuDnnConvolutionEngine cudnn_engine;

    std::vector<float> input(1 * 3 * 224 * 224, 1.0f); // NCHW
    std::vector<float> kernel(64 * 3 * 7 * 7, 0.1f); // OIHW
    std::vector<float> output(1 * 64 * 112 * 112, 0.0f); // NCHW

    flux::terminal::WriteLine(
        flux::terminal::OutputStream::kStdout,
        absl::StrCat(
            flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kYellow),
            flux::terminal::BeginStyleSequence({.bold = true}),
            "Configuring cuDNN convolution engine..."
        )
    );

    flux::terminal::WriteLine(
        flux::terminal::OutputStream::kStdout,
        absl::StrCat(
            "Input: 1x3x224x224, Kernel: 64x3x7x7, Stride: 2, Padding: 3"
        )
    );

    cudnn_engine.Configure(3, 64, 7, 2, 3);
    cudnn_engine.Forward(input.data(), kernel.data(), nullptr, output.data(), 1, 224, 224);

    flux::terminal::WriteLine(
        flux::terminal::OutputStream::kStdout,
        absl::StrCat(
            flux::terminal::ForegroundSequence(flux::terminal::TerminalColor::kYellow),
            flux::terminal::BeginStyleSequence({.bold = true}),
            "Performed convolution using cuDNN engine."
        )
    );

    flux::terminal::WriteLine(
        flux::terminal::OutputStream::kStdout,
        absl::StrCat(
            "Output[0]: ", output[0], "\n",
            "Output[1]: ", output[1], "\n",
            "Output[2]: ", output[2], "\n"
        )
    );
    return 0;
}