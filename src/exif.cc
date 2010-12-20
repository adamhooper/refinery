#include "refinery/exif.h"

#include <boost/any.hpp>
#include <boost/tr1/unordered_map.hpp>

namespace refinery {

class InMemoryExifData::Impl {
  std::tr1::unordered_map<std::string, boost::any> mData;

public:
  bool hasKey(const char* key) const {
    return mData.find(key) != mData.end();
  }

  std::string getString(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<std::string>(value);
  }

  void getBytes(const char* key, std::vector<byte>& outBytes) const {
    typedef const std::vector<byte>& ArrayType;

    const boost::any value(mData.at(key));
    ArrayType bytes(boost::any_cast<ArrayType>(value));
    outBytes = bytes;
  }

  int getInt(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<int>(value);
  }

  int getFloat(const char* key) const {
    boost::any value(mData.at(key));
    return boost::any_cast<float>(value);
  }

  void setString(const char* key, const std::string& s) {
    mData[key] = s;
  }

  void setInt(const char* key, int i) {
    mData[key] = i;
  }

  void setFloat(const char* key, float f) {
    mData[key] = f;
  }

  void setBytes(const char* key, const std::vector<byte>& bytes) {
    mData[key] = bytes;
  }
};

InMemoryExifData::InMemoryExifData()
  : impl(new Impl())
{
}

InMemoryExifData::~InMemoryExifData()
{
  delete impl;
}

bool InMemoryExifData::hasKey(const char* key) const
{
  return impl->hasKey(key);
}

std::string InMemoryExifData::getString(const char* key) const
{
  return impl->getString(key);
}

void InMemoryExifData::getBytes(
    const char* key, std::vector<byte>& outBytes) const
{
  impl->getBytes(key, outBytes);
}

int InMemoryExifData::getInt(const char* key) const
{
  return impl->getInt(key);
}

float InMemoryExifData::getFloat(const char* key) const
{
  return impl->getFloat(key);
}

void InMemoryExifData::setString(const char* key, const std::string& s)
{
  impl->setString(key, s);
}

} // namespace refinery
