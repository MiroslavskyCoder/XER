#pragma once

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6 && defined(QT_WIDGETS_LIB)
#define HAS_QT_UI_FACTORY 1

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
#include <QTimeEdit>
#include <QTreeWidget>
#include <QWidget>

#include <memory>
#include <string>
#else
#define HAS_QT_UI_FACTORY 0
#endif

namespace qt6::ui {

#if HAS_QT_UI_FACTORY
std::unique_ptr<QWidget> MakeLabel(QWidget* parent, const std::string& text, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeButton(QWidget* parent, const std::string& text, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeLineEdit(QWidget* parent, const std::string& placeholder, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeTextEdit(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeComboBox(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeCheckBox(QWidget* parent, const std::string& text, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeRadioButton(QWidget* parent, const std::string& text, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeSpinBox(QWidget* parent, int value, int min, int max, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeDoubleSpinBox(QWidget* parent, double value, double min, double max, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeSlider(QWidget* parent, int orientation, int min, int max, int value, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeDial(QWidget* parent, int min, int max, int value, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeProgressBar(QWidget* parent, int min, int max, int value, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeListWidget(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeTableWidget(QWidget* parent, int rows, int cols, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeTreeWidget(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeDateEdit(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeTimeEdit(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeDateTimeEdit(QWidget* parent, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeGroupBox(QWidget* parent, const std::string& title, int x, int y, int w, int h);
std::unique_ptr<QWidget> MakeTabWidget(QWidget* parent, int x, int y, int w, int h);
#endif

}  // namespace qt6::ui
