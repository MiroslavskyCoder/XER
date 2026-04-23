#include "libxml2/libxml/xmlstring.h"
#include "libxml2/libxml/tree.h"

#include "absl/strings/string_view.h"

#include <memory>

class DocumentXML {
public:
    DocumentXML();

    void reader(absl::string_view path);

    const xmlDoc* document() const;

private:
    std::unique_ptr<xmlDoc, void (*)(xmlDoc*)> document_;
};