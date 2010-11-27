#include "refinery/image.h"

#include <boost/cstdint.hpp>

static const char *TIFF_LENGTH_CHECK = "11124811248488";

struct TiffTag {
  uint16_t tag;
  uint16_t type;
  uint32_t len;
  uint32_t newOffset;
};

class ImageReader_private
{
public:
  Image* readImage(InputStream& istream)
  {

  }
};

ImageReader::ImageReader()
  : mPrivate(new ImageReader_private)
{
}

ImageReader::~ImageReader()
{
  delete mPrivate;
}

Image* ImageReader::readImage(InputStream& istream)
{
  return mPrivate->readImage(istream);
}
