#include "document_xml.h"

#include "absl/strings/str_format.h"

#include "libxml2/libxml/relaxng.h"
#include "libxml2/libxml/xmlreader.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h" 

DocumentXML::DocumentXML() : document_(nullptr, xmlFreeDoc) {}

void DocumentXML::reader(absl::string_view path) {
    std::string path_buffer(path);
    const char* path_chars = path_buffer.c_str();

    if (!xmlCheckFilename(path_chars)) {
        llvm::errs() << absl::StrFormat("Invalid xml path: %s\n", path_buffer);
        return;
    }

    xmlDoc* loaded = xmlReadFile(path_chars, nullptr, 0);
    if (loaded == nullptr) {
        llvm::errs() << absl::StrFormat("Failed to read xml document: %s\n", path_buffer);
        return;
    }

    document_.reset(loaded);
}

const xmlDoc* DocumentXML::document() const {
    return document_.get();
}