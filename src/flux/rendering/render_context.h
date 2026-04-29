#pragma once

#include <string>

#include "flux/rendering/frame_buffer.h"
#include "flux/terminal/terminal_size_detector.h"
#include "flux/terminal/terminal_window.h"

namespace flux::rendering {

class RenderContext {
public:
	explicit RenderContext(const flux::terminal::TerminalSize& size = flux::terminal::DefaultTerminalSize());
	static RenderContext FromWindow(const flux::terminal::TerminalWindow& window);

	FrameBuffer& frame();
	const FrameBuffer& frame() const;

	const flux::terminal::TerminalSize& size() const;
	void Resize(const flux::terminal::TerminalSize& size);

	void Clear(char fill = ' ', const flux::terminal::TerminalStyle& style = {});
	void SetTitle(std::string title);
	const std::string& title() const;

private:
	flux::terminal::TerminalSize size_;
	FrameBuffer frame_;
	std::string title_;
};

}  // namespace flux::rendering
