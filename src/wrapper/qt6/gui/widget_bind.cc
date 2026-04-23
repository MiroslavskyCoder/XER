#include "wrapper/qt6/gui/widget.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::gui::widget_v8_detail {

void WidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetGeometry(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetResize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetMove(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetX(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetY(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetWidth(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetHeight(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetIsLayout(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCreateLayout(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetGetLayout(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetShow(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetShowMaximized(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetShowMinimized(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetShowFullScreen(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetHide(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetClose(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetRaise(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLower(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetActivateWindow(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetIsVisible(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetEnabled(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetIsEnabled(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetHasFocus(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetFocus(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetStyleSheet(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetToolTip(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetFixedSize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetWindowOpacity(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetWindowOpacity(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetWindowState(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddLabel(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddButton(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddLineEdit(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddTextEdit(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddComboBox(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddCheckBox(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddRadioButton(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddSpinBox(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddSlider(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddProgressBar(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddListWidget(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetControlExists(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetControlIds(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetRemoveControl(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetClearControls(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetControlText(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetControlText(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetControlValue(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetControlValue(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddComboItem(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetClearComboItems(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetAddListItem(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetClearListItems(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetPlaceholder(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSetControlGeometry(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetOnClicked(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetOnTextChanged(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticIsAvailable(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticExec(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticQuit(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticProcessEvents(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticScreenCount(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticPrimaryScreenWidth(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetStaticPrimaryScreenHeight(const v8::FunctionCallbackInfo<v8::Value>& args);

void WidgetLabelCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelSetAlignment(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelResize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelSetParent(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelSetPosition(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelSetStyle(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelSetText(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLabelText(const v8::FunctionCallbackInfo<v8::Value>& args);

void WidgetButtonCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLineEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTextEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetComboBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCheckBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetRadioButtonCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSpinBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetDoubleSpinBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetSliderCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetDialCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetProgressBarCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetListWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTableWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTreeWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetDateEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTimeEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetDateTimeEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetGroupBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetTabWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args);

void WidgetCompatControlSetParent(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetPosition(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlResize(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetStyle(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetText(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlText(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetValue(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlValue(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetChecked(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlAddItem(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlClearItems(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetPlaceholder(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetCompatControlSetAlignment(const v8::FunctionCallbackInfo<v8::Value>& args);

void WidgetLayoutCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLayoutAddWidget(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLayoutAddRow(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLayoutAddSpacing(const v8::FunctionCallbackInfo<v8::Value>& args);
void WidgetLayoutAddStretch(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace qt6::gui::widget_v8_detail

namespace qt6::gui {

using namespace qt6::v8bridge;

bool RegisterQtWidgetClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtWidget", widget_v8_detail::WidgetCtor,
        {
            { "setTitle",           widget_v8_detail::WidgetSetTitle },
            { "title",              widget_v8_detail::WidgetTitle },
            { "setGeometry",        widget_v8_detail::WidgetSetGeometry },
            { "resize",             widget_v8_detail::WidgetResize },
            { "move",               widget_v8_detail::WidgetMove },
            { "x",                  widget_v8_detail::WidgetX },
            { "y",                  widget_v8_detail::WidgetY },
            { "width",              widget_v8_detail::WidgetWidth },
            { "height",             widget_v8_detail::WidgetHeight },
            { "size",               widget_v8_detail::WidgetSize },
            { "show",               widget_v8_detail::WidgetShow },
            { "showMaximized",      widget_v8_detail::WidgetShowMaximized },
            { "showMinimized",      widget_v8_detail::WidgetShowMinimized },
            { "showFullScreen",     widget_v8_detail::WidgetShowFullScreen },
            { "hide",               widget_v8_detail::WidgetHide },
            { "close",              widget_v8_detail::WidgetClose },
            { "raise",              widget_v8_detail::WidgetRaise },
            { "lower",              widget_v8_detail::WidgetLower },
            { "activateWindow",     widget_v8_detail::WidgetActivateWindow },
            { "isVisible",          widget_v8_detail::WidgetIsVisible },
            { "setEnabled",         widget_v8_detail::WidgetSetEnabled },
            { "isEnabled",          widget_v8_detail::WidgetIsEnabled },
            { "hasFocus",           widget_v8_detail::WidgetHasFocus },
            { "setFocus",           widget_v8_detail::WidgetSetFocus },
            { "setStyleSheet",      widget_v8_detail::WidgetSetStyleSheet },
            { "setToolTip",         widget_v8_detail::WidgetSetToolTip },
            { "setMinimumSize",     widget_v8_detail::WidgetSetMinimumSize },
            { "setMaximumSize",     widget_v8_detail::WidgetSetMaximumSize },
            { "setFixedSize",       widget_v8_detail::WidgetSetFixedSize },
            { "setWindowOpacity",   widget_v8_detail::WidgetSetWindowOpacity },
            { "windowOpacity",      widget_v8_detail::WidgetWindowOpacity },
            { "windowState",        widget_v8_detail::WidgetWindowState },
            { "isLayout",           widget_v8_detail::WidgetIsLayout },
            { "createLayout",       widget_v8_detail::WidgetCreateLayout },
            { "getLayout",          widget_v8_detail::WidgetGetLayout },
            { "addLabel",           widget_v8_detail::WidgetAddLabel },
            { "addButton",          widget_v8_detail::WidgetAddButton },
            { "addLineEdit",        widget_v8_detail::WidgetAddLineEdit },
            { "addTextEdit",        widget_v8_detail::WidgetAddTextEdit },
            { "addComboBox",        widget_v8_detail::WidgetAddComboBox },
            { "addCheckBox",        widget_v8_detail::WidgetAddCheckBox },
            { "addRadioButton",     widget_v8_detail::WidgetAddRadioButton },
            { "addSpinBox",         widget_v8_detail::WidgetAddSpinBox },
            { "addSlider",          widget_v8_detail::WidgetAddSlider },
            { "addProgressBar",     widget_v8_detail::WidgetAddProgressBar },
            { "addListWidget",      widget_v8_detail::WidgetAddListWidget },
            { "controlExists",      widget_v8_detail::WidgetControlExists },
            { "controlIds",         widget_v8_detail::WidgetControlIds },
            { "removeControl",      widget_v8_detail::WidgetRemoveControl },
            { "clearControls",      widget_v8_detail::WidgetClearControls },
            { "setControlText",     widget_v8_detail::WidgetSetControlText },
            { "controlText",        widget_v8_detail::WidgetControlText },
            { "setControlValue",    widget_v8_detail::WidgetSetControlValue },
            { "controlValue",       widget_v8_detail::WidgetControlValue },
            { "setControlChecked",  widget_v8_detail::WidgetSetControlChecked },
            { "controlChecked",     widget_v8_detail::WidgetControlChecked },
            { "addComboItem",       widget_v8_detail::WidgetAddComboItem },
            { "clearComboItems",    widget_v8_detail::WidgetClearComboItems },
            { "addListItem",        widget_v8_detail::WidgetAddListItem },
            { "clearListItems",     widget_v8_detail::WidgetClearListItems },
            { "setPlaceholder",     widget_v8_detail::WidgetSetPlaceholder },
            { "setControlGeometry", widget_v8_detail::WidgetSetControlGeometry },
            { "onClicked",          widget_v8_detail::WidgetOnClicked },
            { "onTextChanged",      widget_v8_detail::WidgetOnTextChanged },
        },
        {
            { "isAvailable",         widget_v8_detail::WidgetStaticIsAvailable },
            { "exec",                widget_v8_detail::WidgetStaticExec },
            { "quit",                widget_v8_detail::WidgetStaticQuit },
            { "processEvents",       widget_v8_detail::WidgetStaticProcessEvents },
            { "screenCount",         widget_v8_detail::WidgetStaticScreenCount },
            { "primaryScreenWidth",  widget_v8_detail::WidgetStaticPrimaryScreenWidth },
            { "primaryScreenHeight", widget_v8_detail::WidgetStaticPrimaryScreenHeight },
        });

    if (!ExportClass(isolate, context, "QtWidget", tpl)) return false;

    const std::vector<MethodDef> compat_control_methods = {
        { "setParent",      widget_v8_detail::WidgetCompatControlSetParent },
        { "setPosition",    widget_v8_detail::WidgetCompatControlSetPosition },
        { "resize",         widget_v8_detail::WidgetCompatControlResize },
        { "setStyle",       widget_v8_detail::WidgetCompatControlSetStyle },
        { "setText",        widget_v8_detail::WidgetCompatControlSetText },
        { "text",           widget_v8_detail::WidgetCompatControlText },
        { "setValue",       widget_v8_detail::WidgetCompatControlSetValue },
        { "value",          widget_v8_detail::WidgetCompatControlValue },
        { "setChecked",     widget_v8_detail::WidgetCompatControlSetChecked },
        { "isChecked",      widget_v8_detail::WidgetCompatControlChecked },
        { "addItem",        widget_v8_detail::WidgetCompatControlAddItem },
        { "clearItems",     widget_v8_detail::WidgetCompatControlClearItems },
        { "setPlaceholder", widget_v8_detail::WidgetCompatControlSetPlaceholder },
        { "setAlignment",   widget_v8_detail::WidgetCompatControlSetAlignment },
    };

    auto label_tpl = MakeClass(isolate, "QtWidgetLabel", widget_v8_detail::WidgetLabelCtor,
        {
            { "setAlignment", widget_v8_detail::WidgetLabelSetAlignment },
            { "resize",       widget_v8_detail::WidgetLabelResize },
            { "setParent",    widget_v8_detail::WidgetLabelSetParent },
            { "setPosition",  widget_v8_detail::WidgetLabelSetPosition },
            { "setStyle",     widget_v8_detail::WidgetLabelSetStyle },
            { "setText",      widget_v8_detail::WidgetLabelSetText },
            { "text",         widget_v8_detail::WidgetLabelText },
        });
    if (!ExportClass(isolate, context, "QtWidgetLabel", label_tpl)) return false;

    auto button_tpl = MakeClass(isolate, "QtWidgetButton", widget_v8_detail::WidgetButtonCtor, compat_control_methods);
    auto line_edit_tpl = MakeClass(isolate, "QtWidgetLineEdit", widget_v8_detail::WidgetLineEditCtor, compat_control_methods);
    auto text_edit_tpl = MakeClass(isolate, "QtWidgetTextEdit", widget_v8_detail::WidgetTextEditCtor, compat_control_methods);
    auto combo_box_tpl = MakeClass(isolate, "QtWidgetComboBox", widget_v8_detail::WidgetComboBoxCtor, compat_control_methods);
    auto check_box_tpl = MakeClass(isolate, "QtWidgetCheckBox", widget_v8_detail::WidgetCheckBoxCtor, compat_control_methods);
    auto radio_tpl = MakeClass(isolate, "QtWidgetRadioButton", widget_v8_detail::WidgetRadioButtonCtor, compat_control_methods);
    auto spin_tpl = MakeClass(isolate, "QtWidgetSpinBox", widget_v8_detail::WidgetSpinBoxCtor, compat_control_methods);
    auto dspin_tpl = MakeClass(isolate, "QtWidgetDoubleSpinBox", widget_v8_detail::WidgetDoubleSpinBoxCtor, compat_control_methods);
    auto slider_tpl = MakeClass(isolate, "QtWidgetSlider", widget_v8_detail::WidgetSliderCtor, compat_control_methods);
    auto dial_tpl = MakeClass(isolate, "QtWidgetDial", widget_v8_detail::WidgetDialCtor, compat_control_methods);
    auto progress_tpl = MakeClass(isolate, "QtWidgetProgressBar", widget_v8_detail::WidgetProgressBarCtor, compat_control_methods);
    auto list_tpl = MakeClass(isolate, "QtWidgetListWidget", widget_v8_detail::WidgetListWidgetCtor, compat_control_methods);
    auto table_tpl = MakeClass(isolate, "QtWidgetTableWidget", widget_v8_detail::WidgetTableWidgetCtor, compat_control_methods);
    auto tree_tpl = MakeClass(isolate, "QtWidgetTreeWidget", widget_v8_detail::WidgetTreeWidgetCtor, compat_control_methods);
    auto date_tpl = MakeClass(isolate, "QtWidgetDateEdit", widget_v8_detail::WidgetDateEditCtor, compat_control_methods);
    auto time_tpl = MakeClass(isolate, "QtWidgetTimeEdit", widget_v8_detail::WidgetTimeEditCtor, compat_control_methods);
    auto dt_tpl = MakeClass(isolate, "QtWidgetDateTimeEdit", widget_v8_detail::WidgetDateTimeEditCtor, compat_control_methods);
    auto group_tpl = MakeClass(isolate, "QtWidgetGroupBox", widget_v8_detail::WidgetGroupBoxCtor, compat_control_methods);
    auto tab_tpl = MakeClass(isolate, "QtWidgetTabWidget", widget_v8_detail::WidgetTabWidgetCtor, compat_control_methods);

    if (!ExportClass(isolate, context, "QtWidgetButton", button_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetLineEdit", line_edit_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetTextEdit", text_edit_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetComboBox", combo_box_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetCheckBox", check_box_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetRadioButton", radio_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetSpinBox", spin_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetDoubleSpinBox", dspin_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetSlider", slider_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetDial", dial_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetProgressBar", progress_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetListWidget", list_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetTableWidget", table_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetTreeWidget", tree_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetDateEdit", date_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetTimeEdit", time_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetDateTimeEdit", dt_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetGroupBox", group_tpl)) return false;
    if (!ExportClass(isolate, context, "QtWidgetTabWidget", tab_tpl)) return false;

    auto layout_tpl = MakeClass(isolate, "QtWidgetLayout", widget_v8_detail::WidgetLayoutCtor,
        {
            { "addWidget", widget_v8_detail::WidgetLayoutAddWidget },
            { "addRow", widget_v8_detail::WidgetLayoutAddRow },
            { "addSpacing", widget_v8_detail::WidgetLayoutAddSpacing },
            { "addStretch", widget_v8_detail::WidgetLayoutAddStretch },
        });
    if (!ExportClass(isolate, context, "QtWidgetLayout", layout_tpl)) return false;

    auto global = context->Global();
    auto maybe_widget = global->Get(context, ToV8Str(isolate, "QtWidget"));
    auto maybe_label = global->Get(context, ToV8Str(isolate, "QtWidgetLabel"));
    auto maybe_button = global->Get(context, ToV8Str(isolate, "QtWidgetButton"));
    auto maybe_line_edit = global->Get(context, ToV8Str(isolate, "QtWidgetLineEdit"));
    auto maybe_text_edit = global->Get(context, ToV8Str(isolate, "QtWidgetTextEdit"));
    auto maybe_combo = global->Get(context, ToV8Str(isolate, "QtWidgetComboBox"));
    auto maybe_check = global->Get(context, ToV8Str(isolate, "QtWidgetCheckBox"));
    auto maybe_radio = global->Get(context, ToV8Str(isolate, "QtWidgetRadioButton"));
    auto maybe_spin = global->Get(context, ToV8Str(isolate, "QtWidgetSpinBox"));
    auto maybe_dspin = global->Get(context, ToV8Str(isolate, "QtWidgetDoubleSpinBox"));
    auto maybe_slider = global->Get(context, ToV8Str(isolate, "QtWidgetSlider"));
    auto maybe_dial = global->Get(context, ToV8Str(isolate, "QtWidgetDial"));
    auto maybe_progress = global->Get(context, ToV8Str(isolate, "QtWidgetProgressBar"));
    auto maybe_list = global->Get(context, ToV8Str(isolate, "QtWidgetListWidget"));
    auto maybe_table = global->Get(context, ToV8Str(isolate, "QtWidgetTableWidget"));
    auto maybe_tree = global->Get(context, ToV8Str(isolate, "QtWidgetTreeWidget"));
    auto maybe_date = global->Get(context, ToV8Str(isolate, "QtWidgetDateEdit"));
    auto maybe_time = global->Get(context, ToV8Str(isolate, "QtWidgetTimeEdit"));
    auto maybe_dt = global->Get(context, ToV8Str(isolate, "QtWidgetDateTimeEdit"));
    auto maybe_group = global->Get(context, ToV8Str(isolate, "QtWidgetGroupBox"));
    auto maybe_tab = global->Get(context, ToV8Str(isolate, "QtWidgetTabWidget"));
    if (maybe_widget.IsEmpty() || maybe_label.IsEmpty()) return false;
    if (!maybe_widget.ToLocalChecked()->IsFunction() || !maybe_label.ToLocalChecked()->IsFunction()) return false;

    auto widget_ctor = maybe_widget.ToLocalChecked().As<v8::Object>();
    auto label_ctor = maybe_label.ToLocalChecked();
    auto button_ctor = maybe_button.ToLocalChecked();
    auto line_edit_ctor = maybe_line_edit.ToLocalChecked();
    auto text_edit_ctor = maybe_text_edit.ToLocalChecked();
    auto combo_ctor = maybe_combo.ToLocalChecked();
    auto check_ctor = maybe_check.ToLocalChecked();
    auto radio_ctor = maybe_radio.ToLocalChecked();
    auto spin_ctor = maybe_spin.ToLocalChecked();
    auto dspin_ctor = maybe_dspin.ToLocalChecked();
    auto slider_ctor = maybe_slider.ToLocalChecked();
    auto dial_ctor = maybe_dial.ToLocalChecked();
    auto progress_ctor = maybe_progress.ToLocalChecked();
    auto list_ctor = maybe_list.ToLocalChecked();
    auto table_ctor = maybe_table.ToLocalChecked();
    auto tree_ctor = maybe_tree.ToLocalChecked();
    auto date_ctor = maybe_date.ToLocalChecked();
    auto time_ctor = maybe_time.ToLocalChecked();
    auto dt_ctor = maybe_dt.ToLocalChecked();
    auto group_ctor = maybe_group.ToLocalChecked();
    auto tab_ctor = maybe_tab.ToLocalChecked();
    if (!widget_ctor->Set(context, ToV8Str(isolate, "Label"), label_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "Button"), button_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "LineEdit"), line_edit_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "TextEdit"), text_edit_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "ComboBox"), combo_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "CheckBox"), check_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "RadioButton"), radio_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "SpinBox"), spin_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "DoubleSpinBox"), dspin_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "Slider"), slider_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "Dial"), dial_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "ProgressBar"), progress_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "ListWidget"), list_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "TableWidget"), table_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "TreeWidget"), tree_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "DateEdit"), date_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "TimeEdit"), time_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "DateTimeEdit"), dt_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "GroupBox"), group_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "TabWidget"), tab_ctor).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "VBoxLayout"), v8::Integer::New(isolate, 1)).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "HBoxLayout"), v8::Integer::New(isolate, 2)).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "GridLayout"), v8::Integer::New(isolate, 3)).FromMaybe(false)) return false;
    if (!widget_ctor->Set(context, ToV8Str(isolate, "FormLayout"), v8::Integer::New(isolate, 4)).FromMaybe(false)) return false;

    return true;
}

}  // namespace qt6::gui
