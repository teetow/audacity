#include <QImage>

#include "WaveformPainter.h"
#include "au3-screen-geometry/ZoomInfo.h"

namespace au::projectscene {
namespace {
//! A selection range that contains every pixel of a clip, so that the second bitmap cache
//! renders every column in the selected colors and never has to be rebuilt when the selection moves
constexpr double ALL_SELECTED_START_TIME = -1.0e7;
constexpr double ALL_SELECTED_END_TIME = 1.0e9;
}

struct WaveBitmapCacheElementQt final : public WaveBitmapCacheElement
{
    uint8_t* Allocate(size_t width, size_t height) override
    {
        mWidth = width;
        mHeight = height;

        mBytes.resize(std::max(mWidth * mHeight * 3, mBytes.size()));
        mImageDirty = true;
        return mBytes.data();
    }

    //! The cache writes packed RGB888, which QPainter converts pixel by pixel on every draw.
    //! Converting once per update to RGB32 lets every later draw be a plain copy.
    const QImage& GetImage() const
    {
        if (mImageDirty) {
            mImage = QImage(mBytes.data(), static_cast<int>(mWidth), static_cast<int>(mHeight),
                            static_cast<qsizetype>(mWidth * 3), QImage::Format_RGB888)
                     .convertToFormat(QImage::Format_RGB32);
            mImageDirty = false;
        }

        return mImage;
    }

    size_t Width() const override
    {
        return mWidth;
    }

    size_t Height() const override
    {
        return mHeight;
    }

private:
    size_t mWidth{};
    size_t mHeight{};
    std::vector<uint8_t> mBytes;

