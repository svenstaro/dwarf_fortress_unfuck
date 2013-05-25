#ifndef BIMAP_H
#define BIMAP_H

#include <map>

template<typename A, typename B>
struct bimap {
  std::map<A,B> left;
  std::map<B,A> right;

  void insert(A a, B b) {
    left.insert(std::pair<A,B>(a,b));
    right.insert(std::pair<B,A>(b,a));
  }
};

#endif
