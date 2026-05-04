#include "content/ai/sd_base/generate/sd_pipeline.h"

#include "content/ai/models_builder/utility/mb_logger.h"
#include "content/ai/ml/utils/ml_random_generator.h"
#include "content/ai/sd_base/controlnet/sd_controlnet_router.h"
#include "content/ai/sd_base/depth/sd_depth_conditioner.h"
#include "content/ai/sd_base/fast/sd_fast_path.h"
#include "content/ai/sd_base/flow/sd_flow_context.h"
#include "content/ai/sd_base/gpu/sd_gpu_dispatch.h"
#include "content/ai/sd_base/layer/sd_text_encoder_stub.h"
#include "content/ai/sd_base/seed/sd_seed_generator.h"
#include "content/ai/sd_base/tensor/sd_latent_tensor.h"
#include "content/ai/sd_base/to/sd_image_decoder.h"
#include "content/ai/sd_base/xl/sd_xl_prompt_expander.h"

#include <algorithm>

namespace Engine::AI::SDBase {

SdPipeline::SdPipeline(SdBaseConfig config)
    : config_(std::move(config)) {
}

SdGenerationResult SdPipeline::Generate(const SdGenerationRequest& request) {
    SdGenerationResult result;
    std::string cfg_error;
    if (!config_.Validate(&cfg_error)) {
        result.error = cfg_error;
        return result;
    }

    const auto caps = SdGpuDispatch::Detect();
    SdFlowContext flow;
    flow.request = request;
    flow.caps = caps;

    auto& log = Engine::ModelsBuilder::Utility::ModelBuilderLogger::GetInstance();
    log.Info("SDBase: generation started");

    SdGenerationRequest req = request;
    req.width = std::min(req.width, config_.max_width);
    req.height = std::min(req.height, config_.max_height);
    req.steps = std::clamp(SdFastPath::ResolveStepCount(req), config_.min_steps, config_.max_steps);

    SdXlPromptExpander prompt_expander;
    const std::string expanded_prompt = prompt_expander.Expand(req.prompt);

    const uint64_t seed = SdSeedGenerator::FromPrompt(expanded_prompt, req.negative_prompt, req.seed);
    Engine::ML::Utils::MlRandomGenerator rng(seed);

    scheduler_.BuildLinearSchedule(req.steps);

    SdTextEncoderStub text_encoder;
    auto text_embedding = text_encoder.Encode(expanded_prompt, 77, config_.text_embedding_dim);
    (void)text_embedding;

    auto latent = SdLatentTensor::Make(req.width, req.height);
    SdLatentTensor::FillGaussian(latent, rng, config_.init_noise_sigma);

    SdControlNetRouter controlnet;
    SdDepthConditioner depth;

    float* latent_data = static_cast<float*>(latent.GetData());
    for (uint32_t t = 0; t < req.steps; ++t) {
        const float sigma = scheduler_.SigmaAt(t);
        const float beta = scheduler_.BetaAt(t);
        const float guidance = std::clamp(req.guidance_scale, 0.0f, 30.0f);

        const uint64_t n = latent.GetElementCount();
        for (uint64_t i = 0; i < n; ++i) {
            latent_data[i] -= beta * (latent_data[i] + 0.01f * guidance * sigma);
        }

        if ((t % 8) == 0) {
            controlnet.ApplyHintScale(latent, 0.05f);
            depth.ApplyDepthPrior(latent, 0.10f);
        }
    }

    SdImageDecoder decoder;
    auto image = decoder.DecodeLatentToImage(latent, req.width, req.height);

    result.ok = true;
    result.latent = std::move(latent);
    result.image = std::move(image);
    result.metadata["seed"] = std::to_string(seed);
    result.metadata["steps"] = std::to_string(req.steps);
    result.metadata["cuda"] = caps.cuda_available ? "true" : "false";
    result.metadata["opencv"] = caps.opencv_available ? "true" : "false";
    result.metadata["ffmpeg"] = caps.ffmpeg_available ? "true" : "false";
    result.metadata["elapsed_ms"] = std::to_string(flow.ElapsedMs());

    log.Info("SDBase: generation finished");
    return result;
}

}  // namespace Engine::AI::SDBase
