#ifndef SUCCINCT_TRIES__DFUDS_HPP_
#define SUCCINCT_TRIES__DFUDS_HPP_

#include "findclose.hpp"

#include <string>
#include <cstring>
#include <string_view>
#include <iterator>
#include <type_traits>
#include <cassert>
#include <exception>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <initializer_list>
#include <iostream>
#include <x86intrin.h>

#include <sdsl/int_vector.hpp>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/select_support.hpp>
#include <sdsl/util.hpp>

namespace strie {

class BpSupport {
 public:
  using index_type = size_t;
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  using word_type = uint64_t;
  static constexpr unsigned W = 64;
 private:
  sdsl::bit_vector* bvp_;
  sdsl::bit_vector r_;
  sdsl::rank_support_v<1, 1> r_rank1_;
  sdsl::select_support_mcl<1, 1> r_select1_;
  sdsl::int_vector<> fc_;
 public:
  void init_support(sdsl::bit_vector* bvp) {
    bvp_ = bvp;
    index_type n = bvp_->size();
    std::stack<index_type> os;
    // build R, pioneer group
    std::vector<index_type> p(n);
    for (size_t i = 0; i < n; i++) {
      if ((*bvp_)[i] == kLbra) {
        os.push(i);
      } else {
        p[os.top()] = i;
        os.pop();
      }
    }
    assert(os.empty());
    for (long long i = n-1; i >= 0; i--) {
      if ((*bvp_)[i] == kRbra) {
        os.push(i);
      } else {
        p[os.top()] = i;
        os.pop();
      }
    }
    assert(os.empty());
    r_ = sdsl::bit_vector(n);
    os.push(0);
    for (size_t i = 1; i+1 < n; i++) {
      if ((*bvp_)[i] == kLbra) {
        if (p[i]/W != p[os.top()]/W) {
          r_[i] = r_[p[i]] = 1;
        }
        os.push(i);
      } else {
        os.pop();
      }
    }
    os.pop();
    assert(os.empty());
    os.push(n-1);
    for (long long i = n-1; i > 0; i--) {
      if ((*bvp_)[i] == kRbra) {
        if (p[i]/W != p[os.top()]/W)
          r_[i] = r_[p[i]] = 1;
        os.push(i);
      } else {
        os.pop();
      }
    }
    os.pop();

    sdsl::util::init_support(r_rank1_, &r_);
    sdsl::util::init_support(r_select1_, &r_);

    // build find-close
    assert(os.empty());
    auto num_pioneers = r_rank1_(r_.size());
    fc_.resize(num_pioneers);
    for (size_t i = 1; i <= num_pioneers; i++) {
      auto j = r_select1_(i);
      if ((*bvp_)[i] == kLbra) {
        os.push(i);
      } else {
        fc_[os.top()] = i;
        os.pop();
      }
    }
    sdsl::util::bit_compress(fc_);
  }

  index_type findclose(index_type i) const {
    assert((*bvp_)[i] == kLbra);
    index_type r = i % W;
    word_type w = *(bvp_->data() + i / W);
    w = (~w) >> r;
    index_type in = findclose64(w);
    if (in < 64-r) // findclose is in same block of i
      return i + r + in;

    auto pred_sub = r_rank1_(i+1);
    auto pred = r_select1_(pred_sub);
    auto q = r_select1_(fc_[pred_sub] + 1);
    if (i == pred)
      return q;
    // TODO: find min index of target depth.
  }
};

class Dfuds {
 public:
  using value_type = std::string;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  static constexpr char_type kDelim = '\0';
  static constexpr char_type kRootLabel = '^'; // for visualization
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  using word_type = uint64_t;
  static constexpr unsigned W = 64;
  using index_type = size_t;
 private:
  sdsl::bit_vector bv_;
  sdsl::rank_support_v<kLbra, 1> rankL_;
  sdsl::select_support_mcl<kRbra, 1> selectR_;
  sdsl::bit_vector leaf_;
  sdsl::rank_support_v<1, 1> leaf_rank_;
  std::vector<char_type> chars_;
  size_t size_;
  BpSupport bp_;

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

  index_type _findclose(index_type i) const {
    assert(bv_[i] == kLbra);
    return bp_.findclose(i);
  }

  index_type _child(index_type i) const {
    return _findclose(i) + 1;
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
  }

};

template<typename It>
void Dfuds::_build(It begin, It end) {
  using traits = std::iterator_traits<It>;
  static_assert(std::is_convertible_v<typename traits::value_type, value_type>);
  static_assert(std::is_base_of_v<std::forward_iterator_tag, typename traits::iterator_category>);

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
    while (it != e) {
      auto t = it;
      auto c = (*t)[d];
      cs.push_back(c);
      ++it;
      while ((*it)[d] == c)
        ++it;
      f(f, t, it, d+1);
    }
    size_t t = bv_.size();
    bv_.resize(t + cs.size() + 1);
    chars_.resize(t + cs.size() + 1);
    for (size_t i = 0; i < cs.size(); i++) {
//      bv_[t + i] = kLbra;
      chars_[t + i] = cs[i];
    }
    bv_[t + cs.size()] = kRbra;
    chars_[t + cs.size()] = kDelim;
    leaf_.resize(leaf_.size()+1);
    leaf_[leaf_.size()-1] = has_leaf;
  };
  dfs(dfs, begin, end, 0);

  sdsl::util::init_support(rankL_, &bv_);
  sdsl::util::init_support(selectR_, &bv_);
  sdsl::util::init_support(leaf_rank_, &leaf_);

  bp_.init_support(&bv_);
}

template<typename STR>
bool Dfuds::contains(STR&& key, index_type len) const {
  // TODO:
}

} // namespace strie

#endif //SUCCINCT_TRIES__DFUDS_HPP_
