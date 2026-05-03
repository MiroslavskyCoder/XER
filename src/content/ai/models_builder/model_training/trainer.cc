#include "trainer.h"

#include "../model_inference/predictor.h"
#include "../utility/mb_error_handler.h"
#include "../utility/mb_logger.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>
#include <random>

namespace Engine::ModelsBuilder::Training {

namespace {

void ShuffleIndices(std::vector<size_t>& indices) {
    boost::random::mt19937 generator(static_cast<uint32_t>(std::random_device{}()));
    for (size_t index = indices.size(); index > 1U; --index) {
        boost::random::uniform_int_distribution<size_t> distribution(0U, index - 1U);
        std::swap(indices[index - 1U], indices[distribution(generator)]);
    }
}

} // namespace

Trainer::Trainer(std::shared_ptr<Core::Model> model)
    : model_(model) {
}

void Trainer::SetOptimizer(std::shared_ptr<Optimization::Optimizer> optimizer) {
    optimizer_ = optimizer;
}

void Trainer::SetLossFunction(std::shared_ptr<Optimization::LossFunction> loss) {
    loss_ = loss;
}

bool Trainer::Train(const std::vector<float>& x_train, const std::vector<float>& y_train,
                    const TrainingConfig& config) {
    if (!model_ || !optimizer_ || !loss_) {
        Utility::ModelBuilderErrorHandler::GetInstance().ReportError(
            Utility::ModelBuilderErrorCode::InvalidArgument,
            "Trainer requires model, optimizer, and loss function.",
            "ModelsBuilder::Training::Trainer::Train");
        return false;
    }

    if (!model_->IsCompiled()) {
        Utility::ModelBuilderErrorHandler::GetInstance().ReportError(
            Utility::ModelBuilderErrorCode::CompilationFailed,
            "Model must be compiled before training.",
            "ModelsBuilder::Training::Trainer::Train");
        return false;
    }

    if (x_train.empty() || y_train.empty() || x_train.size() != y_train.size()) {
        Utility::ModelBuilderErrorHandler::GetInstance().ReportError(
            Utility::ModelBuilderErrorCode::InvalidArgument,
            "Training data must be non-empty and aligned by sample count.",
            "ModelsBuilder::Training::Trainer::Train");
        return false;
    }

    if (config.epochs == 0U || config.batch_size == 0U) {
        Utility::ModelBuilderErrorHandler::GetInstance().ReportError(
            Utility::ModelBuilderErrorCode::InvalidArgument,
            "Training config requires positive epochs and batch size.",
            "ModelsBuilder::Training::Trainer::Train");
        return false;
    }

    optimizer_->SetLearningRate(config.learning_rate);
    epoch_losses_.clear();
    best_loss_ = std::numeric_limits<float>::infinity();
    uint32_t stale_epochs = 0U;

    Inference::Predictor predictor(model_);
    std::vector<size_t> indices(x_train.size());
    std::iota(indices.begin(), indices.end(), 0U);

    for (uint32_t epoch = 0; epoch < config.epochs; ++epoch) {
        if (config.shuffle_data) {
            ShuffleIndices(indices);
        }

        using MeanAccumulator = boost::accumulators::accumulator_set<
            float,
            boost::accumulators::features<boost::accumulators::tag::mean>>;

        MeanAccumulator epoch_accumulator;
        const size_t batch_size = std::min<size_t>(config.batch_size, indices.size());
        for (size_t batch_begin = 0; batch_begin < indices.size(); batch_begin += batch_size) {
            const size_t batch_end = std::min(indices.size(), batch_begin + batch_size);
            MeanAccumulator batch_accumulator;

            for (size_t cursor = batch_begin; cursor < batch_end; ++cursor) {
                const size_t sample_index = indices[cursor];
                std::vector<float> prediction = predictor.Predict({x_train[sample_index]});
                if (prediction.empty()) {
                    prediction.push_back(0.0f);
                }

                std::vector<float> target(prediction.size(), y_train[sample_index]);
                const float loss_value = loss_->Compute(
                    prediction.data(),
                    target.data(),
                    prediction.size());
                batch_accumulator(loss_value);
                epoch_accumulator(loss_value);
            }

            if (config.verbose) {
                Utility::ModelBuilderLogger::GetInstance().Debug(
                    "Epoch " + std::to_string(epoch) +
                    " batch loss=" + std::to_string(boost::accumulators::mean(batch_accumulator)));
            }
        }

        const float epoch_loss = boost::accumulators::mean(epoch_accumulator);
        epoch_losses_.push_back(epoch_loss);

        const bool is_first_epoch = std::isinf(best_loss_);
        const bool improved = is_first_epoch ||
            (best_loss_ - epoch_loss) >= std::max(0.0f, config.early_stopping_min_delta);
        if (improved) {
            best_loss_ = epoch_loss;
            stale_epochs = 0U;
        } else {
            ++stale_epochs;
        }

        float running_mean = 0.0f;
        if (!epoch_losses_.empty()) {
            running_mean = std::accumulate(epoch_losses_.begin(), epoch_losses_.end(), 0.0f)
                         / static_cast<float>(epoch_losses_.size());
        }

        if (config.verbose) {
            Utility::ModelBuilderLogger::GetInstance().Info(
                "Epoch " + std::to_string(epoch) +
                " optimizer=" + optimizer_->GetOptimizerName() +
                " lr=" + std::to_string(optimizer_->GetLearningRate()) +
                " loss=" + std::to_string(epoch_loss) +
                " running_loss=" + std::to_string(running_mean));
        }

        if (callback_) {
            callback_(epoch, epoch_loss);
        }

        if (config.early_stopping_patience > 0U && stale_epochs >= config.early_stopping_patience) {
            Utility::ModelBuilderLogger::GetInstance().Info(
                "Early stopping triggered at epoch " + std::to_string(epoch) +
                ", best_loss=" + std::to_string(best_loss_));
            break;
        }
    }
    
    return true;
}

} // namespace Engine::ModelsBuilder::Training
