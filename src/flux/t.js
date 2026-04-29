const t = {
    "core": [
      "flux_core.h",
      "flux_core.cc",
      "flux_context.h",
      "flux_context.cc",
      "flux_config.h",
      "flux_config.cc",
      "module_manager.h",
      "module_manager.cc",
      "logger.h",
      "logger.cc"
    ],
    "terminal": [
      "terminal_manager.h",
      "terminal_manager.cc",
      "terminal_interface.h",
      "terminal_interface.cc",
      "terminal_emulator.h",
      "terminal_emulator.cc",
      "terminal_input_handler.h",
      "terminal_input_handler.cc",
      "terminal_output_renderer.h",
      "terminal_output_renderer.cc",
      "terminal_window.h",
      "terminal_window.cc",
      "terminal_buffer.h",
      "terminal_buffer.cc",
      "terminal_colors.h",
      "terminal_styles.h",
      "terminal_ansi_parser.h",
      "terminal_ansi_parser.cc",
      "terminal_size_detector.h",
      "terminal_size_detector.cc"
    ],
    "cfluxreg": [
      "cfluxreg_engine.h",
      "cfluxreg_engine.cc",
      "cfluxreg_registry.h",
      "cfluxreg_registry.cc",
      "cfluxreg_context.h",
      "cfluxreg_context.cc",
      "cfluxreg_api.h",
      "cfluxreg_api.cc",
      "cfluxreg_event_handler.h",
      "cfluxreg_event_handler.cc",
      "cfluxreg_state_manager.h",
      "cfluxreg_state_manager.cc",
      "cfluxreg_script_runner.h",
      "cfluxreg_script_runner.cc",
      "cfluxreg_plugin_manager.h",
      "cfluxreg_plugin_manager.cc"
    ],
    "components": [
      "component_manager.h",
      "component_manager.cc",
      "base_component.h",
      "base_component.cc",
      "window_component.h",
      "window_component.cc",
      "text_input_component.h",
      "text_input_component.cc",
      "button_component.h",
      "button_component.cc",
      "list_component.h",
      "list_component.cc",
      "status_bar_component.h",
      "status_bar_component.cc"
    ],
    "plugins": [
      "plugin_manager.h",
      "plugin_manager.cc",
      "base_plugin.h",
      "base_plugin.cc",
      "cfluxreg_plugin.h",
      "cfluxreg_plugin.cc",
      "example_plugin.h",
      "example_plugin.cc"
    ],
    "utils": [
      "string_utils.h",
      "string_utils.cc",
      "file_utils.h",
      "file_utils.cc",
      "event.h",
      "event.cc",
      "terminal_utils.h",
      "terminal_utils.cc"
    ],
    "history": [
      "command_history.h",
      "command_history.cc",
      "action_history.h",
      "action_history.cc"
    ],
    "input": [
      "input_manager.h",
      "input_manager.cc",
      "key_binding_manager.h",
      "key_binding_manager.cc",
      "command_parser.h",
      "command_parser.cc"
    ],
    "rendering": [
      "renderer.h",
      "renderer.cc",
      "render_context.h",
      "render_context.cc",
      "frame_buffer.h",
      "frame_buffer.cc",
      "render_pipeline.h",
      "render_pipeline.cc"
    ], 
}

for (const category of Object.keys(t)) {
    for (const file of t[category]) {
        console.log(`${category}/${file}`);
        require("fs").writeFileSync(`${category}/${file}`, "");
    }
}