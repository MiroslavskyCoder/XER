#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::xml {

struct XmlAttr {
    std::string name;
    std::string value;
    std::string ns_uri;
};

struct XmlElement {
    std::string name;
    std::string text;
    std::string ns_uri;
    std::vector<XmlAttr>    attrs;
    std::vector<XmlElement> children;
};

struct XmlParseResult {
    bool             ok    = false;
    std::string      error;
    int              error_line   = 0;
    int              error_column = 0;
    XmlElement       root;
};

// Parse XML from string, returns tree
XmlParseResult ParseXml(const std::string& xml_text);

// Parse XML from file
XmlParseResult ParseXmlFile(const std::string& file_path);

// Serialize element tree back to XML string
std::string SerializeXml(const XmlElement& root, bool pretty = true);

// XPath-like simple selector: "root/child/grandchild"
std::vector<const XmlElement*> FindElements(const XmlElement& root,
                                             const std::string& path);

}  // namespace qt6::xml
