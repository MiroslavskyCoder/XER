#include "document_xml_dom.h"

#include <utility>

DocumentXMLDom::DocumentXMLDom(std::string raw_xml)
	: raw_xml_(std::move(raw_xml)) {}

const std::string& DocumentXMLDom::raw_xml() const {
	return raw_xml_;
}
