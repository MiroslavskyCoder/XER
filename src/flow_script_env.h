
#include <map>
#include <memory>
#include <string>

struct FlowScriptEnvData
{
    std::map<std::string, std::string> env;
};


class FlowScriptEnv {
public:
    FlowScriptEnv(FlowScriptEnvData* data);

    const FlowScriptEnvData& data() const;

private:
    std::unique_ptr<FlowScriptEnvData> data_;
};