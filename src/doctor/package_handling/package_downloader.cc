#include "package_handling/package_downloader.h"
#include "package_handling/remote_package_source.h"

namespace EngineDoctor {

bool PackageDownloader::Download(const std::string& url,
                                 const std::string& dest) {
    RemotePackageSource source;
    return source.Download(url, dest);
}

}  // namespace EngineDoctor
