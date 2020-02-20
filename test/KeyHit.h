#ifndef KEYHIT_H
#define KEYHIT_H

#include <termios.h>
#include <unistd.h>

#include <functional>
#include <thread>

typedef std::function<void(char key)> KeyFunc;

class KeyHit {

  KeyFunc m_keyFunc;
  bool m_stop { false };
  bool m_stopped { false };
  struct termios m_term;
  std::thread m_thread;

  void setTerminal();
  void resetTerminal();

  void readLine();
  int setNonblocking(int fd);

public:
  KeyHit();
  ~KeyHit();

  KeyHit(const KeyHit& ) = delete;
  KeyHit(KeyHit&& ) = default;

  KeyHit& operator=(const KeyHit& ) = delete;
  KeyHit& operator=(KeyHit&& ) = default;

  void setKeyReceiver(KeyFunc keyFunc);

  void start();
  void stop();

};


#endif // KEYHIT_H
