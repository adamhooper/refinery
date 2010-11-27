#ifndef _REFINERY_INPUT_H
#define _REFINERY_INPUT_H

#include <fstream>
#include <memory>
#include <string>

namespace refinery {

class InputStream {
public:
  virtual ~InputStream();

  std::streamsize read(char* buffer, std::streamsize n);
  std::streampos seek(std::streamoff offset, std::ios::seekdir dir);
  std::streampos tell();

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n) = 0;
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir) = 0;
};

class BufferInputStream : public InputStream {
  const char* mBuffer;
  std::streampos mPos; /* number of chars already read */
  std::streamsize mSize; /* total number of chars */

public:
  BufferInputStream(const char* buffer, std::streamsize size)
      : mBuffer(buffer), mSize(size) {}

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n);
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir);
};

class FileInputStream : public InputStream {
  std::string mFilename;
  std::auto_ptr<std::streambuf> mStreambuf;

public:
  FileInputStream(const std::string& filename);

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n);
  virtual std::streampos doSeek(std::streamoff offset, std::ios::seekdir dir);
};

class HuffmanDecodedInputStream : public InputStream {
  InputStream& mInputStream;

public:
  HuffmanDecodedInputStream(InputStream& inputStream);

protected:
  virtual std::streamsize doRead(char* buffer, std::streamsize n);
};

};

#endif /* REFINERY_INPUT_H */
