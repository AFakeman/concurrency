#include <QThread>

#include <iostream>

class Runner : public QThread {
  Q_OBJECT;

public:
  Runner(int idx) : idx_(idx) {}

public slots:
  void Participate() {
    std::cout << "Thread " << idx_ << " was called" << std::endl;
    emit ThreadDone();
    exit(0);
  }

signals:
  void ThreadDone();

private:
  const int idx_;
};
