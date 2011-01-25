#include "refinery/filters.h"

#include "refinery/camera.h"
#include "refinery/color.h"
#include "refinery/image.h"

namespace refinery {

namespace {
  template<typename T>
  class ScaleColorsFilterImpl {
  public:
    typedef T ImageType;
    typedef typename ImageType::ColorType ColorType;
    typedef typename ImageType::PixelType PixelType;
    typedef typename ImageType::ValueType ValueType;

  private:
    ImageType& mImage;
    const CameraData& mCameraData;

    ValueType clamp16(int val) {
      if (val < 0) return 0;
      if (val > 0xffff) return 0xffff;
      return val;
      /*
      // Assume right-shift of signed values leaves 1s, not 0s
      const unsigned int trashBits = val >> 16;
      return trashBits ? (~trashBits >> 16) : val;
      */
    }

  public:
    ScaleColorsFilterImpl(ImageType& image)
        : mImage(image), mCameraData(image.cameraData()) {}

    void filter() {
      Camera::ColorConversionData colorData(mCameraData.colorConversionData());

      const unsigned int height(mImage.height());
      const unsigned int width(mImage.width());

      for (unsigned int row = 0; row < height; row++) {
        PixelType* pix(mImage.pixelsAtRow(row));
        const PixelType* lastPixel(mImage.constPixelsAtPoint(row, width - 1));

        Image::ColorType c1 = mImage.colorAtPoint(Point(row, 0));
        Image::ColorType c2 = mImage.colorAtPoint(Point(row, 1));

        double multiplier1 = colorData.scalingMultipliers[c1];
        double multiplier2 = colorData.scalingMultipliers[c2];

        while (pix < lastPixel) {
          pix->value = clamp16(multiplier1 * pix->value);
          pix++;
          pix->value = clamp16(multiplier2 * pix->value);
          pix++;
        }
        if (pix == lastPixel) {
          pix->value = clamp16(multiplier1 * pix->value);
        }
      }
    }
  };

  template<typename T>
  class ConvertToRgbFilterImpl {
  public:
    typedef T ImageType;
    typedef typename ImageType::ColorType ColorType;
    typedef typename ImageType::PixelType PixelType;
    typedef typename ImageType::ValueType ValueType;

  private:
    ImageType& mImage;
    const CameraData& mCameraData;

    unsigned short clamp16(int val) {
      // Assume right-shift of signed values leaves 1s, not 0s
      const unsigned int trashBits = val >> 16;
      return trashBits ? (~trashBits >> 16) : val;
    }

  public:
    ConvertToRgbFilterImpl(ImageType& image)
        : mImage(image), mCameraData(image.cameraData()) {}

    void filter() {
      Camera::ColorConversionData colorData(mCameraData.colorConversionData());

      ColorConverter<float, 4, 3> converter(colorData.cameraToRgb);

      const unsigned int height(mImage.height());
      const unsigned int width(mImage.width());

      for (unsigned int row = 0; row < height; row++) {
        PixelType* pixels(mImage.pixelsAtRow(row));
        for (unsigned int col = 0; col < width; col++, pixels++) {
          RGBPixel<float> rgb;
          converter.convert(pixels[0], rgb);

          pixels[0][0] = clamp16(rgb.r());
          pixels[0][1] = clamp16(rgb.g());
          pixels[0][2] = clamp16(rgb.b());
        }
      }
    }
  };
} // namespace {}

template<typename T>
void ScaleColorsFilter::filter(T& image)
{
  ScaleColorsFilterImpl<T> impl(image);
  impl.filter();
}

template<typename T>
void ConvertToRgbFilter::filter(T& image)
{
  ConvertToRgbFilterImpl<T> impl(image);
  impl.filter();
}

// Instantiate the ones we need... (hack-ish)
template void ScaleColorsFilter::filter<GrayImage>(GrayImage&);
template class ScaleColorsFilterImpl<GrayImage>;
template void ConvertToRgbFilter::filter<RGBImage>(RGBImage&);
template class ConvertToRgbFilterImpl<RGBImage>;

};