    mutable QImage mImage;
    mutable bool mImageDirty = true;
};

static au::au3::Au3WaveClip::Attachments::RegisteredFactory sKeyW{ [](au::au3::Au3WaveClip&) {
        return std::make_unique<WaveformPainter>();
    } };

WaveformPainter& WaveformPainter::Get(const au::au3::Au3WaveClip& clip)
{
    return const_cast< au::au3::Au3WaveClip& >(clip)   // Consider it mutable data
           .Attachments::Get<WaveformPainter>(sKeyW).EnsureClip(clip);
}

WaveformPainter& WaveformPainter::EnsureClip(const au::au3::Au3WaveClip& clip)
{
    const auto changed = mChanged.exchange(false);
    if (&clip != mWaveClip || changed) {
        mChannelCaches.clear();
    }

    const auto nChannels = clip.NChannels();

    if (mChannelCaches.size() == nChannels) {
        return *this;
    }

    mWaveClip = &clip;

    mChannelCaches.reserve(nChannels);

    const auto makeElement = [] { return std::make_unique<WaveBitmapCacheElementQt>(); };

    for (size_t channelIndex = 0; channelIndex < nChannels; ++channelIndex) {
        auto dataCache = std::make_shared<WaveDataCache>(clip, channelIndex);

        auto bitmapCache = std::make_unique<WaveBitmapCache>(clip, dataCache, makeElement);
        auto selectedBitmapCache = std::make_unique<WaveBitmapCache>(clip, dataCache, makeElement);

        mChannelCaches.push_back({ std::move(dataCache), std::move(bitmapCache), std::move(selectedBitmapCache) });
    }

    return *this;
}

namespace {
template<typename Range>
void drawRange(QPainter& painter, const Range& range, double left, int top, int height)
{
    for (auto it = range.begin(); it != range.end(); ++it) {
        const auto elementLeftOffset = it.GetLeftOffset();
        const auto elementRightOffset = it.GetRightOffset();

        const auto width = WaveBitmapCache::CacheElementWidth - elementLeftOffset - elementRightOffset;

        const auto drawableWidth = std::min<int32_t>(width, it->Width() - elementLeftOffset);

        const QImage& image = static_cast<const WaveBitmapCacheElementQt&>(*it).GetImage();
        painter.drawImage(
            QRectF(left, top, drawableWidth, height),
            image,
            QRectF(
                elementLeftOffset,
                0,
                std::clamp(drawableWidth, 0, image.width() - static_cast<int>(elementLeftOffset)),
                std::clamp(height, 0, image.height())
                )
            );

        left += width;
    }
}
}

void WaveformPainter::Draw(size_t channelIndex,
                           QPainter& painter,
                           const WavePaintParameters& params,
                           const au::projectscene::WaveMetrics& metrics)
{
    assert(channelIndex < mChannelCaches.size());
    if (channelIndex >= mChannelCaches.size()) {
        return;
    }

    const int top = static_cast<int>(metrics.top);
    const int height = static_cast<int>(metrics.height);
    const double left = metrics.left;
    if (height <= 0) {
        return;
    }

    const ZoomInfo zoomInfo(0.0, metrics.zoom);

    //! NOTE The time selection is not rendered into the tiles: one cache holds every column in
    //! the normal colors and the other in the selected colors, and the selection only decides
    //! which part of each is shown. Moving the selection then costs a few copies instead of
    //! rasterizing every tile of the clip again.
    auto& caches = mChannelCaches[channelIndex];

    WaveBitmapCache& normalCache = *caches.BitmapCache;
    normalCache.SetPaintParameters(params);
    normalCache.SetSelection(zoomInfo, 0.0, 0.0, false);
    drawRange(painter, normalCache.PerformLookup(zoomInfo, metrics.fromTime, metrics.toTime), left, top, height);

    //! Same rule as WaveBitmapCache::SetSelection, so the selected columns do not change
    const bool hasSelection = metrics.selectionStartTime <= metrics.selectionEndTime
                              && (metrics.selectionEndTime - metrics.selectionStartTime) >= std::numeric_limits<double>::epsilon();
    if (hasSelection) {
        const int64_t fromPixel = zoomInfo.TimeToPosition(metrics.fromTime);
        const int64_t toPixel = zoomInfo.TimeToPosition(metrics.toTime) + 1;
        const int64_t selectionFirst = zoomInfo.TimeToPosition(metrics.selectionStartTime);
        const int64_t selectionLast = std::max(zoomInfo.TimeToPosition(metrics.selectionEndTime), selectionFirst + 1);

        const int64_t first = std::max(selectionFirst, fromPixel);
        const int64_t last = std::min(selectionLast, toPixel);

        if (first < last) {
            WaveBitmapCache& selectedCache = *caches.SelectedBitmapCache;
            selectedCache.SetPaintParameters(params);
            selectedCache.SetSelection(zoomInfo, ALL_SELECTED_START_TIME, ALL_SELECTED_END_TIME, true);

            //! Look up a pixel more than needed on each side, the clip rect trims it to the selection
            const double pixelTime = 1.0 / zoomInfo.GetZoom();
            const double lookupFrom = std::max(metrics.fromTime, metrics.selectionStartTime - pixelTime);
            const double lookupTo = std::min(metrics.toTime, metrics.selectionEndTime + pixelTime);
            const int64_t lookupFromPixel = zoomInfo.TimeToPosition(lookupFrom);

            painter.save();
            painter.setClipRect(QRectF(left + (first - fromPixel), top, last - first, height), Qt::IntersectClip);
            drawRange(painter, selectedCache.PerformLookup(zoomInfo, lookupFrom, lookupTo),
                      left + (lookupFromPixel - fromPixel), top, height);
            painter.restore();
        }
    }
}

void WaveformPainter::MarkChanged() noexcept
{
    mChanged.store(true);
}

void WaveformPainter::Invalidate()
{
    for (auto& channelCache : mChannelCaches) {
        channelCache.DataCache->Invalidate();
        channelCache.BitmapCache->Invalidate();
        channelCache.SelectedBitmapCache->Invalidate();
    }
}

std::unique_ptr<WaveClipListener> WaveformPainter::Clone() const
{
    return std::make_unique<WaveformPainter>();
}
}
