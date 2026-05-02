#pragma once
#include <functional>
#include <string>
#include <unordered_map>
namespace EngineDoctor {
class DiagnosticManager {
public:
    void Attach(const std::string& name, std::function<std::string()> check);
    std::unordered_map<std::string,std::string> Collect();
private:
    std::unordered_map<std::string,std::function<std::string()>> checks_;
};
}
