#pragma once

#include <algorithm>
#include <string_view>
#include <string>
#include <cstddef>
#include <vector>
#include <utility>
#include <iterator>
#include <meta>

#include <rsl/string_constant>
#include <rsl/string_view>
#include <rsl/span>
#include <rsl/assert>

namespace rsl::cli {
namespace _trie_impl {
template <auto... Transitions>
struct State {
  rsl::string_view prefix;
  int word_index;

  [[clang::always_inline]]
  constexpr int visit(std::string_view str) const {
    // assumes the prefix is never empty
    //? empty prefixes are only allowed at the root note
    if (str.empty() || str[0] != prefix[0] || str.size() < prefix.size()) {
      return 0;
    }

    if (str.size() == prefix.size()) {
      // str would become empty after removing the prefix
      return word_index;
    }

    str.remove_prefix(prefix.size());
    template for (constexpr auto T : {Transitions...}) {
      if (int result = T.visit(str); result) {
        return result;
      }
    }
    return 0;
  }
};

template <std::meta::info Words, auto... Transitions>
struct Root {
  static constexpr auto&& words = [:Words:];

  constexpr static bool matches(std::string_view str) {
    if (str.empty()) {
      return false;
    }
    return find(str) >= 0;
  }

  constexpr static int find(std::string_view str) {
    int result = -1;
    if (str.empty()) {
      return -1;
    }
    template for (constexpr auto T : {Transitions...}) {
      if (int result = T.visit(str); result && str == words[result - 1]) {
        return result - 1;
      }
    }
    return -1;
  }
};

template <auto... Transitions>
constexpr inline Root<Transitions...> make_trie{};

template <string_constant transition, int word_index, auto... Transitions>
constexpr inline auto make_state = State<Transitions...>{
    {transition.data, transition.size},
    word_index
};

struct ParsedState {
  std::string prefix;
  std::vector<ParsedState> transitions;
  rsl::span<rsl::string_view> words;
  int word_index = 0;

  static constexpr ParsedState make(rsl::span<rsl::string_view> words) {
    std::vector<rsl::string_view> sorted_words{};
    sorted_words.assign_range(words);
    std::ranges::sort(sorted_words);

    ParsedState root{"", {}, words};

    for (auto word : sorted_words) {
      ParsedState* node = &root;
      std::size_t i     = 0;

      while (i < word.size()) {
        auto it = std::ranges::find_if(node->transitions, [&](const ParsedState& s) {
          return s.prefix.starts_with(word[i]);
        });

        if (it == node->transitions.end()) {
          int word_idx = std::ranges::distance(words.begin(), std::ranges::find(words, word)) + 1;

          node->transitions.emplace_back(std::string(word.substr(i)),
                                         std::vector<ParsedState>{
                                             {"", {}, {}, word_idx}
          });
          break;
        }

        std::size_t const match_len =
            std::mismatch(it->prefix.begin(), it->prefix.end(), word.begin() + i).first -
            it->prefix.begin();

        if (match_len < it->prefix.size()) {
          ParsedState split_node{it->prefix.substr(match_len), std::move(it->transitions)};
          it->prefix.resize(match_len);
          it->transitions = {std::move(split_node)};
        }

        i += match_len;
        node = &*it;
      }
    }
    return root;
  }

  explicit(false) consteval operator std::meta::info() const {
    std::vector<std::meta::info> states = {};
    int index                           = 0;
    for (auto&& state : transitions) {
      if (state.prefix.empty()) {
        index = state.word_index;
        continue;
      }
      states.push_back(state);
    }

    if (!transitions.empty() && prefix.empty()) {
      // prevent aggressive inlining at the root level
      states.insert(states.begin(), reflect_constant(static_cast<std::meta::info>(words)));
      return substitute(^^make_trie, states);
    }

    std::vector args = {std::meta::reflect_constant_string(prefix),
                        std::meta::reflect_constant(index)};
    for (auto state : states) {
      args.push_back(state);
    }
    return substitute(^^make_state, args);
  }
};

constexpr bool has_duplicates(std::span<rsl::string_view> wordlist) {
  std::ranges::sort(wordlist);
  return std::ranges::adjacent_find(wordlist) != wordlist.end();
}
}  // namespace _trie_impl

struct Trie {
  bool (*matches)(std::string_view str) = nullptr;
  int (*find)(std::string_view str)     = nullptr;

  explicit consteval Trie(std::vector<rsl::string_view> words) {
    constexpr_assert(!words.empty(), "An empty word list is not allowed.");
    constexpr_assert(!_trie_impl::has_duplicates(words), "Duplicates in the word list are not allowed.");

    (this->*extract<void (Trie::*)()>(
                substitute(^^assign_handlers, {_trie_impl::ParsedState::make(words)})))();
  }

private:
  template <auto Parser>
  constexpr void assign_handlers() {
    matches = &Parser.matches;
    find    = &Parser.find;
  }
};
}  // namespace rsl