#ifndef _REFINERY_IMAGE_H
#define _REFINERY_IMAGE_H

#include <refinery/camera.h>

#include <vector>

namespace refinery {

class CameraData;

/**
 * A pixel coordinate counted from the top-left.
 *
 * This can also represent a pixel-coordinate offset. In other words, you can
 * add or subtract Point instances.
 */
struct Point {
  int row; /**< Pixel row. */
  int col; /**< Pixel column. */

  /**
   * Empty constructor.
   */
  Point() : row(0), col(0) {}
  /**
   * Constructor.
   *
   * \param[in] row Pixel row.
   * \param[in] col Pixel column.
   */
  Point(int row, int col) : row(row), col(col) {}
  /**
   * Copy constructor.
   *
   * \param[in] rhs Other Point.
   */
  Point(const Point& rhs) : row(rhs.row), col(rhs.col) {}

  /**
   * Add an offset to this Point.
   *
   * \param[in] point Point offset to add to this one.
   * \return A Point representing the sum.
   */
  Point operator+(const Point& point) const {
    return Point(row + point.row, col + point.col);
  }

  /**
   * Subtract an offset from this Point.
   *
   * \param[in] point Point offset to subtract from this one.
   * \return A Point representing the sum.
   */
  Point operator-(const Point& point) const {
    return Point(row - point.row, col - point.col);
  }

  /**
   * Compare this Point to another.
   *
   * \param[in] rhs Other Point.
   * \return \c true iff this Point is equivalent to the other.
   */
  bool operator==(const Point& rhs) const {
    return this->row == rhs.row && this->col == rhs.col;
  }

  /**
   * Compare this Point to another.
   *
   * \param[in] rhs Other Point.
   * \return \c true iff this Point isn't equivalent to the other.
   */
  bool operator!=(const Point& rhs) const {
    return !(this->operator==(rhs));
  }
};

/**
 * A pixel, consisting of one or more color values.
 *
 * \tparam T Type of each color value.
 * \tparam N Number of colors.
 */
template<typename T, unsigned int N>
struct Pixel {
  typedef unsigned int ColorType; /**< Datatype to access a color with at(). */
  static const ColorType NColors = N; /**< Number of colors. */
  typedef T ValueType; /**< Value type of each color (e.g.\ unsigned short). */
  typedef T ArrayType[NColors]; /**< Type returned from array(). */
  ArrayType colors; /**< Raw data. */

