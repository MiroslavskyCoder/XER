#include "hw_topology_map.h"

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

namespace AsyncIO::IO::Hardware {

HardwareTopologyMap::HardwareTopologyMap()
    : core_count_(0), thread_count_(0), socket_count_(1), numa_node_count_(1), initialized_(false) {}

HardwareTopologyMap::~HardwareTopologyMap() {}

bool HardwareTopologyMap::Initialize() {
    DiscoverTopology();
    initialized_ = true;
    return true;
}

bool HardwareTopologyMap::IsInitialized() const {
    return initialized_;
}

std::vector<TopologyObject> HardwareTopologyMap::GetTopology(TopologyLevel level) const {
    std::vector<TopologyObject> objects;
    for (const auto& pair : topology_objects_) {
        if (pair.second.level == level) {
            objects.push_back(pair.second);
        }
    }
    return objects;
}

TopologyObject HardwareTopologyMap::GetObject(int id) const {
    auto it = topology_objects_.find(id);
    if (it != topology_objects_.end()) {
        return it->second;
    }
    return TopologyObject();
}

int HardwareTopologyMap::GetCoreCount() const {
    return core_count_;
}

int HardwareTopologyMap::GetThreadCount() const {
    return thread_count_;
}

int HardwareTopologyMap::GetSocketCount() const {
    return socket_count_;
}

int HardwareTopologyMap::GetNUMANodeCount() const {
    return numa_node_count_;
}

TopologyObject HardwareTopologyMap::GetParent(const TopologyObject& obj) const {
    return GetObject(obj.parent_id);
}

std::vector<TopologyObject> HardwareTopologyMap::GetChildren(const TopologyObject& obj) const {
    std::vector<TopologyObject> children;
    for (int child_id : obj.child_ids) {
        children.push_back(GetObject(child_id));
    }
    return children;
}

std::vector<TopologyObject> HardwareTopologyMap::GetCacheHierarchy() const {
    return GetTopology(TopologyLevel::CACHE_GROUP);
}

uint64_t HardwareTopologyMap::GetCacheSize(TopologyLevel level) const {
    auto caches = GetTopology(level);
    if (!caches.empty()) {
        return caches[0].cache_size_kb;
    }
    return 0;
}

int HardwareTopologyMap::GetNUMANode(int logical_processor) const {
#ifdef _WIN32
    // Windows NUMA detection would go here
    return 0;
#elif defined(__linux__)
    // Linux NUMA detection would go here
    return 0;
#else
    return 0;
#endif
}

std::vector<int> HardwareTopologyMap::GetProcsOnNUMA(int numa_node) const {
    std::vector<int> procs;
    // Placeholder for NUMA processor discovery
    return procs;
}

std::string HardwareTopologyMap::GetTopologyString() const {
    std::string result;
    result += "Sockets: " + std::to_string(socket_count_) + "\n";
    result += "NUMA Nodes: " + std::to_string(numa_node_count_) + "\n";
    result += "Cores: " + std::to_string(core_count_) + "\n";
    result += "Threads: " + std::to_string(thread_count_) + "\n";
    return result;
}

void HardwareTopologyMap::DiscoverTopology() {
#ifdef _WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    
    thread_count_ = sys_info.dwNumberOfProcessors;
    core_count_ = thread_count_ / 2;  // Simplified
    socket_count_ = 1;
    numa_node_count_ = 1;
#elif defined(__linux__)
    thread_count_ = sysconf(_SC_NPROCESSORS_ONLN);
    core_count_ = thread_count_ / 2;  // Simplified
    socket_count_ = 1;
    numa_node_count_ = 1;
#endif

    // Create basic topology objects
    TopologyObject machine;
    machine.level = TopologyLevel::MACHINE;
    machine.id = 0;
    machine.parent_id = -1;
    topology_objects_[0] = machine;
}

}  // namespace AsyncIO::IO::Hardware
