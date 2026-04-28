/**
 * @file v8_runtime_checker.h
 * @brief Проверяет совместимость окружения с V8 и анализирует конфликты.
 * @author Yoshi Arasaka (main)
 * @date 2023-10-27
 * @copyright MIT License
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "analysis/dependency_analyzer.h"
#include "core/engine_doctor_context.h"

namespace EngineDoctor {

class V8RuntimeChecker {
public:
    V8RuntimeChecker(Context& context);
    ~V8RuntimeChecker() = default;

    /**
     * @brief Проверяет установленные библиотеки на предмет конфликтов с V8.
     * @param installed_libs Список установленных библиотек.
     * @param v8_conflicts Список обнаруженных конфликтов (имена библиотек).
     */
    void check_v8_compatibility(
        const std::vector<LibraryDependencyInfo>& installed_libs,
        std::vector<std::string>& v8_conflicts
    );

private:
    Context& context_;

    // Список известных конфликтующих библиотек для V8
    std::vector<std::pair<std::string, std::string>> known_v8_conflicts_;

    void load_known_v8_conflicts();
    void check_for_specific_conflicts(const std::vector<LibraryDependencyInfo>& installed_libs, std::vector<std::string>& v8_conflicts);
    // ...
};

} // namespace EngineDoctor
