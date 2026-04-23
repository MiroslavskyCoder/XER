#pragma once

#include <v8.h>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6 && defined(QT_WIDGETS_LIB)
#define HAS_QT_WIDGETS 1
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimeEdit>
#include <QTreeWidget>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDial>
#include <QDoubleSpinBox>
#include <QGroupBox>
#else
#define HAS_QT_WIDGETS 0
#endif

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace qt6::gui {

#if HAS_QT_WIDGETS
void EnsureLinuxRuntimeDir();
QApplication* EnsureGuiApp();

class WidgetWrapper {
public:
    explicit WidgetWrapper(
        v8::Isolate* isolate,
        v8::Local<v8::Context> context,
        const std::string& title);
    ~WidgetWrapper();

    QWidget* get() const;

    int addLabel(const std::string& text, int x, int y, int w, int h);
    int addButton(const std::string& text, int x, int y, int w, int h);
    int addLineEdit(const std::string& placeholder, int x, int y, int w, int h);
    int addTextEdit(int x, int y, int w, int h);
    int addComboBox(int x, int y, int w, int h);
    int addCheckBox(const std::string& text, int x, int y, int w, int h);
    int addRadioButton(const std::string& text, int x, int y, int w, int h);
    int addSpinBox(int defaultValue, int min, int max, int x, int y, int w, int h);
    int addSlider(int orientation, int min, int max, int value, int x, int y, int w, int h);
    int addProgressBar(int min, int max, int value, int x, int y, int w, int h);
    int addListWidget(int x, int y, int w, int h);
    int addTableWidget(int rows, int cols, int x, int y, int w, int h);
    int addTreeWidget(int x, int y, int w, int h);
    int addDateEdit(int x, int y, int w, int h);
    int addTimeEdit(int x, int y, int w, int h);
    int addDateTimeEdit(int x, int y, int w, int h);
    int addDial(int min, int max, int value, int x, int y, int w, int h);
    int addDoubleSpinBox(double value, double min, double max, int x, int y, int w, int h);
    int addGroupBox(const std::string& title, int x, int y, int w, int h);
    int addTabWidget(int x, int y, int w, int h);

    bool removeControl(int id);
    bool hasControl(int id) const;
    std::vector<int> controlIds() const;
    void clearControls();

    bool setControlText(int id, const std::string& text);
    std::string controlText(int id) const;
    bool setControlValue(int id, double value);
    double controlValue(int id, bool* ok = nullptr) const;
    bool setControlChecked(int id, bool checked);
    bool controlChecked(int id, bool* ok = nullptr) const;
    bool addComboItem(int id, const std::string& text);
    bool clearComboItems(int id);
    bool addListItem(int id, const std::string& text);
    bool clearListItems(int id);
    bool setPlaceholder(int id, const std::string& text);
    bool setControlGeometry(int id, int x, int y, int w, int h);

    bool setOnClicked(int id, v8::Local<v8::Function> fn);
    bool setOnTextChanged(int id, v8::Local<v8::Function> fn);

private:
    QWidget* control(int id) const;
    void connectButtonSignal(int id, QPushButton* button);
    void connectLineEditSignal(int id, QLineEdit* edit);
    void invokeNoArg(int id, const std::unordered_map<int, v8::Persistent<v8::Function>>& callbacks);

    v8::Isolate* isolate_ = nullptr;
    v8::Persistent<v8::Context> context_;
    std::unique_ptr<QWidget> widget_;
    int next_id_ = 1;
    std::unordered_map<int, std::unique_ptr<QWidget>> controls_;
    std::unordered_map<int, v8::Persistent<v8::Function>> click_callbacks_;
    std::unordered_map<int, v8::Persistent<v8::Function>> text_callbacks_;
};
#endif

}  // namespace qt6::gui
