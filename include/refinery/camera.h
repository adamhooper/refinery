#ifndef _REFINERY_CAMERA_H
#define _REFINERY_CAMERA_H

namespace Exiv2 {
  class ExifData;
}

namespace refinery {

/*
 * A Camera holds static data for an actual camera model.
 *
 * Each camera model behaves slightly differently: different compression, for
 * instance, or different color profiles. Each camera model is represented by
 * a class which derives from Camera.
 *
 * Some Camera methods accept an Exiv2::ExifData& as a parameter. That's
 * because different cameras treat the same Exif tags differently.
 */
class Camera {
public:
  /*
   * Every camera has a different notion of R, G, B, C, Y, M, or K. This table
   * maps the camera's RGB or CMYK to XYZ.
   */
  struct ColorConversionData {
    short black;
    short maximum;
    double cameraToXyz[4][3];
    double cameraToRgb[4][3];
    double rgbToCamera[3][4];
    double xyzToCamera[3][4];
    double scalingMultipliers[4];
  };

  virtual const char* name() const = 0;
  virtual const char* make() const = 0;
  virtual const char* model() const = 0;
  virtual unsigned int colors() const = 0;
  virtual ColorConversionData colorConversionData() const = 0;
  virtual unsigned int orientation(const Exiv2::ExifData& exifData) const = 0;
  virtual bool canHandle(const Exiv2::ExifData& exifData) const = 0;
};

/*
 * A Camera& coupled with an Exiv2::ExifData&.
 *
 * This is the public face that callers care about. Callers shouldn't need to
 * know whether a default color profile comes hard-coded or embedded in Exif.
 * This class abstracts everything.
 *
 * CamerData instances are lightweight. Shallow-copy them at will.
 */
class CameraData {
  const Camera& mCamera;
  const Exiv2::ExifData& mExifData;

public:
  CameraData(const Camera& camera, const Exiv2::ExifData& exifData)
      : mCamera(camera), mExifData(exifData) {}
  CameraData(const CameraData& rhs)
      : mCamera(rhs.camera()), mExifData(rhs.exifData()) {}

  const Camera& camera() const { return mCamera; }
  const Exiv2::ExifData& exifData() const { return mExifData; }

  /* Returns the image orientation, a number from 1 to 8 (see TIFF docs) */
  unsigned int orientation() const;

  /* Returns the number of colors in the image */
  unsigned int colors() const;

  Camera::ColorConversionData colorConversionData() const;
};

/*
 * Returns Camera instances.
 *
 * Call CameraFactory::instance().detectCamera(exifData) to return a Camera
 * instance representing the camera model that shot the photo which includes
 * exifData.
 *
 * Each Camera is initialized once, statically. You can only pass Camera
 * instances by reference.
 *
 * If you want to find out anything about an actual photograph, you should look
 * to CameraDataFactory, not CameraFactory.
 */
class CameraFactory {
private:
  CameraFactory();
  ~CameraFactory();

public:
  static CameraFactory& instance();

  /*
   * Figures out which const Camera& applies to the given Exif data, and
   * returns it.
   *
   * The returned const Camera& should not be memory-managed. It will last the
   * duration of the program execution.
   *
   * If no known real-world camera could have produced the given Exif data
   * (that is, if a computer program did it), a Camera will still be returned.
   * It's called the NullCamera, and there's nothing special about it.
   */
  const Camera& detectCamera(const Exiv2::ExifData& exifData) const;
};

/*
 * Returns CameraData instances.
 *
 * Each photograph corresponds with one CameraData instance. Only the Exif data
 * is necessary to determine the photograph's properties.
 */
class CameraDataFactory {
public:
  static CameraDataFactory& instance();

  CameraData getCameraData(const Exiv2::ExifData& exifData) const;
};

} // namespace refinery

#endif /* _REFINERY_CAMERA_H */
