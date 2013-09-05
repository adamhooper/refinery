#ifndef _REFINERY_IMAGE_TILE_H
#define _REFINERY_IMAGE_TILE_H

namespace refinery {

/**
 * A scratch-pad partial Image with coordinate data to relate to a real one.
 *
 * \tparam T Image type.
 */
template<typename T>
class ImageTile {
public:
  typedef T ImageType; /**< Image type. */
  typedef typename T::PixelType PixelType; /**< Pixel type. */
  typedef typename T::ValueType ValueType; /**< Color value type. */
  typedef typename T::ColorType ColorType; /**< Color index type. */

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

  std::ptrdiff_t offsetForImagePoint(const Point& imagePoint) const
  {
    Point tilePoint(imagePoint - mTopLeft);
    return tilePoint.row * mSize.col + tilePoint.col;
  }

public:
  /**
   * Constructor.
   *
   * This will allocate memory corresponding to the \p size.
   *
   * \param[in] imageSize Pixel value one past the bottom-right of the Image.
   * \param[in] topLeft Image-realm pixel corresponding to this tile's (0,0).
   * \param[in] size Width and height of this tile.
   * \param[in] border Number of pixels in the full Image edges we never modify.
   * \param[in] margin Number of pixels we overlap adjacent tiles (read-only).
   */
  ImageTile(
      const Point& imageSize, const Point& topLeft, const Point& size,
      unsigned int border, unsigned int margin)
    : mImageSize(imageSize), mTopLeft(topLeft), mSize(size),
      mEdgeSize(static_cast<unsigned int>(border - margin))
  {
    this->allocate();
  }

  /**
   * Top pixel we can modify, relative to the full Image.
   */
  unsigned int top() const {
    return std::max<unsigned int>(mTopLeft.row, mEdgeSize);
  }
  /**
   * Leftmost pixel we can modify, relative to the full Image.
   */
  unsigned int left() const {
    return std::max<unsigned int>(mTopLeft.col, mEdgeSize);
  }
  /**
   * The number of pixel rows in the tile.
   */
  unsigned int height() const {
    return mSize.row;
  }
  /**
   * Number of pixel columns in the tile.
   */
  unsigned int width() const {
    return mSize.col;
  }
  /**
   * Bottom pixel we can modify, relative to the full Image.
   */
  unsigned int bottom() const {
    return std::min<unsigned int>(
        mImageSize.row - mEdgeSize,
        mTopLeft.row + mSize.row);
  }
  /**
   * Rightmost pixel we can modify, relative to the full Image.
   */
  unsigned int right() const {
    return std::min<unsigned int>(
        mImageSize.col - mEdgeSize,
        mTopLeft.col + mSize.col);
  }

  /**
   * Sets a new top-left, repurposing this scratch-pad.
   */
  void setTopLeft(const Point& topLeft) { mTopLeft = topLeft; }

  /**
   * Pixel pointer in original image using this tile's relative coordinates.
   */
  PixelType* pixelsAtImageCoords(const Point& point) {
    const std::ptrdiff_t offset(offsetForImagePoint(point));
    return &mPixels[offset];
  }
  /**
   * Pixel pointer in original image using this tile's relative coordinates.
   */
  PixelType* pixelsAtImageCoords(int row, int col) {
    return pixelsAtImageCoords(Point(row, col));
  }
  /**
   * Pixel pointer in original image using this tile's relative coordinates.
   */
  const PixelType* constPixelsAtImageCoords(const Point& point) const {
    const std::ptrdiff_t offset(offsetForImagePoint(point));
    return &mPixels[offset];
  }
  /**
   * Pixel pointer in original image using this tile's relative coordinates.
   */
  const PixelType* constPixelsAtImageCoords(int row, int col) const {
    return constPixelsAtImageCoords(Point(row, col));
  }
};

}; /* namespace refinery */

#endif /* _REFINERY_IMAGE_TILE_H */
