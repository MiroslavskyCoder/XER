#include "wrapper/skia/skia_engine_bridge_pdf.h"
#if ENGINE_HAS_SKIA_BRIDGE
#include <SkDocument.h>
#include <SkCanvas.h>
#endif

namespace engine::bridge::skia {
    int PDFBeginDocument(const std::string& author, const std::string& title) { return -1; /* TODO: Implement */ }
    int PDFBeginPage(int doc_id, float width, float height) { return -1; /* TODO: Implement */ }
    bool PDFEndPage(int doc_id) { return false; /* TODO: Implement */ }
    bool PDFEndDocument(int doc_id, std::vector<uint8_t>* out_bytes) { return false; /* TODO: Implement */ }
}
