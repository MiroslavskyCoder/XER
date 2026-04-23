#include "wrapper/qt6/pdf/document.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QString>
#include <QSizeF>
#endif

#if ENGINE_HAS_QT6 && defined(QT_PDF_LIB)
#define HAS_QT_PDF 1
#else
#define HAS_QT_PDF 0
#endif

namespace qt6::pdf {

PdfDocumentInfo LoadDocument(const std::string& path) {
#if HAS_QT_PDF
    QPdfDocument doc;
    auto err = doc.load(QString::fromUtf8(path.c_str()));
    if (err != QPdfDocument::Error::None) {
        return { false, "Failed to load PDF: " + path };
    }
    PdfDocumentInfo info;
    info.ok         = true;
    info.page_count = doc.pageCount();
    info.title      = doc.metaData(QPdfDocument::MetaDataField::Title).toString().toUtf8().constData();
    info.author     = doc.metaData(QPdfDocument::MetaDataField::Author).toString().toUtf8().constData();
    info.subject    = doc.metaData(QPdfDocument::MetaDataField::Subject).toString().toUtf8().constData();
    info.keywords   = doc.metaData(QPdfDocument::MetaDataField::Keywords).toString().toUtf8().constData();
    info.creator    = doc.metaData(QPdfDocument::MetaDataField::Creator).toString().toUtf8().constData();
    info.producer   = doc.metaData(QPdfDocument::MetaDataField::Producer).toString().toUtf8().constData();
    return info;
#else
    return { false, "Qt6::Pdf not available" };
#endif
}

PdfPageInfo GetPageInfo(const std::string& path, int page_index) {
#if HAS_QT_PDF
    QPdfDocument doc;
    if (doc.load(QString::fromUtf8(path.c_str())) != QPdfDocument::Error::None) {
        return {};
    }
    if (page_index < 0 || page_index >= doc.pageCount()) return {};
    QSizeF sz = doc.pagePointSize(page_index);
    return { page_index, sz.width(), sz.height(), 0 };
#else
    return {};
#endif
}

bool RenderPageToPng(const std::string& pdf_path, int page_index,
                     const std::string& out_png, double scale) {
#if HAS_QT_PDF
    QPdfDocument doc;
    if (doc.load(QString::fromUtf8(pdf_path.c_str())) != QPdfDocument::Error::None) {
        return false;
    }
    QSizeF sz = doc.pagePointSize(page_index);
    QSize target(static_cast<int>(sz.width() * scale),
                 static_cast<int>(sz.height() * scale));
    QImage img = doc.render(page_index, target);
    if (img.isNull()) return false;
    return img.save(QString::fromUtf8(out_png.c_str()), "PNG");
#else
    return false;
#endif
}

std::string ExtractPageText(const std::string& pdf_path, int page_index) {
#if HAS_QT_PDF
    QPdfDocument doc;
    if (doc.load(QString::fromUtf8(pdf_path.c_str())) != QPdfDocument::Error::None) {
        return "";
    }
    QPdfSelection sel = doc.getAllText(page_index);
    return sel.text().toUtf8().constData();
#else
    return "";
#endif
}

}  // namespace qt6::pdf
