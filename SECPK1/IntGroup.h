#ifndef INTGROUPH
#define INTGROUPH

#include "Int.h"
#include <vector>

class IntGroup {

public:

  IntGroup(int size);
  ~IntGroup();
  void Set(Int *pts);
  void ModInv();

private:

  Int *ints;
  Int *subp;
  int size;

};

#endif // INTGROUPCPUH
