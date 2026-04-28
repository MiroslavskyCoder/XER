const f = {
"core": [
      "engine_doctor_core.h",
      "engine_doctor_core.cc",
      "engine_doctor_context.h",
      "engine_doctor_context.cc",
      "engine_doctor_config.h",
      "engine_doctor_config.cc",
      "engine_doctor_factory.h",
      "engine_doctor_factory.cc",
      "module_manager.h",
      "module_manager.cc",
      "event_bus.h",
      "event_bus.cc",
      "logger.h",
      "logger.cc",
      "exceptions.h",
      "common_types.h"
    ],
    "scanner": [
      "file_scanner.h",
      "file_scanner.cc",
      "directory_scanner.h",
      "directory_scanner.cc",
      "scan_context.h",
      "scan_context.cc",
      "scan_filter.h",
      "scan_filter.cc",
      "scan_result_aggregator.h",
      "scan_result_aggregator.cc",
      "file_visitor.h",
      "directory_visitor.h",
      "file_metadata_extractor.h",
      "file_metadata_extractor.cc",
      "path_utils.h",
      "path_utils.cc",
      "scan_parameters.h",
      "scan_result.h",
      "scanner_module.h",
      "scanner_module.cc"
    ],
    "analysis": [
      "analysis_engine.h",
      "analysis_engine.cc",
      "analysis_task_manager.h",
      "analysis_task_manager.cc",
      "analysis_result_handler.h",
      "analysis_result_handler.cc",
      "analysis_strategy.h",
      "analysis_strategy.cc",
      "analysis_pipeline.h",
      "analysis_pipeline.cc",
      "data_provider.h",
      "data_provider.cc",
      "metric_calculator.h",
      "metric_calculator.cc"
    ],
    "classification": [
      "classifier_manager.h",
      "classifier_manager.cc",
      "classification_rule.h",
      "classification_rule.cc",
      "base_classifier.h",
      "base_classifier.cc",
      "no_correct_classifier.h",
      "no_correct_classifier.cc",
      "null_empty_classifier.h",
      "null_empty_classifier.cc",
      "no_0_file_classifier.h",
      "no_0_file_classifier.cc",
      "file_no_empty_classifier.h",
      "file_no_empty_classifier.cc",
      "directory_classifier.h",
      "directory_classifier.cc",
      "file_call_failed_classifier.h",
      "file_call_failed_classifier.cc",
      "fatal_stack_error_classifier.h",
      "fatal_stack_error_classifier.cc",
      "classification_context.h",
      "classification_context.cc",
      "classification_result.h",
      "classification_result.cc"
    ],
    "dependency_management": [
      "dependency_analyzer.h",
      "dependency_analyzer.cc",
      "dependency_graph.h",
      "dependency_graph.cc",
      "conflict_detector.h",
      "conflict_detector.cc",
      "v8_dependency_resolver.h",
      "v8_dependency_resolver.cc",
      "v8_conflict_resolver.h",
      "v8_conflict_resolver.cc",
      "package_dependency.h",
      "package_dependency.cc",
      "dependency_resolver_interface.h",
      "dependency_resolver_interface.cc",
      "dependency_info.h"
    ],
    "system_analysis": {
      "base": [
        "system_analyzer.h",
        "system_analyzer.cc",
        "os_detection_manager.h",
        "os_detection_manager.cc",
        "os_detector_interface.h",
        "os_detector_interface.cc",
        "common_system_info.h",
        "common_system_info.cc",
        "system_requirement_checker.h",
        "system_requirement_checker.cc"
      ],
      "linux": [
        "linux_os_detector.h",
        "linux_os_detector.cc",
        "linux_package_manager.h",
        "linux_package_manager.cc",
        "linux_system_info.h",
        "linux_system_info.cc"
      ],
      "windows": [
        "windows_os_detector.h",
        "windows_os_detector.cc",
        "windows_package_manager.h",
        "windows_package_manager.cc",
        "windows_system_info.h",
        "windows_system_info.cc"
      ],
      "macos": [
        "macos_os_detector.h",
        "macos_os_detector.cc",
        "macos_package_manager.h",
        "macos_package_manager.cc",
        "macos_system_info.h",
        "macos_system_info.cc"
      ],
      "openbsd": [
        "openbsd_os_detector.h",
        "openbsd_os_detector.cc",
        "openbsd_package_manager.h",
        "openbsd_package_manager.cc",
        "openbsd_system_info.h",
        "openbsd_system_info.cc"
      ]
    },
    "sandbox": [
      "sandbox_manager.h",
      "sandbox_manager.cc",
      "execution_environment.h",
      "execution_environment.cc",
      "sandbox_policy.h",
      "sandbox_policy.cc",
      "resource_limiter.h",
      "resource_limiter.cc",
      "sandbox_executor.h",
      "sandbox_executor.cc",
      "sandbox_safe.h"
    ],
    "transport": [
      "transport_manager.h",
      "transport_manager.cc",
      "data_exchange_protocol.h",
      "data_exchange_protocol.cc",
      "secure_communication_manager.h",
      "secure_communication_manager.cc",
      "data_metadata.h",
      "data_metadata.cc",
      "channel_factory.h",
      "channel_factory.cc",
      "observer_registry.h",
      "observer_registry.cc",
      "transport_weak_observer.h",
      "transport_weak_observer.cc"
    ],
    "package_handling": [
      "package_fetcher.h",
      "package_fetcher.cc",
      "package_filter.h",
      "package_filter.cc",
      "package_installer.h",
      "package_installer.cc",
      "package_validator.h",
      "package_validator.cc",
      "package_resolver.h",
      "package_resolver.cc",
      "remote_package_source.h",
      "remote_package_source.cc",
      "local_package_source.h",
      "local_package_source.cc",
      "package_downloader.h",
      "package_downloader.cc",
      "scan_allow_packages_arch.h",
      "scan_allow_packages_arch.cc",
      "filter_engine_pack_a.h",
      "filter_engine_pack_a.cc"
    ],
    "utilities": [
      "string_utils.h",
      "string_utils.cc",
      "file_utils.h",
      "file_utils.cc",
      "time_utils.h",
      "time_utils.cc",
      "config_parser.h",
      "config_parser.cc",
      "command_line_parser.h",
      "command_line_parser.cc"
    ],
    "v8_integration": [
      "v8_engine.h",
      "v8_engine.cc",
      "v8_initializer.h",
      "v8_initializer.cc",
      "v8_runtime_checker.h",
      "v8_runtime_checker.cc",
      "v8_memory_manager.h",
      "v8_memory_manager.cc",
      "v8_script_runner.h",
      "v8_script_runner.cc",
      "v8_exception_handler.h",
      "v8_exception_handler.cc"
    ],
    "diagnostics": [
      "diagnostic_manager.h",
      "diagnostic_manager.cc",
      "performance_monitor.h",
      "performance_monitor.cc",
      "resource_monitor.h",
      "resource_monitor.cc",
      "health_checker.h",
      "health_checker.cc"
    ],
    "report": [
      "report_generator_interface.h",
      "report_generator_interface.cc",
      "text_report_generator.h",
      "text_report_generator.cc",
      "json_report_generator.h",
      "json_report_generator.cc",
      "html_report_generator.h",
      "html_report_generator.cc",
      "report_data_formatter.h",
      "report_data_formatter.cc"
    ],
}

const fs = require('fs');
const path = require('path');

function run() {
  const basePath = path.join(__dirname, '..', 'doctor');
    for (const [module, files] of Object.entries(f)) {
        if (Array.isArray(files)) {
            files.forEach(file => {
                const filePath = path.join(basePath, module, file);
                if (fs.existsSync(filePath)) {
                    console.log(`File exists: ${filePath}`);
                } else {
                    console.error(`File missing: ${filePath}`);
                    fs.mkdirSync(path.dirname(filePath), { recursive: true });
                    fs.writeFileSync(filePath, ``);
                }
            });
        } else if (typeof files === 'object') {
            for (const [subModule, subFiles] of Object.entries(files)) {
                subFiles.forEach(file => {
                    const filePath = path.join(basePath, module, subModule, file);
                    if (fs.existsSync(filePath)) {
                        console.log(`File exists: ${filePath}`);
                    } else {
                        console.error(`File missing: ${filePath}`);
                        fs.mkdirSync(path.dirname(filePath), { recursive: true });
                        fs.writeFileSync(filePath, ``);
                    }
                });
            }
        }
    }
}

run();