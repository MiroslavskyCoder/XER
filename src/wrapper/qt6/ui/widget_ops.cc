#include "wrapper/qt6/ui/widget_ops.h"

#include <QString>

namespace qt6::ui {

#if HAS_QT_UI_WIDGETS
bool SetWidgetText(QWidget* widget, const std::string& text) {
    if (!widget) return false;
    const QString qtext = QString::fromUtf8(text.c_str());

    if (auto* lbl = qobject_cast<QLabel*>(widget)) { lbl->setText(qtext); return true; }
    if (auto* btn = qobject_cast<QPushButton*>(widget)) { btn->setText(qtext); return true; }
    if (auto* edit = qobject_cast<QLineEdit*>(widget)) { edit->setText(qtext); return true; }
    if (auto* plain = qobject_cast<QPlainTextEdit*>(widget)) { plain->setPlainText(qtext); return true; }
    if (auto* text_edit = qobject_cast<QTextEdit*>(widget)) { text_edit->setPlainText(qtext); return true; }
    if (auto* combo = qobject_cast<QComboBox*>(widget)) { combo->setCurrentText(qtext); return true; }
    if (auto* group = qobject_cast<QGroupBox*>(widget)) { group->setTitle(qtext); return true; }

    if (auto* check = qobject_cast<QCheckBox*>(widget)) {
        check->setChecked(text == "checked" || text == "true" || text == "1");
        return true;
    }
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) {
        radio->setChecked(text == "checked" || text == "true" || text == "1");
        return true;
    }

    bool ok = false;
    const int ivalue = qtext.toInt(&ok);
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        if (!ok) return false;
        spin->setValue(ivalue);
        return true;
    }
    if (auto* slider = qobject_cast<QSlider*>(widget)) {
        if (!ok) return false;
        slider->setValue(ivalue);
        return true;
    }
    if (auto* dial = qobject_cast<QDial*>(widget)) {
        if (!ok) return false;
        dial->setValue(ivalue);
        return true;
    }
    if (auto* bar = qobject_cast<QProgressBar*>(widget)) {
        if (!ok) return false;
        bar->setValue(ivalue);
        return true;
    }

    if (auto* list = qobject_cast<QListWidget*>(widget)) {
        list->addItem(qtext);
        return true;
    }

    return false;
}

std::string WidgetText(QWidget* widget) {
    if (!widget) return "";

    if (auto* lbl = qobject_cast<QLabel*>(widget)) return lbl->text().toUtf8().constData();
    if (auto* btn = qobject_cast<QPushButton*>(widget)) return btn->text().toUtf8().constData();
    if (auto* edit = qobject_cast<QLineEdit*>(widget)) return edit->text().toUtf8().constData();
    if (auto* plain = qobject_cast<QPlainTextEdit*>(widget)) return plain->toPlainText().toUtf8().constData();
    if (auto* text_edit = qobject_cast<QTextEdit*>(widget)) return text_edit->toPlainText().toUtf8().constData();
    if (auto* combo = qobject_cast<QComboBox*>(widget)) return combo->currentText().toUtf8().constData();
    if (auto* group = qobject_cast<QGroupBox*>(widget)) return group->title().toUtf8().constData();

    if (auto* check = qobject_cast<QCheckBox*>(widget)) return check->isChecked() ? "checked" : "unchecked";
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) return radio->isChecked() ? "checked" : "unchecked";

    if (auto* spin = qobject_cast<QSpinBox*>(widget)) return std::to_string(spin->value());
    if (auto* dspin = qobject_cast<QDoubleSpinBox*>(widget)) return std::to_string(dspin->value());
    if (auto* slider = qobject_cast<QSlider*>(widget)) return std::to_string(slider->value());
    if (auto* dial = qobject_cast<QDial*>(widget)) return std::to_string(dial->value());
    if (auto* bar = qobject_cast<QProgressBar*>(widget)) return std::to_string(bar->value());

    if (auto* list = qobject_cast<QListWidget*>(widget)) {
        if (auto* item = list->currentItem()) return item->text().toUtf8().constData();
        return "";
    }

    return "";
}

bool SetWidgetValue(QWidget* widget, double value) {
    if (!widget) return false;
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) { spin->setValue(static_cast<int>(value)); return true; }
    if (auto* dspin = qobject_cast<QDoubleSpinBox*>(widget)) { dspin->setValue(value); return true; }
    if (auto* slider = qobject_cast<QSlider*>(widget)) { slider->setValue(static_cast<int>(value)); return true; }
    if (auto* dial = qobject_cast<QDial*>(widget)) { dial->setValue(static_cast<int>(value)); return true; }
    if (auto* bar = qobject_cast<QProgressBar*>(widget)) { bar->setValue(static_cast<int>(value)); return true; }
    return false;
}

bool WidgetValue(QWidget* widget, double* out) {
    if (!widget || !out) return false;
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) { *out = static_cast<double>(spin->value()); return true; }
    if (auto* dspin = qobject_cast<QDoubleSpinBox*>(widget)) { *out = dspin->value(); return true; }
    if (auto* slider = qobject_cast<QSlider*>(widget)) { *out = static_cast<double>(slider->value()); return true; }
    if (auto* dial = qobject_cast<QDial*>(widget)) { *out = static_cast<double>(dial->value()); return true; }
    if (auto* bar = qobject_cast<QProgressBar*>(widget)) { *out = static_cast<double>(bar->value()); return true; }
    return false;
}

bool SetWidgetChecked(QWidget* widget, bool checked) {
    if (!widget) return false;
    if (auto* check = qobject_cast<QCheckBox*>(widget)) { check->setChecked(checked); return true; }
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) { radio->setChecked(checked); return true; }
    return false;
}

bool WidgetChecked(QWidget* widget, bool* out) {
    if (!widget || !out) return false;
    if (auto* check = qobject_cast<QCheckBox*>(widget)) { *out = check->isChecked(); return true; }
    if (auto* radio = qobject_cast<QRadioButton*>(widget)) { *out = radio->isChecked(); return true; }
    return false;
}

bool AddWidgetItem(QWidget* widget, const std::string& text) {
    if (!widget) return false;
    const QString qtext = QString::fromUtf8(text.c_str());
    if (auto* combo = qobject_cast<QComboBox*>(widget)) { combo->addItem(qtext); return true; }
    if (auto* list = qobject_cast<QListWidget*>(widget)) { list->addItem(qtext); return true; }
    return false;
}

bool ClearWidgetItems(QWidget* widget) {
    if (!widget) return false;
    if (auto* combo = qobject_cast<QComboBox*>(widget)) { combo->clear(); return true; }
    if (auto* list = qobject_cast<QListWidget*>(widget)) { list->clear(); return true; }
    return false;
}
#endif

}  // namespace qt6::ui
