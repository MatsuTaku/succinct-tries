#ifndef SUCCINCT_TRIES__CENTROID_PATH_TREE_HPP_
#define SUCCINCT_TRIES__CENTROID_PATH_TREE_HPP_

#include <cassert>
#include <string>
#include <algorithm>
#include <utility>
#include <vector>

namespace strie {

class CentroidPathTreeRaw {
 public:
  using index_type = size_t;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  using value_type = std::string;
 private:
  struct Node {
    std::string l;
    std::map<std::pair<index_type, char_type>, index_type> ch;
    size_t sz;
    Node() = default;
    Node(const std::string& l) : l(l), sz(1) {}
  };
  std::vector<Node> nodes_;
  friend class CentroidPathTree;

 public:
  CentroidPathTreeRaw() {}
  template<typename It>
  CentroidPathTreeRaw(It begin, It end) : CentroidPathTreeRaw() {
    build(begin, end);
  }

 private:
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
  template<typename It>
  void build(It begin, It end) {
    using traits = std::iterator_traits<It>;
    static_assert(std::is_convertible_v<typename traits::value_type, value_type>);
    static_assert(std::is_base_of_v<std::forward_iterator_tag, typename traits::iterator_category>);
    _check_valid_input(begin, end);

    auto construct = [&](auto& f, It b, It e, size_t top, size_t dep) -> index_type {
      assert(b != e);
      if (next(b) == e) {
        auto id = nodes_.size();
        nodes_.emplace_back(b->substr(top));
        return id;
      }
      auto it = b;
      std::vector<std::tuple<size_t, It, It, char_type>> chs;
      if (it->length() == dep) {
        chs.emplace_back(1, it, next(it), kEndLabel);
        ++it;
      }
      assert(it->length() > dep);
      while (it != e) {
        auto t = it;
        auto c = (*t)[dep];
        ++t;
        size_t sz = 1;
        while (t != e and (*t)[dep] == c) {
          ++t;
          ++sz;
        }
        chs.emplace_back(sz, it, t, c);
        it = t;
      }
      std::vector<size_t> I(chs.size());
      std::iota(I.begin(), I.end(), 0ll);
      std::sort(I.begin(), I.end(), [&chs](auto& l, auto& r) {
        return std::get<0>(chs[l]) > std::get<0>(chs[r]);
      });
      auto par = f(f, std::get<1>(chs[I[0]]), std::get<2>(chs[I[0]]), top, dep+1);
      for (size_t i = 1; i < chs.size(); i++) {
        auto [sz,cb,ce,c] = chs[I[i]];
        auto cid = f(f, cb, ce, dep, dep+1);
        nodes_[par].ch[{dep-top, c}] = cid;
        nodes_[par].sz += nodes_[cid].sz;
      }
      return par;
    };
    construct(construct, begin, end, 0, 0);
  }

  template<typename STR>
  bool contains(const STR& key) const {
    index_type id = 0;
    int k, d = 0;
    for (k = 0; k < key.length(); k++) {
      auto& u = nodes_[id];
      if (key[k] != u.l[k-d]) {
        auto nxt = u.ch.find({k-d, key[k]});
        if (nxt == u.ch.end()) {
          return false;
        }
        id = nxt->second;
        d = k;
        assert(nodes_[id].l[0] == key[k]);
      }
    }
    return nodes_[id].l.length() == k-d
        or nodes_[id].ch.find({k-d, kEndLabel}) != nodes_[id].ch.end();
  }

  void print_for_debug() const {}

};


class CentroidPathTree : protected Dfuds {
  using dfuds = Dfuds;
 public:
  using index_type = size_t;
  using char_type = char;
  static constexpr char_type kEndLabel = '\0';
  static constexpr char_type kDelim = '\0';
  using value_type = std::string;
 private:
  std::vector<std::string> labels_;
  sdsl::bit_vector bl_, bs_;
  sdsl::bit_vector::rank_1_type bl_rank1_;
  sdsl::bit_vector::select_1_type bl_select1_;
  std::vector<char_type> cs_;
  sdsl::int_vector<> is_;

