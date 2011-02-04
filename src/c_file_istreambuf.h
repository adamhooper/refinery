#include <cstdio>

namespace refinery {

template<typename T, typename traits = std::char_traits<T> >
class basic_c_file_istreambuf : public std::streambuf
{
  typedef typename traits::int_type int_type;

  FILE* mFile;
  T* mBuffer;

public:
  basic_c_file_istreambuf(FILE* f) : mFile(f) {
    mBuffer = new T[BUFSIZ];
    T* end = &mBuffer[BUFSIZ];
    this->setg(mBuffer, end, end);
  }

  ~basic_c_file_istreambuf() {
    delete[] mBuffer;
  }

protected:

  /* virtual */ int_type underflow() {
    int_type ret = std::fread(mBuffer, 1, BUFSIZ, mFile);

    if (ret > 0) {
      this->setg(eback(), eback(), egptr());
      return traits::to_int_type(eback()[0]);
    } else {
      this->setg(eback(), egptr(), egptr());
      return traits::eof();
    }
  }

  /* virtual */ std::streampos seekoff(
      std::streamoff off, std::ios::seekdir way,
      std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
  {
    if (!(which & std::ios::in)) {
      return traits::eof();
    }

    this->setg(eback(), egptr(), egptr());

    int whence = SEEK_END;
    if (way == std::ios_base::beg) {
      whence = SEEK_SET;
    } else if (way == std::ios_base::cur) {
      whence = SEEK_CUR;
    }

    if (!std::fseek(mFile, off, whence)) {
      return std::ftell(mFile);
    } else {
      return traits::eof();
    }
  }

  /* virtual */ std::streampos seekpos(
      std::streampos pos,
      std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
  {
    return seekoff(pos, std::ios_base::beg, which);
  }
};

typedef basic_c_file_istreambuf<char> c_file_istreambuf;

} // namespace refinery
