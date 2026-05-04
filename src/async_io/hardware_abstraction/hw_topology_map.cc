#include "hw_topology_map.h"

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fstream>
    #include <sstream>
    #include <dirent.h>
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
    // /sys/devices/system/node/nodeX/cpulist contains comma/range cpu lists
    for (int n = 0; n < numa_node_count_; ++n) {
        const std::string cpulist_path =
            "/sys/devices/system/node/node" + std::to_string(n) + "/cpulist";
        std::ifstream f(cpulist_path);
        if (!f.is_open()) continue;
        std::string cpulist;
        std::getline(f, cpulist);
        // Parse comma-separated ranges like "0-3,8-11"
        std::istringstream iss(cpulist);
        std::string token;
        while (std::getline(iss, token, ',')) {
            const auto dash = token.find('-');
            if (dash == std::string::npos) {
                try {
                    if (std::stoi(token) == logical_processor) return n;
                } catch (...) {}
            } else {
                try {
                    int lo = std::stoi(token.substr(0, dash));
                    int hi = std::stoi(token.substr(dash + 1));
                    if (logical_processor >= lo && logical_processor <= hi) return n;
                } catch (...) {}
            }
        }
    }
    return 0;
#else
    return 0;
#endif
}

std::vector<int> HardwareTopologyMap::GetProcsOnNUMA(int numa_node) const {
    std::vector<int> procs;
#ifdef __linux__
    const std::string cpulist_path =
        "/sys/devices/system/node/node" + std::to_string(numa_node) + "/cpulist";
    std::ifstream f(cpulist_path);
    if (f.is_open()) {
        std::string cpulist;
        std::getline(f, cpulist);
        std::istringstream iss(cpulist);
        std::string token;
        while (std::getline(iss, token, ',')) {
            const auto dash = token.find('-');
            if (dash == std::string::npos) {
                try { procs.push_back(std::stoi(token)); } catch (...) {}
            } else {
                try {
                    int lo = std::stoi(token.substr(0, dash));
                    int hi = std::stoi(token.substr(dash + 1));
                    for (int i = lo; i <= hi; ++i) procs.push_back(i);
                } catch (...) {}
            }
        }
    }
#endif
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

    // Physical core count from /proc/cpuinfo
    {
        std::ifstream cpuinfo("/proc/cpuinfo");
        int max_core_id = -1;
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.rfind("core id", 0) == 0) {
                const auto colon = line.find(':');
                if (colon != std::string::npos) {
                    try {
                        int cid = std::stoi(line.substr(colon + 1));
                        if (cid > max_core_id) max_core_id = cid;
                    } catch (...) {}
                }
            }
        }
        core_count_ = (max_core_id >= 0) ? (max_core_id + 1) : thread_count_;
    }

    // Count NUMA nodes from /sys/devices/system/node/
    {
        numa_node_count_ = 0;
        for (int n = 0; n < 64; ++n) {
            std::ifstream f("/sys/devices/system/node/node" +
                            std::to_string(n) + "/cpulist");
            if (!f.is_open()) break;
            ++numa_node_count_;
        }
        if (numa_node_count_ == 0) numa_node_count_ = 1;
    }

    // Count sockets from /sys/devices/system/cpu/cpuX/topology/physical_package_id
    {
        int max_pkg = 0;
        for (int cpu = 0; cpu < thread_count_; ++cpu) {
            std::ifstream pf("/sys/devices/system/cpu/cpu" +
                             std::to_string(cpu) +
                             "/topology/physical_package_id");
            if (!pf.is_open()) break;
            int pkg = 0;
            pf >> pkg;
            if (pkg > max_pkg) max_pkg = pkg;
        }
        socket_count_ = max_pkg + 1;
    }
#endif

    // Machine root object
    TopologyObject machine;
    machine.level = TopologyLevel::MACHINE;
    machine.id = 0;
    machine.parent_id = -1;
    topology_objects_[0] = machine;

    // Per-logical-CPU leaf objects
    for (int cpu = 0; cpu < thread_count_; ++cpu) {
        TopologyObject core_obj;
        core_obj.level = TopologyLevel::CORE;
        core_obj.id = 1000 + cpu;
        core_obj.parent_id = 0;
        topology_objects_[1000 + cpu] = core_obj;
        topology_objects_[0].child_ids.push_back(1000 + cpu);
    }
}

}  // namespace AsyncIO::IO::Hardware
