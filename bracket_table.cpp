#include <iostream>
#include <vector>

int main() {
  std::vector A(1<<8, std::vector(9, 8));
  for (int i = 0; i < 1<<8; i++) {
    int s = 0;
    for (int j = 0; j < 8; j++) {
      if ((i&(1<<j)))
        s--;
      else
        s++;
      if (s <= 0 and A[i][-s] == 8)
        A[i][-s] = j;
    }
  }
  for (int i = 0; i < 1<<8; i++) {
    std::cout << "{ ";
    for (int j = 0; j < 9; j++) {
      std::cout << A[i][j];
      if (j+1 < 9) std::cout << ", ";
    }
    std::cout << " }," << std::endl;
  }
  std::cout << std::endl;
}