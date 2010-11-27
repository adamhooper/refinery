#ifndef _REFINERY_INPUT_H
#define _REFINERY_INPUT_H

namespace refinery {

class OutputStream {
public:
  virtual void write(char* buffer, size_t n);
};

class FileOutputStream : OutputStream {
public:
  virtual void write(char* buffer, size_t n);
};

};

#endif /* REFINERY_INPUT_H */
