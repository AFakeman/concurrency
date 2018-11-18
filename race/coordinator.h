#include <QThread>

#include <forward_list>
#include <iostream>
#include <list>

#include "runner.h"

class Coordinator : public QThread {
  Q_OBJECT;
public:
  Coordinator(int n) : n_(n) {}
  virtual void run() {
    threads_.emplace_front(0);
    Runner *prev = &(threads_.front());
    QObject::connect(this, &Coordinator::Start, prev, &Runner::Participate);
    for (int i = 1; i < n_; ++i) {
      threads_.emplace_front(i);
      QObject::connect(prev, &Runner::ThreadDone, &(threads_.front()),
                       &Runner::Participate);
      prev = &(threads_.front());
    }
    QObject::connect(prev, &Runner::ThreadDone, this, &Coordinator::RaceDone);
    emit Start();
    exec();
  }
public slots:
  void RaceDone() {
    for (auto &thread : threads_) {
      thread.wait();
    }
    exit(0);
  }

signals:
  void Start();

private:
  std::forward_list<Runner> threads_;
  const int n_;
};
