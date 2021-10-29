#ifndef SUCCINCT_TRIES__LOUDS_HPP_
#define SUCCINCT_TRIES__LOUDS_HPP_

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
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <sdsl/util.hpp>

namespace strie {

class Louds {
 public:
  using value_type = std::string;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  static constexpr char_type kDelim = '\0';
  static constexpr char_type kRootLabel = '^'; // for visualization
  using index_type = size_t;
 private:
  sdsl::bit_vector bv_;
  sdsl::rank_support_v<1, 1> rank1_;
  sdsl::select_support_mcl<0, 1> select0_;
  sdsl::bit_vector leaf_;
  sdsl::rank_support_v<1, 1> rank_leaf_;
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

  index_type _rank0(index_type i) const {
    return i - rank1_(i);
  }

  index_type _child(index_type i) const {
    return select0_(rank1_(i) + 1);
  }

 public:
  Louds() : size_(0) {}
  template<typename It>
  Louds(It begin, It end) : Louds() {
    _build(begin, end);
  }
  Louds(std::initializer_list<value_type> list) : Louds(list.begin(), list.end()) {}

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
//    std::cout << "rank0" << std::endl;
//    for (int i = 0; i < bv_.size(); i++)
//      std::cout << rank0_(i);
//    std::cout << std::endl;
//    std::cout << "rank1" << std::endl;
//    for (int i = 0; i < bv_.size(); i++)
//      std::cout << rank1_(i);
//    std::cout << std::endl;
//    std::cout << "child" << std::endl;
//    for (int i = 0; i < bv_.size(); i++) if (bv_[i] == 1)
//      std::cout << _child(i) << ' ';
//    std::cout << std::endl;
    for (int i = 0; i < chars_.size(); i++)
      std::cout << chars_[i];
    std::cout << std::endl;
    for (int i = 0; i < leaf_.size(); i++)
      std::cout << leaf_[i];
    std::cout << std::endl;
  }

};

template<typename It>
void Louds::_build(It begin, It end) {
  using traits = std::iterator_traits<It>;
  static_assert(std::is_convertible_v<typename traits::value_type, value_type>);
  static_assert(std::is_base_of_v<std::forward_iterator_tag, typename traits::iterator_category>);

  _check_valid_input(begin, end);

  bv_.resize(1);
  bv_[0] = 1;
  chars_.resize(1);
  chars_[0] = kRootLabel;
  std::queue<std::tuple<It, It, size_t>> qs;
  qs.emplace(begin, end, 0);
  std::vector<char_type> cs;
  while (!qs.empty()) {
    auto [b,e,d] = qs.front(); qs.pop();
    assert(b != e);
    cs.clear();
    bool has_leaf = false;
    auto it = b;
    if ((*b).size() == d) {
      has_leaf = true;
      ++it;
    }
    while (it != e) {
      auto f = it++;
      assert(f->length() > d);
      auto c = (*f)[d];
      cs.push_back(c);
      while (it != e and (*it)[d] == c)
        ++it;
      qs.emplace(f, it, d+1);
    }
    size_t t = bv_.size();
    bv_.resize(t + 1 + cs.size());
    bv_[t] = 0;
    chars_.resize(t + 1 + cs.size());
    chars_[t] = kDelim;
    for (int i = 0; i < cs.size(); i++) {
      bv_[t + 1 + i] = 1;
      chars_[t + 1 + i] = cs[i];
    }
    leaf_.resize(leaf_.size()+1);
    leaf_[leaf_.size()-1] = has_leaf;
  }
  sdsl::util::init_support(rank1_, &bv_);
  sdsl::util::init_support(select0_, &bv_);
  sdsl::util::init_support(rank_leaf_, &leaf_);
  size_ = rank_leaf_(leaf_.size());
}

template<typename STR>
bool Louds::contains(STR&& key, index_type len) const {
  index_type i, idx = 1;
  for (i = 0; i < len; i++) {
    idx++;
    char_type c;
    while ((c = chars_[idx]) != kDelim and c < key[i])
      ++idx;
    if (c != key[i])
      return false;
    idx = _child(idx);
  }
  return leaf_[_rank0(idx)];
}

} // namespace strie

#endif //SUCCINCT_TRIES__LOUDS_HPP_
