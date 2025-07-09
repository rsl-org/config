#include <string>
#include <rsl/test>
#include <rsl/_impl/trie.hpp>

namespace rsl::cli::trie {
[[=rsl::test]]
void matches() {
  auto trie = Trie{
      {"apple", "banana", "cherry"}
  };

  ASSERT(trie.matches("apple"));
  ASSERT(trie.matches("banana"));
  ASSERT(trie.matches("cherry"));

  ASSERT(not trie.matches("pear"));
  ASSERT(not trie.matches(""));
  ASSERT(not trie.matches("APPLE"));
}

[[=rsl::test]]
void find() {
  auto trie = Trie{
      {"apple", "banana", "cherry"}
  };

  ASSERT(trie.find("apple") == 0);
  ASSERT(trie.find("banana") == 1);
  ASSERT(trie.find("cherry") == 2);

  ASSERT(trie.find("pear") == -1);
  ASSERT(trie.find("") == -1);
  ASSERT(trie.find("APPLE") == -1);
}
}  // namespace rsl::cli::trie