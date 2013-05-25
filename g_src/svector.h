//pelican aka sam dennis wrote this
#ifndef SVECTOR_H
#define SVECTOR_H

#include <vector>
#include <memory>

template <class T, class A = std::allocator<T> >
class svector : public std::vector<T, A> {
#ifndef WIN32
        public:
		using std::vector<T, A>::begin;
#endif

#ifdef WIN32
        public:
#endif
                void erase(typename std::vector<T, A>::size_type i) {
                        std::vector<T, A> &vec = *this;
                        vec.erase(begin() + i);
                }
                void insert(typename std::vector<T, A>::size_type i, const T &v) {

                        std::vector<T, A> &vec = *this;
                        vec.insert(begin() + i, v);
                }
};
#endif
