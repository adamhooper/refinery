#ifndef _REFINERY_IMAGE_TILE_H
#define _REFINERY_IMAGE_TILE_H

namespace refinery {

template<typename T, std::size_t N> class TypedImage;

template<typename T, std::size_t N = 3> class TypedImageTile {
public:
  typedef TypedImage<T,N> ImageType;
  typedef T ValueType;
  typedef ValueType (*PixelsType)[N];
  typedef const ValueType (*ConstPixelsType)[N];

private:
  typedef std::vector<ValueType> StorageType;
  Point mImageSize;
  Point mTopLeft;
  Point mSize;
  unsigned int mEdgeSize;
  StorageType mPixels;

  void allocate()
  {
    mPixels.assign(mSize.row * mSize.col * N, 0);
  }

  ptrdiff_t offsetForImagePoint(const Point& imagePoint) const
  {
    Point tilePoint(imagePoint - mTopLeft);
    return (tilePoint.row * mSize.col + tilePoint.col) * N;
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

  PixelsType pixelsAtImageCoords(const Point& point) {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return reinterpret_cast<PixelsType>(&mPixels[offset]);
  }
  PixelsType pixelsAtImageCoords(int row, int col) {
    return pixelsAtImageCoords(Point(row, col));
  }
  ConstPixelsType constPixelsAtImageCoords(const Point& point) const {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return reinterpret_cast<ConstPixelsType>(&mPixels[offset]);
  }
  ConstPixelsType constPixelsAtImageCoords(int row, int col) const {
    return constPixelsAtImageCoords(Point(row, col));
  }
};

typedef TypedImageTile<unsigned short, 3> RGBImageTile;
typedef RGBImageTile ImageTile;
typedef TypedImageTile<short, 3> LABImageTile;

}; /* namespace refinery */

#endif /* _REFINERY_IMAGE_TILE_H */
