#include "louds.hpp"

int main() {
  std::vector<std::string> keys = {
      "aa",
      "ab",
      "bc",
      "ca",
      "cb",
      "cc",
  };
  strie::Louds louds(keys.begin(), keys.end());
//  louds.print_for_debug();

  for (auto& key : keys) {
    if (!louds.contains(key)) {
      std::cerr << key << " is not contained!" << std::endl;
    }
  }
}