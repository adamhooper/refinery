#ifndef _REFINERY_EXIF_H
#define _REFINERY_EXIF_H

#include <string>
#include <vector>

namespace refinery {

class ExifData {
public:
  typedef unsigned char byte;

  virtual bool hasKey(const char* key) const = 0;
  virtual std::string getString(const char* key) const = 0;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const = 0;
  virtual int getInt(const char* key) const = 0;
  virtual float getFloat(const char* key) const = 0;

  virtual void setString(const char* key, const std::string& s) = 0;
};

class InMemoryExifData : public ExifData {
  class Impl;
  Impl* impl;

public:
  InMemoryExifData();
  ~InMemoryExifData();

  virtual bool hasKey(const char* key) const;
  virtual std::string getString(const char* key) const;
  virtual void getBytes(const char* key, std::vector<byte>& outBytes) const;
  virtual int getInt(const char* key) const;
  virtual float getFloat(const char* key) const;

  virtual void setString(const char* key, const std::string& s);
};

} // namespace refinery

#endif /* _REFINERY_EXIF_H */
