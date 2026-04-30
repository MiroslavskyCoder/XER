#pragma once
#include <string>
#include <absl/strings/string_view.h>

namespace Engine::ErrorHandler {

// TODO: Реализовать генератор minidump с использованием absl, range-v3, zlib, openssl и т.д.
class ErrMinidumpGenerator {
public:
	ErrMinidumpGenerator() = default;
	~ErrMinidumpGenerator() = default;

	// Заглушка: сгенерировать дамп по пути
	bool Generate(absl::string_view path) {
		// TODO: Реализация
		return false;
	}
};

} // namespace Engine::ErrorHandler
