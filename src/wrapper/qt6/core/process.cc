#include "wrapper/qt6/core/process.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>
#endif

namespace qt6::core {

ProcessWrapper::ProcessWrapper() = default;
ProcessWrapper::~ProcessWrapper() = default;

void ProcessWrapper::setWorkDir(const std::string& dir) { work_dir_ = dir; }
const std::string& ProcessWrapper::workDir() const       { return work_dir_; }
void ProcessWrapper::setEnv(const std::string& k, const std::string& v) { env_.push_back({k, v}); }
void ProcessWrapper::clearEnv() { env_.clear(); }

ProcessResult ProcessWrapper::run(const std::string& program,
                                   const std::vector<std::string>& args,
                                   int timeout_ms) const {
    ProcessResult result;

#if ENGINE_HAS_QT6
    QProcess proc;

    if (!work_dir_.empty())
        proc.setWorkingDirectory(QString::fromUtf8(work_dir_.c_str()));

    if (!env_.empty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const auto& kv : env_)
            env.insert(QString::fromUtf8(kv.first.c_str()),
                       QString::fromUtf8(kv.second.c_str()));
        proc.setProcessEnvironment(env);
    }

    QStringList q_args;
    for (const auto& a : args)
        q_args << QString::fromUtf8(a.c_str());

    proc.start(QString::fromUtf8(program.c_str()), q_args);

    if (!proc.waitForStarted(timeout_ms)) {
        result.error = proc.errorString().toUtf8().constData();
        return result;
    }
    if (!proc.waitForFinished(timeout_ms)) {
        proc.kill();
        result.error = "Process timeout";
        return result;
    }

    result.ok         = true;
    result.exit_code  = proc.exitCode();
    result.stdout_out = proc.readAllStandardOutput().toStdString();
    result.stderr_out = proc.readAllStandardError().toStdString();
#else
    result.error = "Qt6 unavailable";
#endif

    return result;
}

}  // namespace qt6::core
