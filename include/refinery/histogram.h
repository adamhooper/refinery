#ifndef _REFINERY_HISTOGRAM_H
#define _REFINERY_HISTOGRAM_H

#include <cstddef>
#include <limits>

namespace refinery {

/**
 * Says, for each color, how many of a picture's pixels hold each value.
 *
 * The Coarseness parameter decides to what extent pixels will be "grouped"
 * into histogram slots. For example, a Coarseness of 3 on an unsigned short
 * value means the histogram will have 2^13 slots, rather than 2^16
 * (because 16 - 3 = 13). The first green slot in the histogram will sum the
 * number of pixels with green values from 0 to 7 (7 = 2^3 - 1). A
 * Coarseness of 0 makes for an exact (but sometimes slower or less handy)
 * histogram.
 *
 * \tparam ImageType TypedImage instance.
 * \tparam Coarseness Shrinking option. For 16-bit TypedImage, 3 is nice.
 */
template<typename ImageType, unsigned int Coarseness = 0>
class Histogram {
public:
  /** Value type of each pixel color, for instance "unsigned short". */
  typedef typename ImageType::ValueType ValueType;
  /** Pixel type, for instance RGBPixel<unsigned char>. */
  typedef typename ImageType::PixelType PixelType;
  /** Color type, that is, index type. */
  typedef typename ImageType::ColorType ColorType;
  /** Number of colors per pixel. */
  static const ColorType NColors = PixelType::NColors;

private:
  typedef std::vector<unsigned int> CurveType;

  CurveType mCurves[NColors];
  unsigned int mNPixels;

  void init(const ImageType& image)
  {
    // XXX only works for unsigned integer types
    const std::size_t nSlots
      = (std::numeric_limits<ValueType>::max() >> Coarseness) + 1;

    for (ColorType c = 0; c < NColors; c++) {
      mCurves[c].assign(nSlots, 0);
    }

    const PixelType* pixel(image.constPixels());
    const PixelType* endPixel(image.constPixelsEnd());

    for (;pixel < endPixel; pixel++) {
      for (ColorType c = 0; c < NColors; c++) {
        mCurves[c][pixel->at(c) >> Coarseness]++;
      }
    }

    this->mNPixels = image.nPixels();
  }

public:
  /**
   * Constructor.
   *
   * Once the Image has been read, it is forgotten.
   *
   * \param[in] image Image to read when building this Histogram.
   */
  Histogram(const ImageType& image)
  {
    this->init(image);
  }

  /**
   * Number of distinct counting slots.
   *
   * For instance, if we're using a char-based ImageType and Coarseness is 0,
   * nSlots() will return 256. If Coarseness is 1, it'll be 128; 2, 64; etc.
   *
   * \return Number of slots.
   */
  inline unsigned int nSlots() const { return this->mCurves[0].size(); }

  /**
   * Number of pixels in the original image.
   *
   * This is useful in case we want to show, for instance, what percentage of
   * an image's pixels fall above or below a certain threshold.
   *
   * \return Pixel count.
   */
  inline unsigned int nPixels() const { return this->mNPixels; }

  /**
   * Number of pixels in a particular slot.
   *
   * This is indexed by slot, not by pixel value.
   *
   * \param color Color.
   * \param slot Slot, from 0 to nSlots()-1.
   * \return Number of pixels of the colors specified by the slot.
   */
  inline unsigned short count(const ColorType& color, std::size_t slot) const
  {
    return mCurves[color][slot];
  }
};

} // namespace refinery

#endif /* _REFINERY_HISTOGRAM_H */
