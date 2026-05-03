#include "val_metrics.h"
#include "val_accuracy.h"
#include "val_f1score.h"
#include "val_precision.h"
#include "val_recall.h"
#include <sstream>
#include <iomanip>

namespace Engine::ML::Validation {

std::string ValidationMetrics::ToString() const {
    std::ostringstream os;
    os << std::fixed << std::setprecision(4)
       << "Accuracy="   << accuracy
       << " Precision=" << precision
       << " Recall="    << recall
       << " F1="        << f1_score
       << " MacroF1="   << macro_f1
       << " WeightedF1=" << weighted_f1
       << " N="         << n_samples;
    return os.str();
}

ValidationMetrics ValMetrics::Compute(const std::vector<int>& predictions,
                                       const std::vector<int>& ground_truth,
                                       int positive_class) {
    ValidationMetrics m;
    m.n_samples    = ground_truth.size();
    m.accuracy     = ValAccuracy::Compute(predictions, ground_truth);
    m.precision    = ValPrecision::Compute(predictions, ground_truth, positive_class);
    m.recall       = ValRecall::Compute(predictions, ground_truth, positive_class);
    m.f1_score     = ValF1Score::Compute(predictions, ground_truth, positive_class);
    m.macro_f1     = ValF1Score::MacroAverage(predictions, ground_truth);
    m.weighted_f1  = ValF1Score::WeightedAverage(predictions, ground_truth);
    return m;
}

}  // namespace Engine::ML::Validation
