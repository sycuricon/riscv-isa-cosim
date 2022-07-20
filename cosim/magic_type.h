#ifndef _COSIM_MAGIC_TYPE_H
#define _COSIM_MAGIC_TYPE_H

#include <vector>

class magic_type {
public:
  magic_type(const std::string &name, std::vector<magic_type*> parents={});
  bool is_child_of(magic_type *b);
  const std::string name;
private:
  std::vector<magic_type*> parents;
  std::vector<magic_type*> children;
};

extern magic_type magic_void;
extern magic_type magic_int;
extern magic_type magic_float;
extern magic_type magic_zero;
extern magic_type magic_address;
extern magic_type magic_fuzz_address;

#endif