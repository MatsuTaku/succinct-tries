#include <strie/bp.hpp>

#include <iostream>
#include <vector>

#include <sdsl/bit_vectors.hpp>

int main() {
  std::string bv = "11110110001001110000";
  std::stack<int> os;
  int n = bv.size();
  std::vector<int> p(n);
  for (int i = 0; i < n; i++) {
    if (bv[i] == '1') {
      os.push(i);
    } else {
      p[os.top()] = i;
      p[i] = os.top();
      os.pop();
    }
  }
  sdsl::bit_vector v(n);
  for (int i = 0; i < n; i++) v[i] = bv[i] == '1';
  sdsl::rank_support_v<> rank(&v);
  strie::BpSupport<> bp(&v, &rank);
//  bp.print_for_debug();
  std::vector<int> fc(n);
  for (int i = 0; i < n; i++) {
    fc[i] = bv[i] == '1' ? bp.findclose(i) : p[i];
  }

//  std::cout << bv << std::endl;
//  for (int i = 0; i < n; i++) {
//    std::cout << p[i] << ' ';
//  }
//  std::cout << std::endl;
//  for (int i = 0; i < n; i++) {
//    std::cout << fc[i] << ' ';
//  }
//  std::cout << std::endl;

  for (int i = 0; i < n; i++) {
    if (bv[i] == '1' and fc[i] != p[i]) {
      std::cout << i << ' ' << fc[i] << " != " << p[i] << std::endl;
      exit(EXIT_FAILURE);
      assert(false);
    }
  }
  std::cout << "OK" << std::endl;
}
