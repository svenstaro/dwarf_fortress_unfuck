#ifndef MAIL_H
#define MAIL_H

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <queue>

template <typename T>
class MBox {
  T val;
  SDL_sem *fill, *empty;
public:
  bool try_read(T &r) { // Attempt to read the mbox. Returns true if read succeeded.
    if (SDL_SemTryWait(fill) == 0) {
      r = val;
      SDL_SemPost(empty);
      return true;
    } else
      return false;
  }
  void read(T &r) {
    SDL_SemWait(fill);
    r = val;
    SDL_SemPost(empty);
  }
  void write(const T &v) {
    SDL_SemWait(empty);
    val = v;
    SDL_SemPost(fill);
  }
  bool try_write(const T &v) { // Returns true if the write succeeded
    if (SDL_SemTryWait(empty) == 0) {
      val = v;
      SDL_SemPost(fill);
      return true;
    } else
      return false;
  }
  MBox(T &v) {
    MBox();
    write(v);
  }
  MBox() {
    fill  = SDL_CreateSemaphore(0);
    empty = SDL_CreateSemaphore(1);
  }
  ~MBox() {
    SDL_DestroySemaphore(fill);
    SDL_DestroySemaphore(empty);
  }
};

template <typename T>
class MVar {
  SDL_sem *s;
public:
  T val;
  void lock() { SDL_SemWait(s); }
  void unlock() { SDL_SemPost(s); }
  MVar() { s = SDL_CreateSemaphore(1); }
  ~MVar() { SDL_DestroySemaphore(s); }
  void write(const T &w) { lock(); val = w; unlock(); }
  void read(T &r) { lock(); r = val; unlock(); }
  T read() { T r; read(r); return r; }
};
  

template<bool start_locked = false>
class Lock {
  SDL_sem *s;
public:
  void lock() { SDL_SemWait(s); }
  void unlock() { SDL_SemPost(s); }
  Lock() { s = SDL_CreateSemaphore(start_locked ? 0 : 1); }
  ~Lock() { SDL_DestroySemaphore(s); }
};


template<typename T>
class Chan {
  MVar<std::queue<T> > vals;
  SDL_sem *fill;
public:
  bool try_read(T &r) {
    if (SDL_SemTryWait(fill) == 0) {
      vals.lock();
      r = vals.val.front();
      vals.val.pop();
      vals.unlock();
      return true;
    } else
      return false;
  }
  void read(T &r) {
    SDL_SemWait(fill);
    vals.lock();
    r = vals.val.front();
    vals.val.pop();
    vals.unlock();
  }
  void write(const T &w) {
    vals.lock();
    vals.val.push(w);
    vals.unlock();
    SDL_SemPost(fill);
  }
  Chan() {
    fill = SDL_CreateSemaphore(0);
  }
  ~Chan() {
    SDL_DestroySemaphore(fill);
  }
};

template<>
class Chan<void> {
  SDL_sem *fill;
public:
  bool try_read() {
    if (SDL_SemTryWait(fill) == 0)
      return true;
    return false;
  }
  void read() {
    SDL_SemWait(fill);
  }
  void write() {
    SDL_SemPost(fill);
  }
  Chan() {
    fill = SDL_CreateSemaphore(0);
  }
  ~Chan() {
    SDL_DestroySemaphore(fill);
  }
};

template<typename L, typename R>
struct Either {
  bool isL;
  union {
    L left;
    R right;
  };
  Either(const L &l) {
    isL = true;
    left = l;
  }
  Either(const R &r) {
    isL = false;
    right = r;
  }
};

#endif
