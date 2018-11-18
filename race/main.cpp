#include <QCoreApplication>
#include <QObject>

#include <iostream>
#include <forward_list>

#include "coordinator.h"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  if (argc != 2) {
    std::cout << "Usage: main <N>" << std::endl;
    exit(2);
  }
  int thread_count = std::stoi(argv[1]);
  if (thread_count <= 0) {
    std::cout << "<N> should be positive" << std::endl;
  }

  Coordinator coord(thread_count);
  coord.run();
  coord.wait();
  std::cout << "Race is finished" << std::endl;
  return 0;
}
