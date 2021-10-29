#ifndef SUCCINCT_TRIES__DFUDS_HPP_
#define SUCCINCT_TRIES__DFUDS_HPP_

#include "bp.hpp"

#include <string>
#include <cstring>
#include <string_view>
#include <iterator>
#include <type_traits>
#include <cassert>
#include <exception>
#include <vector>
#include <queue>
#include <tuple>
#include <initializer_list>
#include <iostream>

#include <sdsl/int_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <sdsl/util.hpp>

namespace strie {

class Dfuds {
 public:
  using value_type = std::string;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  static constexpr char_type kDelim = '\0';
  static constexpr char_type kRootLabel = '^'; // for visualization
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  using index_type = size_t;
 private:
  sdsl::bit_vector bv_;
  sdsl::rank_support_v<kLbra, 1> rankL_;
  sdsl::select_support_mcl<kRbra, 1> selectR_;
  sdsl::bit_vector leaf_;
  sdsl::rank_support_v<1, 1> leaf_rank_;
  std::vector<char_type> chars_;
  size_t size_;
  BpSupport<> bp_;

 private:
  template<typename It>
  void _build(It begin, It end);

  template<typename It>
  void _check_valid_input(It begin, It end) const {
    // Check input be sorted.
    if (begin == end)
      return;
    for (auto pre = begin, it = std::next(begin); it != end; ++pre, ++it)
      if (not (*pre < *it))
        throw std::domain_error("Input string collection is not sorted.");
  }

  index_type _rankR(index_type i) const {
    return i - rankL_(i);
  }

  index_type _degree(index_type x) const {
    return selectR_(_rankR(x) + 1) - x;
  }

  index_type _child(index_type x, index_type i) const {
    assert(bv_[x + i] == kLbra);
    return bp_.findclose(x + _degree(x) - 1 - i) + 1;
  }

 public:
  Dfuds() : size_(0) {}
  template<typename It>
  Dfuds(It begin, It end) : Dfuds() {
    _build(begin, end);
  }
  Dfuds(std::initializer_list<value_type> list) : Dfuds(list.begin(), list.end()) {}

  size_t size() const { return size_; }
  bool empty() const { return size() == 0; }

  template<typename STR>
  bool contains(STR&& key, index_type len) const;
  bool contains(const std::string& key) const { return contains(key, key.length()); }
  bool contains(std::string_view key) const { return contains(key, key.length()); }
  bool contains(const char* key) const { return contains(key, std::strlen(key)); }

 public:
  void print_for_debug() const {
    for (int i = 0; i < bv_.size(); i++)
      std::cout << bv_[i];
    std::cout << std::endl;
    for (int i = 0; i < chars_.size(); i++)
      std::cout << chars_[i];
    std::cout << std::endl;
    for (int i = 0; i < leaf_.size(); i++)
      std::cout << leaf_[i];
    std::cout << std::endl;
  }

};

template<typename It>
void Dfuds::_build(It begin, It end) {
  using traits = std::iterator_traits<It>;
  static_assert(std::is_convertible_v<typename traits::value_type, value_type>);
  static_assert(std::is_base_of_v<std::forward_iterator_tag, typename traits::iterator_category>);

  _check_valid_input(begin, end);

  bv_.resize(1);
  bv_[0] = kLbra;
  chars_.resize(1);
  chars_[0] = kRootLabel;

  std::vector<char_type> cs;
  auto dfs = [&](auto& f, It b, It e, size_t d) -> void {
    assert(b != e);
    auto it = b;
    bool has_leaf = false;
    if ((*it).length() == d) {
      has_leaf = true;
      ++it;
    }
    cs.clear();
    std::vector<std::tuple<It,It>> ns;
    while (it != e) {
      auto t = it;
      auto c = (*t)[d];
      cs.push_back(c);
      ++it;
      while (it != e and (*it)[d] == c)
        ++it;
      ns.emplace_back(t, it);
    }
    size_t t = bv_.size();
    bv_.resize(t + cs.size() + 1);
    chars_.resize(t + cs.size() + 1);
    for (size_t i = 0; i < cs.size(); i++) {
      bv_[t + i] = kLbra;
      chars_[t + i] = cs[i];
    }
    bv_[t + cs.size()] = kRbra;
    chars_[t + cs.size()] = kDelim;
    leaf_.resize(leaf_.size()+1);
    leaf_[leaf_.size()-1] = has_leaf;
    for (auto [b,e] : ns)
      f(f, b, e, d+1);
  };
  dfs(dfs, begin, end, 0);

  sdsl::util::init_support(rankL_, &bv_);
  sdsl::util::init_support(selectR_, &bv_);
  sdsl::util::init_support(leaf_rank_, &leaf_);

  bp_.init_support(&bv_, &rankL_);
}

template<typename STR>
bool Dfuds::contains(STR&& key, index_type len) const {
  index_type idx = 1;
  for (index_type k = 0; k < len; k++) {
    index_type i = 0;
    while (bv_[idx + i] == kLbra and chars_[idx + i] < key[k])
      i++;
    if (chars_[idx + i] != key[k])
      return false;
    idx = _child(idx, i);
  }
  return leaf_[_rankR(idx)];
}

} // namespace strie

#endif //SUCCINCT_TRIES__DFUDS_HPP_
