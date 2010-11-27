#include "refinery/input.h"

#include <algorithm>

namespace refinery {

InputStream::~InputStream() {}

/**
 * Reads up to n bytes into buffer.
 *
 * Returns the number of bytes. 0 means the entire stream has been read (or 0
 * bytes were requested).
 */
std::streamsize InputStream::read(char* buffer, std::streamsize n)
{
  return this->doRead(buffer, n);
}

std::streampos InputStream::seek(std::streamoff offset, std::ios::seekdir dir)
{
  return this->doSeek(offset, dir);
}

std::streampos InputStream::tell()
{
  return this->seek(0, std::ios::cur);
}

std::streamsize BufferInputStream::doRead(char* buffer, std::streamsize n)
{
  std::streampos end = mPos + n;

  if (end > mSize) {
    n = mSize - mPos;
    end = mSize;
  }

  std::copy(mBuffer + mPos, mBuffer + end, buffer);
  mPos = end;

  return n;
}

std::streampos BufferInputStream::doSeek(
    std::streamoff offset, std::ios::seekdir dir)
{
  switch (dir)
  {
    case std::ios::beg:
      mPos = offset;
      break;
    case std::ios::cur:
      mPos += offset;
      break;
    case std::ios::end:
    default:
      mPos = mSize + offset;
      break;
  }

  if (mPos < 0) mPos = 0;
  if (mPos > mSize) mPos = mSize;

  return mPos;
}

FileInputStream::FileInputStream(const std::string& filename)
  : mFilename(filename)
{
  std::auto_ptr<std::filebuf> buf(new std::filebuf());
  buf->open(mFilename.data(), std::ios_base::in | std::ios_base::binary);
  if (buf->is_open()) {
    mStreambuf = buf;
  }
}

std::streamsize FileInputStream::doRead(char* buffer, std::streamsize n)
{
  return mStreambuf->sgetn(buffer, n);
}

std::streampos FileInputStream::doSeek(
    std::streamoff offset, std::ios::seekdir dir)
{
  return mStreambuf->pubseekoff(offset, dir);
}

HuffmanDecodedInputStream::HuffmanDecodedInputStream(InputStream& inputStream)
  : mInputStream(inputStream)
{
}

std::streamsize HuffmanDecodedInputStream::doRead(
    char* buffer, std::streamsize n)
{
  // FIXME: implement!
  return 0;
}

} /* namespace refinery */
