#include "wrapper/qt6/ui/widget_factory.h"

#include <algorithm>

namespace qt6::ui {

#if HAS_QT_UI_FACTORY
std::unique_ptr<QWidget> MakeLabel(QWidget* parent, const std::string& text, int x, int y, int w, int h) {
    auto widget = std::make_unique<QLabel>(QString::fromUtf8(text.c_str()), parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeButton(QWidget* parent, const std::string& text, int x, int y, int w, int h) {
    auto widget = std::make_unique<QPushButton>(QString::fromUtf8(text.c_str()), parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeLineEdit(QWidget* parent, const std::string& placeholder, int x, int y, int w, int h) {
    auto widget = std::make_unique<QLineEdit>(parent);
    widget->setPlaceholderText(QString::fromUtf8(placeholder.c_str()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeTextEdit(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QPlainTextEdit>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeComboBox(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QComboBox>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeCheckBox(QWidget* parent, const std::string& text, int x, int y, int w, int h) {
    auto widget = std::make_unique<QCheckBox>(QString::fromUtf8(text.c_str()), parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeRadioButton(QWidget* parent, const std::string& text, int x, int y, int w, int h) {
    auto widget = std::make_unique<QRadioButton>(QString::fromUtf8(text.c_str()), parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeSpinBox(QWidget* parent, int value, int min, int max, int x, int y, int w, int h) {
    auto widget = std::make_unique<QSpinBox>(parent);
    widget->setMinimum(min);
    widget->setMaximum(std::max(min, max));
    widget->setValue(std::clamp(value, widget->minimum(), widget->maximum()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeDoubleSpinBox(QWidget* parent, double value, double min, double max, int x, int y, int w, int h) {
    auto widget = std::make_unique<QDoubleSpinBox>(parent);
    widget->setMinimum(min);
    widget->setMaximum(std::max(min, max));
    widget->setValue(std::clamp(value, widget->minimum(), widget->maximum()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeSlider(QWidget* parent, int orientation, int min, int max, int value, int x, int y, int w, int h) {
    auto widget = std::make_unique<QSlider>(orientation == 0 ? Qt::Horizontal : Qt::Vertical, parent);
    widget->setMinimum(min);
    widget->setMaximum(std::max(min, max));
    widget->setValue(std::clamp(value, widget->minimum(), widget->maximum()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeDial(QWidget* parent, int min, int max, int value, int x, int y, int w, int h) {
    auto widget = std::make_unique<QDial>(parent);
    widget->setMinimum(min);
    widget->setMaximum(std::max(min, max));
    widget->setValue(std::clamp(value, widget->minimum(), widget->maximum()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeProgressBar(QWidget* parent, int min, int max, int value, int x, int y, int w, int h) {
    auto widget = std::make_unique<QProgressBar>(parent);
    widget->setMinimum(min);
    widget->setMaximum(std::max(min, max));
    widget->setValue(std::clamp(value, widget->minimum(), widget->maximum()));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeListWidget(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QListWidget>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeTableWidget(QWidget* parent, int rows, int cols, int x, int y, int w, int h) {
    auto widget = std::make_unique<QTableWidget>(parent);
    widget->setRowCount(std::max(0, rows));
    widget->setColumnCount(std::max(0, cols));
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeTreeWidget(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QTreeWidget>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeDateEdit(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QDateEdit>(parent);
    widget->setCalendarPopup(true);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeTimeEdit(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QTimeEdit>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeDateTimeEdit(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QDateTimeEdit>(parent);
    widget->setCalendarPopup(true);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeGroupBox(QWidget* parent, const std::string& title, int x, int y, int w, int h) {
    auto widget = std::make_unique<QGroupBox>(QString::fromUtf8(title.c_str()), parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}

std::unique_ptr<QWidget> MakeTabWidget(QWidget* parent, int x, int y, int w, int h) {
    auto widget = std::make_unique<QTabWidget>(parent);
    widget->setGeometry(x, y, w, h);
    return widget;
}
#endif

}  // namespace qt6::ui
