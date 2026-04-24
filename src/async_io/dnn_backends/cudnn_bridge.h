 
#include <string>

namespace AsyncIO::IO::DNNBackends {

class CuDnnBridge {
public:
    CuDnnBridge();
    ~CuDnnBridge();

    CuDnnBridge(const CuDnnBridge &) = delete;
    CuDnnBridge &operator=(const CuDnnBridge &) = delete;

    bool Initialize();
    void Shutdown();

    bool IsReady() const;
    std::string LastError() const;
    void *Handle() const;

private:
    void *handle_;
    bool ready_;
    std::string lastError_;
};

} // namespace AsyncIO::IO::DNNBackends 