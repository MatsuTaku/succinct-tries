#include <strie/louds.hpp>
#include <strie/dfuds.hpp>
#include <strie/centroid_path_tree.hpp>

template<typename D>
void test_string_collection() {
  std::vector<std::string> keys = {
      "aa",
      "ab",
      "bc",
      "ca",
      "cb",
      "cc",
  };
  D d(keys.begin(), keys.end());
  d.print_for_debug();

  for (auto& key : keys) {
    if (!d.contains(key)) {
      std::cerr << key << " is not contained!" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

int main() {
//  test_string_collection<strie::Louds>();
//  test_string_collection<strie::DfudsTrie>();
//  test_string_collection<strie::CentroidPathTreeRaw>();
  test_string_collection<strie::CentroidPathTree>();
  std::cout << "OK" << std::endl;
}