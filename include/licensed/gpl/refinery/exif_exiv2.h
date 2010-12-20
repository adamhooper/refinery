#ifndef _REFINERY_EXIF_EXIV2_H
#define _REFINERY_EXIF_EXIV2_H

#include <refinery/exif.h>

namespace Exiv2 {
  class ExifData;
}

namespace refinery {

class Exiv2ExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  Exiv2ExifData(Exiv2::ExifData& exiv2ExifData);
  ~Exiv2ExifData();

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;

  virtual void setString(const char* key, const std::string& s);
};

}

#endif /* _REFINERY_EXIF_EXIV2_H */
