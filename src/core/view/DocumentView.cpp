#include "DocumentView.h"

#include "model/BackgroundConfig.h"
#include "model/Layer.h"
#include "model/eraser/ErasableStroke.h"
#include "view/background/DottedBackgroundView.h"
#include "view/background/GraphBackgroundView.h"
#include "view/background/ImageBackgroundView.h"
#include "view/background/IsoDottedBackgroundView.h"
#include "view/background/IsoGraphBackgroundView.h"
#include "view/background/LinedBackgroundView.h"
#include "view/background/RuledBackgroundView.h"
#include "view/background/StavesBackgroundView.h"
#include "view/background/TransparentCheckerboardBackgroundView.h"

#include "LayerView.h"
#include "StrokeView.h"

using xoj::util::Rectangle;

/**
 * Mark stroke with Audio
 */
void DocumentView::setMarkAudioStroke(bool markAudioStroke) { this->markAudioStroke = markAudioStroke; }

void DocumentView::limitArea(double x, double y, double width, double height) {
    this->lX = x;
    this->lY = y;
    this->lWidth = width;
    this->lHeight = height;
}

/**
 * Drawing first step
 * @param page The page to draw
 * @param cr Draw to thgis context
 * @param dontRenderEditingStroke false to draw currently drawing stroke
 */
void DocumentView::initDrawing(PageRef page, cairo_t* cr, bool dontRenderEditingStroke) {
    this->cr = cr;
    this->page = page;
    this->width = page->getWidth();
    this->height = page->getHeight();
    this->dontRenderEditingStroke = dontRenderEditingStroke;
}

/**
 * Last step in drawing
 */
void DocumentView::finializeDrawing() {
#ifdef DEBUG_SHOW_REPAINT_BOUNDS
    if (this->lX != -1) {
        g_message("DBG:repaint area");
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_set_line_width(cr, 1);
        cairo_rectangle(cr, this->lX + 3, this->lY + 3, this->lWidth - 6, this->lHeight - 6);
        cairo_stroke(cr);
    } else {
        g_message("DBG:repaint complete");
    }
#endif  // DEBUG_SHOW_REPAINT_BOUNDS

    this->lX = -1;
    this->lY = -1;
    this->lWidth = -1;
    this->lHeight = -1;

    this->page = nullptr;
    this->cr = nullptr;
}

/**
 * Draw the background
 */
void DocumentView::drawBackground(bool hidePdfBackground, bool hideImageBackground, bool hideRulingBackground) {
    PageType pt = page->getBackgroundType();
    if (pt.isPdfPage()) {
        // Handled in PdfView
    } else if (pt.isImagePage() && !hideImageBackground) {
        xoj::view::ImageBackgroundView bgView(page->getBackgroundImage(), page->getWidth(), page->getHeight());
        bgView.draw(cr);
    } else if (!hideRulingBackground) {
        auto bgView =
                xoj::view::BackgroundView::create(page->getWidth(), page->getHeight(), page->getBackgroundColor(), pt);
        bgView->draw(cr);
    }
}

/**
 * Draw the full page, usually you would like to call this method
 * @param page The page to draw
 * @param cr Draw to thgis context
 * @param dontRenderEditingStroke false to draw currently drawing stroke
 * @param hideBackground true to hide the background
 */
void DocumentView::drawPage(PageRef page, cairo_t* cr, bool dontRenderEditingStroke, bool hidePdfBackground,
                            bool hideImageBackground, bool hideRulingBackground) {
    initDrawing(page, cr, dontRenderEditingStroke);

    bool backgroundVisible = page->isLayerVisible(0);

    if (backgroundVisible) {
        drawBackground(hidePdfBackground, hideImageBackground, hideRulingBackground);
    }

    if (!backgroundVisible) {
        xoj::view::TransparentCheckerboardBackgroundView bgView(page->getWidth(), page->getHeight());
        bgView.draw(cr);
    }

    xoj::view::Context context{cr, (xoj::view::NonAudioTreatment)this->markAudioStroke,
                               (xoj::view::EditionTreatment) !this->dontRenderEditingStroke, xoj::view::NORMAL_COLOR};
    const Rectangle<double> drawArea{this->lX, this->lY, this->lWidth, this->lHeight};
    for (Layer* layer: *page->getLayers()) {
        if (layer->isVisible()) {
            xoj::view::LayerView layerView(layer);
            if (this->lX == -1) {
                layerView.draw(context);
            } else {
                layerView.draw(context, drawArea);
            }
        }
    }

    finializeDrawing();
}
