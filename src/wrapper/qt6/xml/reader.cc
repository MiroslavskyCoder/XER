#include "wrapper/qt6/xml/reader.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#include <sstream>
#include <stack>

#if ENGINE_HAS_QT6
#include <QFile>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#endif

namespace qt6::xml {

namespace {

#if ENGINE_HAS_QT6
XmlElement ReadElement(QXmlStreamReader& reader) {
    XmlElement el;
    el.name   = reader.name().toString().toUtf8().constData();
    el.ns_uri = reader.namespaceUri().toString().toUtf8().constData();

    for (const auto& attr : reader.attributes()) {
        XmlAttr a;
        a.name   = attr.name().toString().toUtf8().constData();
        a.value  = attr.value().toString().toUtf8().constData();
        a.ns_uri = attr.namespaceUri().toString().toUtf8().constData();
        el.attrs.push_back(std::move(a));
    }

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isEndElement()) break;
        if (reader.isStartElement()) {
            el.children.push_back(ReadElement(reader));
        } else if (reader.isCharacters()) {
            auto t = reader.text().toString().trimmed();
            if (!t.isEmpty()) el.text += t.toUtf8().constData();
        }
    }
    return el;
}

XmlParseResult ParseFromReader(QXmlStreamReader& reader) {
    XmlParseResult result;
    reader.setNamespaceProcessing(true);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            result.root = ReadElement(reader);
            result.ok   = true;
            break;
        }
    }

    if (reader.hasError()) {
        result.ok           = false;
        result.error        = reader.errorString().toUtf8().constData();
        result.error_line   = static_cast<int>(reader.lineNumber());
        result.error_column = static_cast<int>(reader.columnNumber());
    }
    return result;
}

void WriteElement(QXmlStreamWriter& writer, const XmlElement& el) {
    if (el.ns_uri.empty()) {
        writer.writeStartElement(QString::fromUtf8(el.name.c_str()));
    } else {
        writer.writeStartElement(QString::fromUtf8(el.ns_uri.c_str()),
                                  QString::fromUtf8(el.name.c_str()));
    }
    for (const auto& attr : el.attrs) {
        writer.writeAttribute(QString::fromUtf8(attr.name.c_str()),
                              QString::fromUtf8(attr.value.c_str()));
    }
    if (!el.text.empty()) {
        writer.writeCharacters(QString::fromUtf8(el.text.c_str()));
    }
    for (const auto& child : el.children) {
        WriteElement(writer, child);
    }
    writer.writeEndElement();
}
#endif

}  // namespace

XmlParseResult ParseXml(const std::string& xml_text) {
#if ENGINE_HAS_QT6
    QXmlStreamReader reader(QString::fromUtf8(xml_text.c_str()));
    return ParseFromReader(reader);
#else
    return { false, "Qt6 not available" };
#endif
}

XmlParseResult ParseXmlFile(const std::string& file_path) {
#if ENGINE_HAS_QT6
    QFile f(QString::fromUtf8(file_path.c_str()));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return { false, "Cannot open file: " + file_path };
    }
    QXmlStreamReader reader(&f);
    return ParseFromReader(reader);
#else
    return { false, "Qt6 not available" };
#endif
}

std::string SerializeXml(const XmlElement& root, bool pretty) {
#if ENGINE_HAS_QT6
    QString output;
    QXmlStreamWriter writer(&output);
    writer.setAutoFormatting(pretty);
    writer.writeStartDocument();
    WriteElement(writer, root);
    writer.writeEndDocument();
    return output.toUtf8().constData();
#else
    return "";
#endif
}

std::vector<const XmlElement*> FindElements(const XmlElement& root,
                                             const std::string& path) {
    std::vector<const XmlElement*> results;

    // Split "a/b/c" into segments
    std::vector<std::string> segments;
    std::string seg;
    for (char ch : path) {
        if (ch == '/') {
            if (!seg.empty()) { segments.push_back(seg); seg.clear(); }
        } else {
            seg += ch;
        }
    }
    if (!seg.empty()) segments.push_back(seg);

    if (segments.empty()) return results;

    // BFS traversal down the path
    std::vector<const XmlElement*> current = { &root };
    for (const auto& name : segments) {
        std::vector<const XmlElement*> next;
        for (const auto* el : current) {
            for (const auto& child : el->children) {
                if (child.name == name) next.push_back(&child);
            }
        }
        current = std::move(next);
        if (current.empty()) break;
    }

    return current;
}

}  // namespace qt6::xml
