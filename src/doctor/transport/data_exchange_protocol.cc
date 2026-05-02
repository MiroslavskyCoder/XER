#include "transport/data_exchange_protocol.h"

namespace EngineDoctor {

std::string DataExchangeProtocol::Serialize(const Message& msg) {
    return msg.type + ":" + msg.payload;
}

Message DataExchangeProtocol::Deserialize(const std::string& raw) {
    Message msg;
    const auto pos = raw.find(':');
    if (pos == std::string::npos) {
        msg.type = raw;
    } else {
        msg.type = raw.substr(0, pos);
        msg.payload = raw.substr(pos + 1);
    }
    return msg;
}

}  // namespace EngineDoctor
