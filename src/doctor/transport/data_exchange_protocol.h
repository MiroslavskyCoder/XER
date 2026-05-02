#pragma once
#include <string>

namespace EngineDoctor {

struct Message {
    std::string type;
    std::string payload;
};

class DataExchangeProtocol {
public:
    std::string Serialize(const Message& msg);
    Message Deserialize(const std::string& raw);
};

}  // namespace EngineDoctor
