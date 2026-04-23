#pragma once

#include <v8.h>

namespace qt6::gui {

// Registers global class QtWidget.
// JS API (instance):
//   new QtWidget(title?)
//   setTitle/title, setGeometry, resize, move
//   x/y/width/height, isVisible, show/hide/close
//   showMaximized/showMinimized/showFullScreen
//   raise/lower/activateWindow, setFocus/hasFocus
//   setWindowOpacity/windowOpacity/windowState
//   setEnabled/isEnabled, setStyleSheet, setToolTip
//   setMinimumSize, setMaximumSize, setFixedSize
//   addLabel/addButton/addLineEdit/addRadioButton -> control id
//   addTextEdit/addComboBox/addCheckBox/addSpinBox/addSlider -> control id
//   addProgressBar/addListWidget -> control id
//   controlExists/controlIds/removeControl/clearControls
//   setControlText/controlText/setPlaceholder
//   setControlValue/controlValue
//   setControlChecked/controlChecked
//   addComboItem/clearComboItems/addListItem/clearListItems
//   setControlGeometry
//   onClicked(controlId, fn), onTextChanged(controlId, fn)
// JS API (static):
//   QtWidget.isAvailable()
//   QtWidget.exec() -> int
//   QtWidget.quit()
//   QtWidget.processEvents(maxMs?)
//   QtWidget.screenCount()
//   QtWidget.primaryScreenWidth()
//   QtWidget.primaryScreenHeight()
bool RegisterQtWidgetClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::gui
