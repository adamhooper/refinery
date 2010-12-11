#include "refinery/image.h"
#include "refinery/interpolate.h"
#include "refinery/output.h"
#include "refinery/unpack.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <exiv2/tags.hpp>

using namespace refinery;

long getLong(const Exiv2::ExifData& exifData, const char* key)
{
  Exiv2::ExifData::const_iterator it(exifData.findKey(Exiv2::ExifKey(key)));
  if (it != exifData.end()) {
    return (*it).toLong();
  }

  return 0; // arbitrary
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " INFILE OUTFILE" << std::endl;
    return 1;
  }

  Exiv2::Image::AutoPtr exivImage(Exiv2::ImageFactory::open(argv[1]));
  assert(exivImage.get() != 0);

  exivImage->readMetadata();
  Exiv2::ExifData& exifData(exivImage->exifData());
  if (exifData.empty()) {
    throw Exiv2::Error(1, std::string(argv[1]) + " is missing Exif data");
  }

  refinery::UnpackSettings settings;

  settings.bps = getLong(exifData, "Exif.SubImage2.BitsPerSample");
  settings.width = getLong(exifData, "Exif.SubImage2.ImageWidth");
  settings.height = getLong(exifData, "Exif.SubImage2.ImageLength");
  settings.length = getLong(exifData, "Exif.SubImage2.StripByteCounts");

  settings.format = refinery::UnpackSettings::FORMAT_NEF_COMPRESSED_LOSSY_2;
      // Exif.SubImage2.Compression, Exif.Nikon3.NEFCompression

  settings.filters = 0x4b4b4b4b;

  Exiv2::ExifData::const_iterator linIterator(
      exifData.findKey(Exiv2::ExifKey("Exif.Nikon3.LinearizationTable")));
  if (linIterator == exifData.end()) {
    throw Exiv2::Error(
        1, std::string(argv[1]) + " doesn't have a linearization table");
  }
  const Exiv2::Exifdatum& linDatum(*linIterator);
  std::vector<Exiv2::byte> linBytes(linDatum.size(), 0);
  linDatum.copy(&linBytes[0], Exiv2::bigEndian);

  // (see http://lclevy.free.fr/nef/ to learn about the linearization curve)
  settings.version0 = linBytes[0];
  settings.version1 = linBytes[1];
  settings.vpred[0][0] = linBytes[2] << 8 | linBytes[3];
  settings.vpred[0][1] = linBytes[4] << 8 | linBytes[5];
  settings.vpred[1][0] = linBytes[6] << 8 | linBytes[7];
  settings.vpred[1][1] = linBytes[8] << 8 | linBytes[9];

  unsigned int nLinCurveBytes =
      linBytes[10] << 9 | linBytes[11] << 1; // bytes, not shorts
  std::vector<unsigned short> linearizationVector;
  linearizationVector.reserve(nLinCurveBytes >> 1);
  for (unsigned int i = 12; i < nLinCurveBytes + 12; i += 2) {
    linearizationVector.push_back(
        static_cast<unsigned short>(linBytes[i]) << 8 | linBytes[i+1]);
  }
  settings.linearization_table = linearizationVector;

  /*
  settings.split =
      (static_cast<unsigned short>(linBytes[12 + nLinCurveBytes]) << 8)
      | linBytes[13 + nLinCurveBytes];
      */
  settings.split = 0;

  const int offset = getLong(exifData, "Exif.SubImage2.StripOffsets");

  std::filebuf fb;
  fb.open(argv[1], std::ios::in | std::ios::binary);
  fb.pubseekoff(offset, std::ios::beg);

  ImageReader reader;

  std::auto_ptr<Image> imagePtr(reader.readImage(fb, settings));
  Image& image(*imagePtr);

  Interpolator interpolator(Interpolator::INTERPOLATE_AHD);
  interpolator.interpolate(image);

  std::ofstream out(argv[2], std::ios::binary | std::ios::out);

  ImageWriter writer;
  writer.writeImage(image, out, "PPM", 8);

  return 0;
}
