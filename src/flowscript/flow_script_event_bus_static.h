#include "flow_script_event_bus.h"

#include <memory>

namespace flow_script_detail {
    class StaticEvent {
    public:
        static std::unique_ptr<flow_script_detail::JsEventBusState> state;
    };

    inline std::unique_ptr<flow_script_detail::JsEventBusState> StaticEvent::state = nullptr;
}