#include <mupdf/fitz.h>

#include "gui.h"
#include "log.h"
#include "reader.h"

namespace Reader {
    static fz_context *ctx = nullptr;
    static fz_document *doc = nullptr;
    static fz_pixmap *pix = nullptr;
    static fz_page *page = nullptr;
    static fz_matrix ctm;
    static fz_rect pageBounds = fz_empty_rect;
    static fz_point pageCenter = fz_make_point(0.f, 0.f);
    static float minZoom = 1.f, maxZoom = 2.f;
    static SDL_Rect viewport;

    void Init(void) {
        ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
        if (!ctx) {
            Log::Error("%s: Cannot create MuPDF context\n", __func__);
            return;
        }

        fz_try(ctx)
            fz_register_document_handlers(ctx);
        fz_catch(ctx) {
            Log::Error("%s: Cannot register document handlers: %s\n", __func__, fz_caught_message(ctx));
            fz_drop_context(ctx);
            ctx = nullptr;
        }
    }

    void Exit(void) {
        if (doc) {
            fz_drop_document(ctx, doc);
        }
        if (ctx) {
            fz_drop_context(ctx);
        }

        doc = nullptr;
        ctx = nullptr;
    }

    void OpenDocument(const std::string &path, Book &book) {
        if (!ctx) {
            return;
        }

        fz_try(ctx)
            doc = fz_open_document(ctx, path.c_str());
        fz_catch(ctx) {
            Log::Error("%s: Cannot open document: %s\n", __func__, fz_caught_message(ctx));
            return;
        }

        fz_try(ctx)
            book.page_count = fz_count_pages(ctx, doc);
        fz_catch(ctx) {
            Log::Error("%s: Cannot count pages: %s\n", __func__, fz_caught_message(ctx));
            fz_drop_document(ctx, doc);
            doc = nullptr;
            return;
        }

        if (book.page_number < 0 || book.page_number >= book.page_count) {
            Log::Error("%s: Page number out of range: %d (of %d)\n", __func__, book.page_number + 1, book.page_count);
            fz_drop_document(ctx, doc);
            doc = nullptr;
            return;
        }

        SDL_GetRenderViewport(GUI::GetRenderer(), &viewport);
        RenderPage(book);
    }

    static void CreatePageTexture(Book &book) {
        if (!pix || !pix->samples) {
            return;
        }

        SDL_Surface *surface = SDL_CreateSurfaceFrom(
            pix->w,
            pix->h,
            pix->n == 4 ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_RGB24,
            pix->samples,
            pix->stride
        );

        if (!surface) {
            Log::Error("%s: SDL_CreateSurfaceFrom failed: %s\n", __func__, SDL_GetError());
            return;
        }
        
        if (book.page) {
            SDL_DestroyTexture(book.page);
            book.page = nullptr;
        }

        book.page = SDL_CreateTextureFromSurface(GUI::GetRenderer(), surface);
        if (!book.page) {
            Log::Error("%s: SDL_CreateTextureFromSurface failed: %s\n", __func__, SDL_GetError());
        }

        book.width = pix->w;
        book.height = pix->h;

        SDL_DestroySurface(surface);
    }

    void ResetPosition(const Book& book) {
        pageCenter = fz_make_point((pageBounds.x1 * book.zoom) / 2.f,(pageBounds.y1 * book.zoom) / 2.f);
    }

    void RenderPage(Book &book) {
        if (page) {
            fz_drop_page(ctx, page);
            page = nullptr;
        }

        fz_try(ctx)
            page = fz_load_page(ctx, doc, book.page_number);
        fz_catch(ctx) {
            Log::Error("%s: Cannot load page: %s\n", __func__, fz_caught_message(ctx));
            return;
        }

        fz_rect bounds = fz_bound_page(ctx, page);

        if (pageBounds.x1 != bounds.x1 || pageBounds.y1 != bounds.y1) {
            pageBounds = bounds;
            
            float fitToHeightZoom = viewport.h / bounds.y1;
            
            minZoom = fitToHeightZoom;
            maxZoom = fmaxf(viewport.w / bounds.x1, viewport.h / bounds.y1);
        }


        ctm = fz_scale(book.zoom, book.zoom);
        ctm = fz_pre_rotate(ctm, book.rotate);

        fz_try(ctx)
            pix = fz_new_pixmap_from_page(ctx, page, ctm, fz_device_rgb(ctx), 0);
        fz_catch(ctx) {
            Log::Error("%s: Cannot render page: %s\n", __func__, fz_caught_message(ctx));
            return;
        }

        Reader::CreatePageTexture(book);
        fz_drop_pixmap(ctx, pix);
        pix = nullptr;
    }

    static void MovePage(Book &book, float x, float y) {
        float w = pageBounds.x1 * book.zoom;
        float h = pageBounds.y1 * book.zoom;

        pageCenter.x = fminf(fmaxf(pageCenter.x + x, w / 2.f), viewport.w - w / 2.f);
        pageCenter.y = fminf(fmaxf(pageCenter.y + y, viewport.h - h / 2.f), h / 2.f);
    }

    void SetZoom(Book &book, float value) {
        book.zoom = value;
        Reader::RenderPage(book);
        Reader::MovePage(book, 0.f, 0.f);
    }
}
