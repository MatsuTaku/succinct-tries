#ifndef SUCCINCT_TRIES__BP_HPP_
#define SUCCINCT_TRIES__BP_HPP_

#include "findclose.hpp"

#include <cstdint>
#include <cassert>
#include <vector>
#include <stack>

#include <sdsl/bit_vectors.hpp>

namespace strie {

template<unsigned LEVEL = 0>
class BpSupport;

template<unsigned LEVEL>
class BpSupport {
 public:
  using index_type = size_t;
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  static constexpr unsigned W = 16;
  using bv_type = sdsl::bit_vector;
  using rankL_type = sdsl::rank_support_v<kLbra, 1>;
  using r_type = sdsl::rrr_vector<>;
  using sub_type = BpSupport<LEVEL+1>;
 private:
  bv_type* bvp_;
  rankL_type* rankLp_;
  r_type r_;
  r_type::rank_1_type r_rank1_;
  r_type::select_1_type r_select1_;
  bv_type pd_;
  rankL_type pd_rank_;
  sub_type sub_;

 public:
  BpSupport() = default;
  explicit BpSupport(bv_type* bvp, rankL_type* rankp) : BpSupport() {
    init_support(bvp, rankp);
  }

  void init_support(bv_type* bvp, rankL_type* ranklp);

  index_type depth(index_type i) const { return (*rankLp_)(i) * 2 - i; }

  index_type findclose(index_type i) const;

  void print_for_debug() const {
    for (int i = 0; i < r_.size(); i++)
      std::cout << r_[i];
    std::cout << std::endl;
    for (int i = 0; i < pd_.size(); i++)
      std::cout << pd_[i];
    std::cout << std::endl;
    sub_.print_for_debug();
  }
};

template<>
class BpSupport<2> {
 public:
  using index_type = size_t;
  static constexpr bool kLbra = 1;
  static constexpr bool kRbra = 0;
  using bv_type = sdsl::bit_vector;
  using rankL_type = sdsl::rank_support_v<kLbra, 1>;
 private:
  sdsl::int_vector<> fc_;

 public:
  BpSupport() = default;
  explicit BpSupport(bv_type* bvp, rankL_type* rankp) : BpSupport() {
    init_support(bvp, rankp);
  }

  void init_support(bv_type* bvp, rankL_type* ranklp);

  index_type findclose(index_type i) const { return fc_[i]; }
  index_type findopen(index_type i) const { return fc_[i]; }

  void print_for_debug() const {
    for (int i = 0; i < fc_.size(); i++)
      std::cout << fc_[i] << ' ';
    std::cout << std::endl;
  }
};

template<unsigned LEVEL>
void BpSupport<LEVEL>::init_support(bv_type* bvp, rankL_type* ranklp) {
  bvp_ = bvp;
  rankLp_ = ranklp;
  index_type n = bvp_->size();
  std::stack<index_type> os;
  // build R, pioneer group
  {
    std::vector<index_type> p(n);
    for (size_t i = 0; i < n; i++) {
      if ((*bvp_)[i] == kLbra) {
        os.push(i);
      } else {
        int j = os.top();
        p[j] = i;
        p[i] = j;
        os.pop();
      }
    }
    assert(os.empty());
    auto _r = sdsl::bit_vector(n);
    os.push(0);
    _r[0] = _r[n-1] = 1;
    auto is_long = [&p](size_t i) { return i/W != p[i]/W; };
    for (size_t i = 1; i+1 < n; i++) {
      if (!is_long(i)) continue;
      if ((*bvp_)[i] == kLbra) {
        if (p[i]/W != p[os.top()]/W) {
          _r[i] = _r[p[i]] = 1;
        }
        os.push(i);
      } else {
        os.pop();
      }
    }
    os.pop();
    assert(os.empty());
    os.push(n-1);
    for (long long i = n-2; i > 0; i--) {
      if (!is_long(i)) continue;
      if ((*bvp_)[i] == kRbra) {
        if (p[i]/W != p[os.top()]/W)
          _r[i] = _r[p[i]] = 1;
        os.push(i);
      } else {
        os.pop();
      }
    }
    os.pop();
    r_ = sdsl::rrr_vector<>(_r);
  }
  sdsl::util::init_support(r_rank1_, &r_);
  sdsl::util::init_support(r_select1_, &r_);

  // build find-close
  assert(os.empty());
  auto num_pioneers = r_rank1_(r_.size());
  pd_ = bv_type(num_pioneers);
  for (size_t i = 0; i < num_pioneers; i++) {
    auto j = r_select1_(i+1);
    pd_[i] = (*bvp_)[j];
  }
  sdsl::util::init_support(pd_rank_, &pd_);

  sub_.init_support(&pd_, &pd_rank_);
}

void BpSupport<2>::init_support(bv_type* bvp, [[maybe_unused]] rankL_type* rankp) {
  index_type n = bvp->size();
  std::stack<index_type> os;
  fc_ = sdsl::int_vector<>(n);
  for (size_t i = 0; i < n; i++) {
    if ((*bvp)[i] == kLbra) {
      os.push(i);
    } else {
      fc_[os.top()] = i;
      fc_[i] = os.top();
      os.pop();
    }
  }
  sdsl::util::bit_compress(fc_);
}

template<unsigned LEVEL>
typename BpSupport<LEVEL>::index_type
BpSupport<LEVEL>::findclose(index_type i) const {
  assert((*bvp_)[i] == kLbra);
  uint64_t w = *(bvp_->data() + (i / 64));
  assert(W == 16);
  index_type in = findclose16(~(w >> (i % 64)) & 0xFFFF);
  if (i % 16 + in < 16 and i + in < bvp_->size()) // findclose is in same block of i
    return i + in;

  auto pred_sub = r_rank1_(i + 1) - 1;
  auto pred = r_select1_(pred_sub + 1);
  auto q = r_select1_(sub_.findclose(pred_sub) + 1);
  if (i == pred)
    return q;
  assert(q%16 > 0);
  auto d = depth(i) - depth(q);
  auto lead = finddepthr16((w >> (q%64/16*16)) << (15-q%16) & 0xFFFF, d);
  return q - 1 - lead;
}


} // namespace strie

#endif //SUCCINCT_TRIES__BP_HPP_
