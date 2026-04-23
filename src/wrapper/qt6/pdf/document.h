#pragma once

#include <string>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::pdf {

struct PdfPageInfo {
    int    index  = 0;
    double width  = 0.0;   // in points
    double height = 0.0;
    int    rotation = 0;   // 0/90/180/270
};

struct PdfDocumentInfo {
    bool        ok         = false;
    std::string error;
    int         page_count = 0;
    std::string title;
    std::string author;
    std::string subject;
    std::string keywords;
    std::string creator;
    std::string producer;
};

// Load PDF and read metadata
PdfDocumentInfo LoadDocument(const std::string& path);

// Get page geometry (requires Qt6::Pdf)
PdfPageInfo GetPageInfo(const std::string& path, int page_index);

// Render page to PNG file (requires Qt6::Pdf + Qt6::Gui)
bool RenderPageToPng(const std::string& pdf_path, int page_index,
                     const std::string& out_png, double scale = 1.0);

// Extract plain text from a page (requires Qt6::Pdf)
std::string ExtractPageText(const std::string& pdf_path, int page_index);

}  // namespace qt6::pdf
