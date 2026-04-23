#include "wrapper/qt6/gui/widget_runtime.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/ui/widget_factory.h"
#include "wrapper/qt6/ui/widget_ops.h"
#include "wrapper/qt6/util/v8_string.h"

#include <algorithm>

namespace qt6::gui {

#if HAS_QT_WIDGETS
int WidgetWrapper::addLabel(const std::string& text, int x, int y, int w, int h) {
    int id = next_id_++;
    auto label = qt6::ui::MakeLabel(widget_.get(), text, x, y, w, h);
    controls_[id] = std::move(label);
    return id;
}

int WidgetWrapper::addButton(const std::string& text, int x, int y, int w, int h) {
    int id = next_id_++;
    auto button = qt6::ui::MakeButton(widget_.get(), text, x, y, w, h);
    if (auto* btn = qobject_cast<QPushButton*>(button.get())) {
        connectButtonSignal(id, btn);
    }
    controls_[id] = std::move(button);
    return id;
}

int WidgetWrapper::addLineEdit(const std::string& placeholder, int x, int y, int w, int h) {
    int id = next_id_++;
    auto edit = qt6::ui::MakeLineEdit(widget_.get(), placeholder, x, y, w, h);
    if (auto* line = qobject_cast<QLineEdit*>(edit.get())) {
        connectLineEditSignal(id, line);
    }
    controls_[id] = std::move(edit);
    return id;
}

int WidgetWrapper::addTextEdit(int x, int y, int w, int h) {
    int id = next_id_++;
    auto edit = qt6::ui::MakeTextEdit(widget_.get(), x, y, w, h);
    controls_[id] = std::move(edit);
    return id;
}

int WidgetWrapper::addComboBox(int x, int y, int w, int h) {
    int id = next_id_++;
    auto combo = qt6::ui::MakeComboBox(widget_.get(), x, y, w, h);
    controls_[id] = std::move(combo);
    return id;
}

int WidgetWrapper::addCheckBox(const std::string& text, int x, int y, int w, int h) {
    int id = next_id_++;
    auto checkbox = qt6::ui::MakeCheckBox(widget_.get(), text, x, y, w, h);
    controls_[id] = std::move(checkbox);
    return id;
}

int WidgetWrapper::addRadioButton(const std::string& text, int x, int y, int w, int h) {
    int id = next_id_++;
    auto radio = qt6::ui::MakeRadioButton(widget_.get(), text, x, y, w, h);
    controls_[id] = std::move(radio);
    return id;
}

int WidgetWrapper::addSpinBox(int defaultValue, int min, int max, int x, int y, int w, int h) {
    int id = next_id_++;
    auto spinbox = qt6::ui::MakeSpinBox(widget_.get(), defaultValue, min, max, x, y, w, h);
    controls_[id] = std::move(spinbox);
    return id;
}

int WidgetWrapper::addSlider(int orientation, int min, int max, int value, int x, int y, int w, int h) {
    int id = next_id_++;
    auto slider = qt6::ui::MakeSlider(widget_.get(), orientation, min, max, value, x, y, w, h);
    controls_[id] = std::move(slider);
    return id;
}

int WidgetWrapper::addProgressBar(int min, int max, int value, int x, int y, int w, int h) {
    int id = next_id_++;
    auto bar = qt6::ui::MakeProgressBar(widget_.get(), min, max, value, x, y, w, h);
    controls_[id] = std::move(bar);
    return id;
}

int WidgetWrapper::addListWidget(int x, int y, int w, int h) {
    int id = next_id_++;
    auto list = qt6::ui::MakeListWidget(widget_.get(), x, y, w, h);
    controls_[id] = std::move(list);
    return id;
}

int WidgetWrapper::addTableWidget(int rows, int cols, int x, int y, int w, int h) {
    int id = next_id_++;
    auto table = qt6::ui::MakeTableWidget(widget_.get(), rows, cols, x, y, w, h);
    controls_[id] = std::move(table);
    return id;
}

int WidgetWrapper::addTreeWidget(int x, int y, int w, int h) {
    int id = next_id_++;
    auto tree = qt6::ui::MakeTreeWidget(widget_.get(), x, y, w, h);
    controls_[id] = std::move(tree);
    return id;
}

int WidgetWrapper::addDateEdit(int x, int y, int w, int h) {
    int id = next_id_++;
    auto edit = qt6::ui::MakeDateEdit(widget_.get(), x, y, w, h);
    controls_[id] = std::move(edit);
    return id;
}

int WidgetWrapper::addTimeEdit(int x, int y, int w, int h) {
    int id = next_id_++;
    auto edit = qt6::ui::MakeTimeEdit(widget_.get(), x, y, w, h);
    controls_[id] = std::move(edit);
    return id;
}

int WidgetWrapper::addDateTimeEdit(int x, int y, int w, int h) {
    int id = next_id_++;
    auto edit = qt6::ui::MakeDateTimeEdit(widget_.get(), x, y, w, h);
    controls_[id] = std::move(edit);
    return id;
}

int WidgetWrapper::addDial(int min, int max, int value, int x, int y, int w, int h) {
    int id = next_id_++;
    auto dial = qt6::ui::MakeDial(widget_.get(), min, max, value, x, y, w, h);
    controls_[id] = std::move(dial);
    return id;
}

int WidgetWrapper::addDoubleSpinBox(double value, double min, double max, int x, int y, int w, int h) {
    int id = next_id_++;
    auto spin = qt6::ui::MakeDoubleSpinBox(widget_.get(), value, min, max, x, y, w, h);
    controls_[id] = std::move(spin);
    return id;
}

int WidgetWrapper::addGroupBox(const std::string& title, int x, int y, int w, int h) {
    int id = next_id_++;
    auto group = qt6::ui::MakeGroupBox(widget_.get(), title, x, y, w, h);
    controls_[id] = std::move(group);
    return id;
}

int WidgetWrapper::addTabWidget(int x, int y, int w, int h) {
    int id = next_id_++;
    auto tabs = qt6::ui::MakeTabWidget(widget_.get(), x, y, w, h);
    controls_[id] = std::move(tabs);
    return id;
}

bool WidgetWrapper::removeControl(int id) {
    auto it = controls_.find(id);
    if (it == controls_.end()) return false;
    click_callbacks_.erase(id);
    text_callbacks_.erase(id);
    controls_.erase(it);
    return true;
}

bool WidgetWrapper::hasControl(int id) const {
    return controls_.find(id) != controls_.end();
}

std::vector<int> WidgetWrapper::controlIds() const {
    std::vector<int> out;
    out.reserve(controls_.size());
    for (const auto& [id, _] : controls_) {
        out.push_back(id);
    }
    return out;
}

void WidgetWrapper::clearControls() {
    click_callbacks_.clear();
    text_callbacks_.clear();
    controls_.clear();
}

bool WidgetWrapper::setControlText(int id, const std::string& text) {
    auto* w = control(id);
    return qt6::ui::SetWidgetText(w, text);
}

std::string WidgetWrapper::controlText(int id) const {
    auto* w = control(id);
    return qt6::ui::WidgetText(w);
}

bool WidgetWrapper::setControlValue(int id, double value) {
    auto* w = control(id);
    return qt6::ui::SetWidgetValue(w, value);
}

double WidgetWrapper::controlValue(int id, bool* ok) const {
    auto* w = control(id);
    double out = 0.0;
    const bool has = qt6::ui::WidgetValue(w, &out);
    if (ok) *ok = has;
    return has ? out : 0.0;
}

bool WidgetWrapper::setControlChecked(int id, bool checked) {
    auto* w = control(id);
    return qt6::ui::SetWidgetChecked(w, checked);
}

bool WidgetWrapper::controlChecked(int id, bool* ok) const {
    auto* w = control(id);
    bool out = false;
    const bool has = qt6::ui::WidgetChecked(w, &out);
    if (ok) *ok = has;
    return has ? out : false;
}

bool WidgetWrapper::addComboItem(int id, const std::string& text) {
    auto* w = control(id);
    return qt6::ui::AddWidgetItem(w, text);
}

bool WidgetWrapper::clearComboItems(int id) {
    auto* w = control(id);
    return qt6::ui::ClearWidgetItems(w);
}

bool WidgetWrapper::addListItem(int id, const std::string& text) {
    auto* w = control(id);
    return qt6::ui::AddWidgetItem(w, text);
}

bool WidgetWrapper::clearListItems(int id) {
    auto* w = control(id);
    return qt6::ui::ClearWidgetItems(w);
}

bool WidgetWrapper::setPlaceholder(int id, const std::string& text) {
    auto* w = control(id);
    if (auto* edit = qobject_cast<QLineEdit*>(w)) {
        edit->setPlaceholderText(QString::fromUtf8(text.c_str()));
        return true;
    }
    return false;
}

bool WidgetWrapper::setControlGeometry(int id, int x, int y, int w, int h) {
    auto* c = control(id);
    if (!c) return false;
    c->setGeometry(x, y, w, h);
    return true;
}

bool WidgetWrapper::setOnClicked(int id, v8::Local<v8::Function> fn) {
    if (!hasControl(id)) return false;
    click_callbacks_[id].Reset(isolate_, fn);
    return true;
}

bool WidgetWrapper::setOnTextChanged(int id, v8::Local<v8::Function> fn) {
    if (!hasControl(id)) return false;
    text_callbacks_[id].Reset(isolate_, fn);
    return true;
}

QWidget* WidgetWrapper::control(int id) const {
    auto it = controls_.find(id);
    return it == controls_.end() ? nullptr : it->second.get();
}

void WidgetWrapper::connectButtonSignal(int id, QPushButton* button) {
    QObject::connect(button, &QPushButton::clicked, [this, id]() {
        invokeNoArg(id, click_callbacks_);
    });
}

void WidgetWrapper::connectLineEditSignal(int id, QLineEdit* edit) {
    QObject::connect(edit, &QLineEdit::textChanged, [this, id](const QString& text) {
        auto it = text_callbacks_.find(id);
        if (it == text_callbacks_.end()) return;
        auto cb = it->second.Get(isolate_);
        if (cb.IsEmpty()) return;

        v8::HandleScope hs(isolate_);
        auto ctx = context_.Get(isolate_);
        v8::Context::Scope cs(ctx);
        v8::Local<v8::Value> argv[1] = {qt6::v8bridge::ToV8Str(isolate_, text.toUtf8().constData())};
        cb->Call(ctx, ctx->Global(), 1, argv).IsEmpty();
    });
}

void WidgetWrapper::invokeNoArg(int id, const std::unordered_map<int, v8::Persistent<v8::Function>>& callbacks) {
    auto it = callbacks.find(id);
    if (it == callbacks.end()) return;
    auto cb = it->second.Get(isolate_);
    if (cb.IsEmpty()) return;

    v8::HandleScope hs(isolate_);
    auto ctx = context_.Get(isolate_);
    v8::Context::Scope cs(ctx);
    cb->Call(ctx, ctx->Global(), 0, nullptr).IsEmpty();
}
#endif

}  // namespace qt6::gui