  void orchestrate() {
    dfuds::orchestrate();
    sdsl::util::init_support(bl_rank1_, &bl_);
    sdsl::util::init_support(bl_select1_, &bl_);
    sdsl::util::bit_compress(is_);
  }

 public:
  CentroidPathTree() {}
  template<typename It>
  CentroidPathTree(It begin, It end) : CentroidPathTree() {
    build(begin, end);
  }

  void build(const CentroidPathTreeRaw& raw) {
    bv_ = sdsl::bit_vector();
    cs_ = std::vector<char_type>();
    bl_ = sdsl::bit_vector();
    bs_ = sdsl::bit_vector();
    is_ = sdsl::int_vector<>();
    auto expand = [&](size_t additional) {
      auto newsize = dfuds::bv_.size() + additional;
      dfuds::bv_.resize(newsize);
      bl_.resize(newsize);
      bs_.resize(newsize);
      cs_.resize(newsize);
    };
    expand(1);
    bv_[0] = kLbra;
    cs_[0] = kDelim;
    auto dfs = [&](auto& f, index_type id) -> void {
      const auto& node = raw.nodes_[id];
      labels_.push_back(node.l.substr(id == 0 ? 0 : 1));
      auto off = bv_.size();
      expand(node.ch.size() + 1);
      size_t k = 0;
      if (!node.ch.empty()) {
        index_type pidx = -1;
        for (auto& [key, nxt] : node.ch) {
          auto& [idx,c] = key;
          dfuds::bv_[off+k] = kLbra;
          if (idx != pidx) {
            pidx = idx;
            is_.resize(is_.size()+1);
            assert(id == 0 or idx > 0);
            is_[is_.size()-1] = idx - (id == 0 ? 0 : 1);
            if (k > 0)
              bl_[off+k-1] = 1;
          }
          bs_[off+k] = node.l[idx] < c;
          cs_[off+k] = c;
          ++k;
        }
        bl_[off+k-1] = 1;
      }
      dfuds::bv_[off+k] = kRbra;
      cs_[off+k] = kDelim;

      for (auto& [key, nxt] : node.ch)
        f(f, nxt);
    };
    dfs(dfs, 0);

    orchestrate();

  }
  template<typename It>
  void build(It begin, It end) {
    build(CentroidPathTreeRaw(begin, end));
  }

  template<typename STR>
  bool contains(const STR& key) const {
    index_type idx = 1;
    index_type id = 0;
    int k, d = 0;
    for (k = 0; k < key.length(); k++) {
      if (labels_[id][k-d] != key[k]) {
        index_type r = bl_rank1_(idx), b = 0, f;
        index_type bdeg = bl_rank1_(idx+dfuds::degree(idx))-r;
        while (b < bdeg and (f = is_[r+b]) < k-d) {
          ++b;
        }
        if (b == bdeg or f > k-d)
          return false;
        assert(f == k-d);
        index_type i = b == 0 ? 0 : bl_select1_(r+b)+1 - idx;
        while (cs_[idx+i] != key[k]) {
          if (bl_[idx+i])
            return false;
          ++i;
        }
        d = k+1;
        idx = dfuds::child(idx, i);
        id = dfuds::rankR(idx);
      }
    }
    if (labels_[id].length() == k-d)
      return true;
    index_type i = bl_select1_(bl_rank1_(idx)+k-d)+1;
    while (dfuds::bv_[idx+i] == kLbra and cs_[idx + i] != kEndLabel) {
      if (bl_[idx+i])
        return false;
      i++;
    }
    return true;
  }

  void print_for_debug() const {
    dfuds::print_for_debug();
    for (size_t i = 0; i < bl_.size(); i++)
      std::cout << bl_[i];
    std::cout<<std::endl;
    for (size_t i = 0; i < cs_.size(); i++)
      std::cout << cs_[i];
    std::cout << std::endl;
    for (size_t i = 0; i < is_.size(); i++)
      std::cout << is_[i] << ' ';
    std::cout << std::endl;
    for (auto& k : labels_)
      std::cout << k << std::endl;
  }

};

} // strie

#endif //SUCCINCT_TRIES__CENTROID_PATH_TREE_HPP_