  /**
   * Empty constructor.
   */
  Pixel() {
    for (ColorType c = 0; c < N; c++) {
      colors[c] = 0;
    }
  }
  /**
   * Constructor.
   *
   * \param[in] rhs N-element array to copy into this type.
   * \tparam U Type of the input array (usually inferred).
   */
  template<typename U> Pixel(const U (&rhs)[N]) {
    for (ColorType c = 0; c < N; c++) {
      colors[c] = rhs[c];
    }
  }
  /**
   * Copy constructor.
   *
   * \param[in] rhs Another N-color Pixel.
   * \tparam U ValueType from the input Pixel (usually inferred).
   */
  template<typename U> Pixel(const Pixel<U, N>& rhs) {
    for (ColorType c = 0; c < N; c++) {
      colors[c] = rhs.colors[c];
    }
  }
  /**
   * The Pixel as an array.
   *
   * Changes to this array will modify the underlying Pixel.
   *
   * This method is inlined, so it usually takes 0 CPU instructions.
   *
   * \return An N-element array.
   */
  inline ArrayType& array() { return colors; }
  /**
   * The Pixel as an const array.
   *
   * This method is inlined, so it usually takes 0 CPU instructions.
   *
   * \return A 3-element const array.
   */
  inline const ArrayType& constArray() const { return colors; }
  /**
   * A mutable reference to a single color value.
   *
   * \param[in] index Color to access.
   * \return A mutable reference to the color value.
   */
  inline T& operator[](const ColorType& index) { return colors[index]; }
  /**
   * A single color value.
   *
   * This is like operator[] but works for a const Pixel.
   *
   * \param[in] index Color to access.
   * \return The color value.
   */
  inline const T& at(const ColorType& index) const { return colors[index]; }
};

/**
 * A 3-color pixel with R, G and B values.
 *
 * This is simply a Pixel with some convenience accessors.
 *
 * \tparam T Type of each color value.
 */
template<typename T>
struct RGBPixel : public Pixel<T, 3>
{
  /**
   * Empty constructor.
   */
  RGBPixel() : Pixel<T, 3>() {}
  /**
   * Constructor.
   *
   * \param[in] rhs 3-element array to copy into this type.
   * \tparam U Type of the input array (usually inferred).
   */
  template<typename U> RGBPixel(const U (&rhs)[3]) : Pixel<T, 3>(rhs) {}
  /**
   * Copy constructor.
   *
   * \param[in] rhs Another 3-color Pixel.
   * \tparam U ValueType from the input Pixel (usually inferred).
   */
  template<typename U> RGBPixel(const RGBPixel<U>& rhs) : Pixel<T, 3>(rhs) {}
  /**
   * The red value.
   */
  inline const T& r() const { return this->at(0); }
  /**
   * The green value.
   */
  inline const T& g() const { return this->at(1); }
  /**
   * The blue value.
   */
  inline const T& b() const { return this->at(2); }
  /**
   * A mutable reference to the red value.
   */
  inline T& r() { return this->operator[](0); }
  /**
   * A mutable reference to the green value.
   */
  inline T& g() { return this->operator[](1); }
  /**
   * A mutable reference to the blue value.
   */
  inline T& b() { return this->operator[](2); }
};

/**
 * A 3-color pixel with L, A and B values.
 *
 * This is simply a Pixel with some convenience accessors.
 *
 * \tparam T Type of each color value.
 */
template<typename T>
struct LABPixel : public Pixel<T, 3>
{
  /**
   * Empty constructor.
   */
  LABPixel() : Pixel<T, 3>() {}
  /**
   * Constructor.
   *
   * \param[in] rhs 3-element array to copy into this type.
   * \tparam U Type of the input array (usually inferred).
   */
  template<typename U> LABPixel(const U (&rhs)[3]) : Pixel<T, 3>(rhs) {}
  /**
   * Copy constructor.
   *
   * \param[in] rhs Another 3-color Pixel.
   * \tparam U ValueType from the input Pixel (usually inferred).
   */
  template<typename U> LABPixel(const LABPixel<U>& rhs) : Pixel<T, 3>(rhs) {}
  /**
   * The luminance value.
   */
  inline const T& l() const { return this->at(0); }
  /**
   * The A value.
   */
  inline const T& a() const { return this->at(1); }
  /**
   * The B value.
   */
  inline const T& b() const { return this->at(2); }
  /**
   * A mutable reference to the luminance value.
   */
  inline T& l() { return this->operator[](0); }
  /**
   * A mutable reference to the A value.
   */
  inline T& a() { return this->operator[](1); }
  /**
   * A mutable reference to the B value.
   */
  inline T& b() { return this->operator[](2); }
};

/**
 * A 1-color pixel.
 *
 * \tparam T Type of each pixel value.
 */
template<typename T>
struct GrayPixel : public Pixel<T, 1> {
  /**
   * Empty constructor.
   */
  GrayPixel() : Pixel<T, 1>() {}
  /**
   * Constructor.
   *
   * \param[in] rhs 1-element array to copy into this type.
   * \tparam U Type of the input array (usually inferred).
   */
  template<typename U> GrayPixel(const U (&rhs)[1]) : Pixel<T, 1>(rhs) {}
  /**
   * Constructor.
   *
   * \param[in] value Value to copy into this type.
   */
  GrayPixel(const T& value) : Pixel<T, 1>() { this[0] = value; }
  /**
   * Copy constructor.
   *
   * \param[in] rhs Another 1-color Pixel.
   * \tparam U ValueType from the input Pixel (usually inferred).
   */
  template<typename U> GrayPixel(const GrayPixel<U>& rhs) : Pixel<T, 3>(rhs) {}

