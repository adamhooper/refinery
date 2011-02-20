#ifndef _REFINERY_FILTERS_H
#define _REFINERY_FILTERS_H

#include <cstddef>

namespace refinery {

/**
 * Scale an Image to fill its data-type.
 *
 * This is intended for use with RAW GrayImages. The Image's CameraData
 * contains information on how much each color should be scaled. This filter
 * reads that and multiplies each pixel according to what color its sensor is.
 *
 * In other words, the pixel values that come from the camera, which are
 * sometimes 12- or 14-bit, need to be scaled to 16-bit. The reason the values
 * don't come pre-scaled is that each color needs a different multiplier.
 */
class ScaleColorsFilter {
public:
  /**
   * Multiplies \p image's colors according to its CameraData.
   */
  template<typename T> void filter(T& image);
};

/**
 * Convert an Image from camera colors to sRGB.
 *
 * This relies on the Image's CameraData, which specifies what the camera
 * colors actually mean in sRGB colorspace.
 */
class ConvertToRgbFilter {
public:
  /**
   * Convert \p image from its pseudo-RGB colors to sRGB.
   */
  template<typename T> void filter(T& image);
};

/**
 * Gamma-correct an Image with a GammaCurve.
 *
 * A suitable GammaCurve can be calculated automatically from a Histogram.
 * For instance:
 *
 * \code
 * typedef Image<RGBPixel<unsigned short> > ImageType;
 * ImageType image;
 * // ... fill in image ...
 * Histogram<ImageType, 3> histogram(image);
 * GammaCurve<ImageType::ValueType> gammaCurve(histogram);
 * GammaFilter gammaFilter;
 * gammaFilter.filter(image, gammaCurve);
 * \endcode
 */
class GammaFilter {
public:
  /**
   * Gamma-correct \p image using \p gammaCurve.
   *
   * This ought to be a 1-param template but Swig can't resolve it.
   *
   * \param[in,out] image Image to gamma-correct.
   * \param[in] gammaCurve Gamma curve to apply.
   * \tparam ImageType Image type (can be inferred).
   * \tparam GammaCurveType Gamma-curve type (can be inferred).
   */
  template<typename ImageType, typename GammaCurveType>
    void filter(ImageType& image, const GammaCurveType& gammaCurve);
};

} // namespace refinery

#endif /* REFINERY_FILTERS_H */
