#ifndef _REFINERY_OUTPUT_H
#define _REFINERY_OUTPUT_H

#include <iosfwd>

namespace refinery {

template<typename T> class RGBPixel;
template<typename T> class TypedImage;
typedef TypedImage<RGBPixel<unsigned short> > RGBImage;

/**
 * Outputs images in PPM format.
 *
 * Refinery is made to be integrated with other image-processing libraries
 * which can output in numerous formats. This class is mostly useful for
 * debugging.
 */
class ImageWriter {
public:
  /**
   * Writes an image as PPM to an output stream.
   *
   * \param[in] image 16-bit RGB image.
   * \param[in] ostream Output stream where PPM data will be written.
   * \param[in] colorDepth 8 or 16, for 8-bit or 16-bit output.
   */
  void writeImage(
      const RGBImage& image, std::ostream& ostream, unsigned int colorDepth = 8);

  /**
   * Writes an image as PPM to an output stream.
   *
   * \param[in] image 16-bit RGB image.
   * \param[in] ostream Output filename where PPM data will be written.
   * \param[in] colorDepth 8 or 16, for 8-bit or 16-bit output.
   */
  void writeImage(
      const RGBImage& image, const char* filename, unsigned int colorDepth = 8);
};

} // namespace refinery

#endif /* REFINERY_OUTPUT_H */