  /**
   * A mutable reference to the pixel value.
   */
  T& value() { return this->operator[](0); }
  /**
   * The pixel value.
   */
  const T& value() const { return this->at(0); }

  /**
   * Build a scaled pixel based on this one.
   *
   * \param[in] rhs What factor to scale this pixel by.
   * \tparam U Type of scaling factor.
   * \return A GrayPixel result of the multiplication.
   */
  template<typename U> GrayPixel& operator*(const U& rhs) {
    return GrayPixel(value() * rhs);
  }
};

/**
 * A grid of pixels with accompanying metadata.
 *
 * The image pixels are contiguous and can be accessed as one big array using
 * pixels().
 *
 * \tparam T The type of Pixel in the grid (use a Pixel).
 */
template<typename T>
class Image {
public:
  typedef T PixelType; /**< Type of each pixel. */
  typedef typename T::ValueType ValueType; /**< Color value type. */
  typedef typename T::ColorType ColorType; /**< Color index type. */

private:
  typedef std::vector<PixelType> PixelsType;
  CameraData mCameraData;
  int mWidth;
  int mHeight;
  int mFilters;
  PixelsType mPixels;

  void allocate()
  {
    mPixels.assign(mWidth * mHeight, PixelType());
  }

public:
  /**
   * Constructor.
   *
   * This allocates the space needed to store all the Pixels and sets them to
   * default values of 0.
   *
   * \param[in] cameraData CameraData that applies to the photograph.
   * \param[in] width Image width in pixels.
   * \param[in] height Image height in pixels.
   */
  Image(const CameraData& cameraData, int width, int height)
    : mCameraData(cameraData), mWidth(width), mHeight(height),
    mFilters(mCameraData.filters())
  {
    this->allocate();
  }

  /**
   * The photograph CameraData.
   */
  const CameraData& cameraData() const { return mCameraData; }
  /**
   * Image width in pixels.
   */
  unsigned int width() const { return mWidth; }
  /**
   * Image height in pixels.
   */
  unsigned int height() const { return mHeight; }
  /**
   * Number of pixels in the Image.
   */
  unsigned int nPixels() const { return mWidth * mHeight; }
  /**
   * Filters (cached from cameraData()) FIXME remove this.
   */
  unsigned int filters() const { return mFilters; }
  /**
   * Filters (cached from cameraData()) FIXME remove this.
   *
   * \param[in] filters New filters to set.
   */
  void setFilters(unsigned int filters) { mFilters = filters; }

  /**
   * The color of the camera sensor array at this point.
   *
   * Even in a fully-interpolated RGB image, it's sometimes useful to know
   * which color value is exact and which are interpolated. This method will
   * tell that for any type Image.
   *
   * \param[in] point Point in question.
   * \return Color in the sensor array, as reported by the camera.
   */
  ColorType colorAtPoint(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return (mFilters >> (((row << 1 & 14) | (col & 1)) << 1)) & 3;
  }
  /**
   * The color of the camera sensor array at this point.
   *
   * Even in a fully-interpolated RGB image, it's sometimes useful to know
   * which color value is exact and which are interpolated. This method will
   * tell that for any type Image.
   *
   * \param[in] row Pixel row (from the top).
   * \param[in] col Pixel column (from the left).
   * \return Color in the sensor array, as reported by the camera.
   */
  ColorType colorAtPoint(unsigned int row, unsigned int col) const {
    return colorAtPoint(Point(row, col));
  }

