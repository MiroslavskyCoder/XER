#include "metric_cpu_gpu_usage.h"

#include <fstream>
#include <numeric>

#if defined(__linux__)
#include <unistd.h>
#endif

namespace Engine::ModelsBuilder::TrainingMetrics {

namespace {

double MeanBy(const std::vector<ResourceUsageSample>& samples,
							double (*selector)(const ResourceUsageSample&)) {
	if (samples.empty()) {
		return 0.0;
	}
	double total = 0.0;
	for (const auto& sample : samples) {
		total += selector(sample);
	}
	return total / static_cast<double>(samples.size());
}

double CpuSelector(const ResourceUsageSample& sample) {
	return sample.cpu_percent;
}

double GpuSelector(const ResourceUsageSample& sample) {
	return sample.gpu_percent;
}

double MemorySelector(const ResourceUsageSample& sample) {
	return sample.memory_mb;
}

}  // namespace

void CpuGpuUsageMetric::Reset() {
	samples_.clear();
}

ResourceUsageSample CpuGpuUsageMetric::CaptureSample() {
	ResourceUsageSample sample;
	sample.cpu_percent = 0.0;
	sample.gpu_percent = 0.0;
	sample.memory_mb = QueryProcessMemoryMb();
	sample.timestamp = std::chrono::steady_clock::now();
	samples_.push_back(sample);
	return sample;
}

double CpuGpuUsageMetric::MeanCpuPercent() const {
	return MeanBy(samples_, &CpuSelector);
}

double CpuGpuUsageMetric::MeanGpuPercent() const {
	return MeanBy(samples_, &GpuSelector);
}

double CpuGpuUsageMetric::MeanMemoryMb() const {
	return MeanBy(samples_, &MemorySelector);
}

double CpuGpuUsageMetric::QueryProcessMemoryMb() const {
#if defined(__linux__)
	std::ifstream statm_file("/proc/self/statm");
	if (!statm_file.good()) {
		return 0.0;
	}

	long total_pages = 0;
	long resident_pages = 0;
	statm_file >> total_pages >> resident_pages;
	if (resident_pages <= 0) {
		return 0.0;
	}

	const long page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0) {
		return 0.0;
	}
	const double bytes = static_cast<double>(resident_pages) * static_cast<double>(page_size);
	return bytes / (1024.0 * 1024.0);
#else
	return 0.0;
#endif
}

}  // namespace Engine::ModelsBuilder::TrainingMetrics

