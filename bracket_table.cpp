#include <iostream>
#include <vector>

int main() {
//  {
//    std::vector A(1<<8, std::vector(15, 8));
//    for (int i = 0; i < 1<<8; i++) {
//      int s = 0;
//      for (int j = 0; j < 8; j++) {
//        if ((i&(1<<j))) {
//          s--;
//          if (-6 <= -s and -s <= 8 and A[i][-s+6] == 8)
//            A[i][-s+6] = j;
//        }
//        else
//          s++;
//      }
//    }
//    for (int i = 0; i < 1<<8; i++) {
//      std::cout << "{ ";
//      for (int j = 0; j < 15; j++) {
//        std::cout << A[i][j];
//        if (j+1 < 15) std::cout << ", ";
//      }
//      std::cout << " }," << std::endl;
//    }
//    std::cout << std::endl;
//  }

//  {

  {
    std::vector<int> PC(1<<8);
    for (int i = 0; i < 1<<8; i++) {
      for (int j = 0; j < 8; j++)
        PC[i] += (i&(1<<j)) != 0;
    }
    for (int i = 0; i < 1<<8; i++) {
      std::cout << PC[i] << ", ";
      if (i % 16 == 15)
        std::cout << std::endl;
    }
  }
}