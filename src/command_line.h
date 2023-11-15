#ifndef GBEMU_COMMAND_LINE_H_
#define GBEMU_COMMAND_LINE_H_

#include <string>

namespace gbemu {

class Options {
 public:
  void Parse(int argc, char* argv[]);
  bool debug() { return debug_; }
  std::string filename() { return filename_; }

 private:
  bool debug_;
  std::string filename_;
};

extern Options options;

}  // namespace gbemu

#endif  // GBEMU_COMMAND_LINE_H_
