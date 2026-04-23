#pragma once

#include <string>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6 && defined(QT_WIDGETS_LIB)
#define HAS_QT_UI_WIDGETS 1

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDial>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTreeWidget>
#include <QWidget>
#else
#define HAS_QT_UI_WIDGETS 0
#endif

namespace qt6::ui {

#if HAS_QT_UI_WIDGETS
bool SetWidgetText(QWidget* widget, const std::string& text);
std::string WidgetText(QWidget* widget);

bool SetWidgetValue(QWidget* widget, double value);
bool WidgetValue(QWidget* widget, double* out);

bool SetWidgetChecked(QWidget* widget, bool checked);
bool WidgetChecked(QWidget* widget, bool* out);

bool AddWidgetItem(QWidget* widget, const std::string& text);
bool ClearWidgetItems(QWidget* widget);
#endif

}  // namespace qt6::ui
