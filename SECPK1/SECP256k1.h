#ifndef SECP256K1H
#define SECP256K1H

#include "Point.h"
#include <string>
#include <vector>

class Secp256K1 {

public:

  Secp256K1();
  ~Secp256K1();
  void  Init();
  Point ComputePublicKey(Int *privKey,bool reduce=true);
  std::vector<Point> ComputePublicKeys(std::vector<Int> &privKeys);
  Point NextKey(Point &key);
  bool  EC(Point &p);

  std::string GetPublicKeyHex(bool compressed, Point &p);
  bool ParsePublicKeyHex(std::string str,Point &p,bool &isCompressed);

  Point Add(Point &p1, Point &p2);
  Point Add2(Point &p1, Point &p2);
  Point AddDirect(Point &p1, Point &p2);
  Point Double(Point &p);
  Point DoubleDirect(Point &p);

  std::vector<Point> AddDirect(std::vector<Point> &p1,std::vector<Point> &p2);

  Point G;                 // Generator
  Int   order;             // Curve order

private:

  uint8_t GetByte(std::string &str,int idx);

  Int GetY(Int x, bool isEven);
  Point GTable[256*32];       // Generator table

};

#endif // SECP256K1H
