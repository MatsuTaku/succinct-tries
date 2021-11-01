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
 public:// for visualization
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  using index_type = size_t;
 protected:
  sdsl::bit_vector bv_;
  sdsl::rank_support_v<kLbra, 1> rankL_;
  sdsl::select_support_mcl<kRbra, 1> selectR_;
  BpSupport<> bp_;

  void orchestrate() {
    sdsl::util::init_support(rankL_, &bv_);
    sdsl::util::init_support(selectR_, &bv_);
    bp_.init_support(&bv_, &rankL_);
  }

 public:
  Dfuds() {}

  index_type rankR(index_type i) const {
    return i - rankL_(i);
  }

  index_type degree(index_type x) const {
    return selectR_(rankR(x) + 1) - x;
  }

  index_type child(index_type x, index_type i) const {
    assert(bv_[x + i] == kLbra);
    auto deg = degree(x);
    return bp_.findclose(x + deg - 1 - i) + 1;
  }

 public:
  void print_for_debug() const {
    std::cout << "DFUDS" << std::endl;
    for (int i = 0; i < bv_.size(); i++)
      std::cout << bv_[i];
    std::cout << std::endl;
  }

};


class DfudsTrie : protected Dfuds {
  using dfuds = Dfuds;
 public:
  using value_type = std::string;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  static constexpr char_type kDelim = '\0';
  static constexpr char_type kRootLabel = '^'; // for visualization
 private:
  sdsl::bit_vector leaf_;
  sdsl::rank_support_v<1, 1> leaf_rank_;
  std::vector<char_type> chars_;
  size_t size_;

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

 public:
  DfudsTrie() : Dfuds(), size_(0) {}
  template<typename It>
  DfudsTrie(It begin, It end) : DfudsTrie() {
    _build(begin, end);
  }
  DfudsTrie(std::initializer_list<value_type> list) : DfudsTrie(list.begin(), list.end()) {}

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
void DfudsTrie::_build(It begin, It end) {
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

  orchestrate();
  sdsl::util::init_support(leaf_rank_, &leaf_);

}

template<typename STR>
bool DfudsTrie::contains(STR&& key, index_type len) const {
  index_type idx = 1;
  for (index_type k = 0; k < len; k++) {
    index_type i = 0;
    while (bv_[idx + i] == kLbra and chars_[idx + i] < key[k])
      i++;
    if (chars_[idx + i] != key[k])
      return false;
    idx = dfuds::child(idx, i);
  }
  return leaf_[dfuds::rankR(idx)];
}

} // namespace strie

#endif //SUCCINCT_TRIES__DFUDS_HPP_
