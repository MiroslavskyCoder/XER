#include "weight_distributor.h"

#include "../model_core/dense_layer.h"
#include "error_handler/err_monitor.h"

#include <sstream>
#include <algorithm>

namespace Engine::ModelsBuilder::Weights {

size_t WeightDistributor::Distribute(Core::Model& model,
                                     const std::vector<WeightTensor>& tensors) const {
    if (tensors.empty()) return 0;

    size_t assigned = 0;
    size_t tensor_idx = 0;
    const auto& layers = model.GetLayers();

    for (size_t i = 0; i < layers.size() && tensor_idx < tensors.size(); ++i) {
        auto& layer = layers[i];
        if (layer->GetLayerType() != Core::LayerType::Dense) continue;

        auto* dense = dynamic_cast<Core::DenseLayer*>(layer.get());
        if (!dense) continue;

        // Assign weight tensor (flat values: units * input_units)
        if (tensor_idx < tensors.size()) {
            dense->SetWeights(tensors[tensor_idx].values);
            ++tensor_idx;
            ++assigned;
        }
        // Assign bias tensor if present
        if (tensor_idx < tensors.size()) {
            dense->SetBiases(tensors[tensor_idx].values);
            ++tensor_idx;
        }
    }

    if (assigned == 0) {
        Engine::ErrorHandler::ReportDiagnostic(
            Engine::ErrorHandler::MakeDiagnosticData("warning",
                "WeightDistributor", "No layers received weight tensors"));
    }

    return assigned;
}

std::vector<DistributionPlan> WeightDistributor::PlanDistribution(
    const Core::Model& model,
    const std::vector<WeightTensor>& tensors) const {

    std::vector<DistributionPlan> plan;
    const auto& layers = model.GetLayers();
    size_t tensor_idx = 0;

    for (size_t i = 0; i < layers.size() && tensor_idx < tensors.size(); ++i) {
        if (layers[i]->GetLayerType() != Core::LayerType::Dense) continue;

        DistributionPlan entry;
        entry.layer_index = i;
        entry.layer_name = layers[i]->GetLayerName();

        if (tensor_idx < tensors.size()) {
            entry.tensor_name = tensors[tensor_idx].name;
            entry.param_count = tensors[tensor_idx].values.size();
            entry.assigned = true;
            plan.push_back(entry);
            ++tensor_idx;
        }
        // bias
        if (tensor_idx < tensors.size()) {
            DistributionPlan bias_entry = entry;
            bias_entry.tensor_name = tensors[tensor_idx].name + "_bias";
            bias_entry.param_count = tensors[tensor_idx].values.size();
            plan.push_back(bias_entry);
            ++tensor_idx;
        }
    }

    return plan;
}

std::vector<WeightTensor> WeightDistributor::Collect(const Core::Model& model) const {
    std::vector<WeightTensor> result;
    const auto& layers = model.GetLayers();

    for (size_t i = 0; i < layers.size(); ++i) {
        if (layers[i]->GetLayerType() != Core::LayerType::Dense) continue;
        auto* dense = dynamic_cast<const Core::DenseLayer*>(layers[i].get());
        if (!dense) continue;

        WeightTensor wt;
        wt.name = dense->GetLayerName() + "_weights";
        wt.values = dense->GetWeights();
        if (!wt.values.empty()) {
            wt.shape = {static_cast<uint32_t>(dense->GetInputUnits()),
                        static_cast<uint32_t>(dense->GetUnits())};
            result.push_back(wt);
        }

        WeightTensor bt;
        bt.name = dense->GetLayerName() + "_biases";
        bt.values = dense->GetBiases();
        if (!bt.values.empty()) {
            bt.shape = {static_cast<uint32_t>(dense->GetUnits())};
            result.push_back(bt);
        }
    }

    return result;
}

}  // namespace Engine::ModelsBuilder::Weights
