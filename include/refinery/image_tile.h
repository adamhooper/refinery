#ifndef _REFINERY_IMAGE_TILE_H
#define _REFINERY_IMAGE_TILE_H

namespace refinery {

template<typename T> class RGBPixel;
template<typename T> class TypedImage;
typedef TypedImage<RGBPixel<unsigned short> > RGBImage;

template<typename T> class TypedImageTile {
public:
  typedef T ImageType;
  typedef typename T::PixelType PixelType;
  typedef typename T::ValueType ValueType;
  typedef typename T::ColorType ColorType;

private:
  typedef std::vector<PixelType> PixelsType;
  Point mImageSize;
  Point mTopLeft;
  Point mSize;
  unsigned int mEdgeSize;
  PixelsType mPixels;

  void allocate()
  {
    mPixels.assign(mSize.row * mSize.col, PixelType());
  }

  ptrdiff_t offsetForImagePoint(const Point& imagePoint) const
  {
    Point tilePoint(imagePoint - mTopLeft);
    return tilePoint.row * mSize.col + tilePoint.col;
  }

public:
  TypedImageTile(
      const Point& imageSize, const Point& topLeft, const Point& size,
      unsigned int border, unsigned int margin)
    : mImageSize(imageSize), mTopLeft(topLeft), mSize(size),
      mEdgeSize(static_cast<unsigned int>(border - margin))
  {
    this->allocate();
  }

  unsigned int top() const {
    return std::max<unsigned int>(mTopLeft.row, mEdgeSize);
  }
  unsigned int left() const {
    return std::max<unsigned int>(mTopLeft.col, mEdgeSize);
  }
  unsigned int height() const {
    return mSize.row;
  }
  unsigned int width() const {
    return mSize.col;
  }
  unsigned int bottom() const {
    return std::min<unsigned int>(
        mImageSize.row - mEdgeSize,
        mTopLeft.row + mSize.row);
  }
  unsigned int right() const {
    return std::min<unsigned int>(
        mImageSize.col - mEdgeSize,
        mTopLeft.col + mSize.col);
  }

  void setTopLeft(const Point& topLeft) { mTopLeft = topLeft; }
  void setSize(int height, int width) {
    Point newSize(height, width);
    if (newSize != mSize) {
      mSize = newSize;
      this->allocate();
    }
  }

  PixelType* pixelsAtImageCoords(const Point& point) {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return &mPixels[offset];
  }
  PixelType* pixelsAtImageCoords(int row, int col) {
    return pixelsAtImageCoords(Point(row, col));
  }
  const PixelType* constPixelsAtImageCoords(const Point& point) const {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return &mPixels[offset];
  }
  const PixelType* constPixelsAtImageCoords(int row, int col) const {
    return constPixelsAtImageCoords(Point(row, col));
  }
};

typedef TypedImageTile<RGBImage> RGBImageTile;
typedef TypedImageTile<LABImage> LABImageTile;

}; /* namespace refinery */

#endif /* _REFINERY_IMAGE_TILE_H */
