#include "stack_error.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ToUtf8(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    if (value.IsEmpty()) {
        return std::string();
    }

    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

bool UseAnsiColors() {
    const char* no_color = std::getenv("NO_COLOR");
    if (no_color != nullptr && no_color[0] != '\0') {
        return false;
    }

    const char* term = std::getenv("TERM");
    if (term == nullptr || term[0] == '\0') {
        return false;
    }

    return std::string(term) != "dumb";
}

std::string Color(std::string_view text, std::string_view ansi_code, bool enable) {
    if (!enable) {
        return std::string(text);
    }
    return std::string(ansi_code) + std::string(text) + "\033[0m";
}

std::string SectionHeader(std::string_view title, bool enable_colors) {
    return Color(std::string("-- ") + std::string(title) + " --", "\033[1;36m", enable_colors);
}

std::string Repeat(char c, int count) {
    if (count <= 0) {
        return std::string();
    }
    return std::string(static_cast<size_t>(count), c);
}

std::vector<std::string> SplitLines(std::string_view text) {
    std::vector<std::string> lines;
    std::string current;
    current.reserve(128);

    for (char c : text) {
        if (c == '\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }
        if (c != '\r') {
            current.push_back(c);
        }
    }

    lines.push_back(current);
    return lines;
}

std::string BuildSourceExcerpt(std::string_view source_text, int line_number_1_based, int column_0_based) {
    if (line_number_1_based <= 0) {
        return std::string();
    }

    const std::vector<std::string> lines = SplitLines(source_text);
    if (lines.empty()) {
        return std::string();
    }

    const int target_index = line_number_1_based - 1;
    if (target_index < 0 || target_index >= static_cast<int>(lines.size())) {
        return std::string();
    }

    const int from = std::max(0, target_index - 2);
    const int to = std::min(static_cast<int>(lines.size()) - 1, target_index + 2);

    std::ostringstream out;

    int max_line_number = to + 1;
    int width = 1;
    while (max_line_number >= 10) {
        max_line_number /= 10;
        ++width;
    }

    for (int i = from; i <= to; ++i) {
        const bool is_target = i == target_index;
        out << (is_target ? " > " : "   ")
            << std::setw(width)
            << (i + 1)
            << " | "
            << lines[i]
            << '\n';

        if (is_target && column_0_based >= 0) {
            out << "   " << Repeat(' ', width) << " | ";
            for (int j = 0; j < column_0_based; ++j) {
                out << ' ';
            }
            out << "^\n";
        }
    }

    return out.str();
}

void AppendStackTrace(v8::Isolate* isolate,
                      v8::Local<v8::Context> context,
                      const v8::TryCatch& try_catch,
                      std::ostringstream* out) {
    v8::Local<v8::Value> stack_value;
    if (!try_catch.StackTrace(context).ToLocal(&stack_value) || stack_value.IsEmpty()) {
        return;
    }

    const std::string stack_text = ToUtf8(isolate, stack_value);
    if (stack_text.empty()) {
        return;
    }

    *out << stack_text << '\n';
}

}  // namespace

std::string StackError::BuildV8Report(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      const v8::TryCatch& try_catch,
                                      std::string_view phase,
                                      std::string_view source_text) {
    const bool colors = UseAnsiColors();

    std::ostringstream out;
    out << '\n';
    out << Color("============================================================", "\033[1;31m", colors) << '\n';
    out << Color("STACK ERROR", "\033[1;31m", colors) << '\n';
    out << Color("============================================================", "\033[1;31m", colors) << '\n';

    out << SectionHeader("CONTEXT", colors) << '\n';
    out << "Phase   : " << phase << '\n';

    const std::string exception_text = ToUtf8(isolate, try_catch.Exception());
    out << "Message : "
        << Color(exception_text.empty() ? "unknown" : exception_text, "\033[1;33m", colors)
        << '\n';

    v8::Local<v8::Message> message = try_catch.Message();
    if (!message.IsEmpty()) {
        const std::string script_name = ToUtf8(isolate, message->GetScriptResourceName());
        const int line = message->GetLineNumber(context).FromMaybe(-1);
        const int start_column = message->GetStartColumn(context).FromMaybe(-1);
        const int end_column = message->GetEndColumn(context).FromMaybe(-1);

        out << SectionHeader("LOCATION", colors) << '\n';

        if (!script_name.empty()) {
            out << "Script  : " << script_name << '\n';
        }
        if (line > 0) {
            out << "Line    : " << line << '\n';
        }
        if (start_column >= 0) {
            out << "Column  : " << (start_column + 1);
            if (end_column > start_column) {
                out << "-" << end_column;
            }
            out << '\n';
        }

        const std::string excerpt = BuildSourceExcerpt(source_text, line, start_column);
        if (!excerpt.empty()) {
            out << SectionHeader("SOURCE EXCERPT", colors) << '\n';
            out << excerpt;
        }
    }

    out << SectionHeader("STACK TRACE", colors) << '\n';
    AppendStackTrace(isolate, context, try_catch, &out);

    out << Color("============================================================", "\033[1;31m", colors) << '\n';
    return out.str();
}