  /**
   * A pointer to the first pixel in the image, useful for iterating.
   *
   * Example: convert an RGB image to BGR
   * \code
   * typedef Image<Pixel<unsigned char, 3> > ImageType;
   * CameraData cameraData = findCameraDataSomehow();
   * ImageType image(cameraData, 100, 100);
   * // fill in image...
   * for (ImageType::PixelType* p = image.pixels(), p < image.pixelsEnd(); p++)
   * {
   *   ImageType::PixelType& pixel(*p);
   *   ImageType::ValueType temp(pixel[0]);
   *   pixel[0] = pixel[2];
   *   pixel[2] = temp;
   * }
   * \endcode
   *
   * \return A pixel pointer.
   */
  PixelType* pixels() { return &mPixels[0]; }
  /**
   * A pointer to one past the last pixel in the image, useful for iterating.
   */
  PixelType* pixelsEnd() { return &mPixels[mWidth * mHeight]; }

  /**
   * A pointer to the first pixel in the image, useful for iterating.
   */
  const PixelType* constPixels() const { return &mPixels[0]; }
  /**
   * A pointer to one past the last pixel in the image, useful for iterating.
   */
  const PixelType* constPixelsEnd() const { return &mPixels[mWidth * mHeight]; }

  /**
   * A pointer to the first pixel in the given row, useful for iterating.
   *
   * \param[in] row Pixel row (from the top).
   * \return A pixel pointer.
   */
  const PixelType* constPixelsAtRow(int row) const {
    return &mPixels[row * mWidth];
  }
  /**
   * A pointer to the first pixel in the given row, useful for iterating.
   *
   * \param[in] row Pixel row (from the top).
   * \return A pixel pointer.
   */
  PixelType* pixelsAtRow(int row) {
    return &mPixels[row * mWidth];
  }

  /**
   * A pointer to the specified pixel, useful for iterating.
   *
   * \param[in] point Point of the pixel.
   * \return A pixel pointer.
   */
  PixelType* pixelsAtPoint(const Point& point) {
    int row = point.row;
    int col = point.col;
    return &mPixels[row * mWidth + col];
  }
  /**
   * A pointer to the specified pixel, useful for iterating.
   *
   * \param[in] row Pixel row (from the top).
   * \param[in] col Pixel column (from the left).
   * \return A pixel pointer.
   */
  PixelType* pixelsAtPoint(unsigned int row, unsigned int col) {
    return pixelsAtPoint(Point(row, col));
  }
  /**
   * A pointer to the specified pixel, useful for iterating.
   *
   * \param[in] point Point of the pixel.
   * \return A pixel pointer.
   */
  const PixelType* constPixelsAtPoint(const Point& point) const {
    int row = point.row;
    int col = point.col;
    return &mPixels[row * mWidth + col];
  }
  /**
   * A pointer to the specified pixel, useful for iterating.
   *
   * \param[in] row Pixel row (from the top).
   * \param[in] col Pixel column (from the left).
   * \return A pixel pointer.
   */
  const PixelType* constPixelsAtPoint(
      unsigned int row, unsigned int col) const {
    return constPixelsAtPoint(Point(row, col));
  }

  /**
   * A mutable pixel at the specified point.
   */
  PixelType& pixelAtPoint(const Point& point) {
    return *pixelsAtPoint(point);
  }
  /**
   * A mutable pixel at the specified point.
   */
  PixelType& pixelAtPoint(unsigned int row, unsigned int col) {
    return *pixelsAtPoint(row, col);
  }
  /**
   * An immutable pixel at the specified point.
   */
  const PixelType& constPixelAtPoint(const Point& point) const {
    return *constPixelsAtPoint(point);
  }
  /**
   * An immutable pixel at the specified point.
   */
  const PixelType& constPixelAtPoint(unsigned int row, unsigned int col) const {
    return *constPixelsAtPoint(row, col);
  }
};

typedef RGBPixel<unsigned short> u16RGBPixel;
typedef LABPixel<short> s16LABPixel;
typedef GrayPixel<unsigned short> u16GrayPixel;

typedef Image<u16RGBPixel> RGBImage;
typedef Image<s16LABPixel> LABImage;
typedef Image<u16GrayPixel> GrayImage;

}; /* namespace refinery */

#endif /* REFINERY_IMAGE_H */
