#pragma once

#include <string>

class DocumentXMLDom {
public:
	explicit DocumentXMLDom(std::string raw_xml);

	const std::string& raw_xml() const;

private:
	std::string raw_xml_;
};
