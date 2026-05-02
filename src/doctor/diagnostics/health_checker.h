#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
namespace EngineDoctor {
enum class HealthStatus { OK, DEGRADED, CRITICAL };
struct HealthReport { HealthStatus status; std::vector<std::string> issues; };
class HealthChecker {
public:
    void AddCheck(const std::string& name, std::function<bool()> check);
    HealthReport Run();
private:
    std::unordered_map<std::string,std::function<bool()>> checks_;
};
}
