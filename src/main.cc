#include <algorithm>
#include <atomic>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "BS_thread_pool.hpp"

#define WORD_LEN 5
#define NUM_WORDS 5
#define ALPHABET_LEN 26
#define TARGET_CHARS_COUNT 4

std::atomic_uint64_t num_comparisons = 0;
std::mutex output_lock;

struct word_obj {
	int num_target_chars;
	std::string word;
	std::bitset<ALPHABET_LEN> bcharset;
	std::vector<size_t> neighbors;
};

std::unordered_map<char, int>
init_char_frequency()
{
	std::unordered_map<char, int> char_frequency;

	for (char i = 'a'; i <= 'z'; i++) {
		char_frequency[i] = 0;
	}

	return char_frequency;
}

template <size_t N>
std::bitset<N>
charset_as_bitset(std::string word)
{
	std::bitset<N> bcharset;
	for (auto c: word) {
		bcharset[c - 'a'] = true;
	}

	return bcharset;
}

bool
sort_by_second(const std::pair<char, int> &a, const std::pair<char, int> &b)
{
	return a.second < b.second;
}

bool
sort_word_list_by_target_chars(const word_obj &a, const word_obj &b)
{
	return a.num_target_chars > b.num_target_chars;
}

void
find_more_connections(std::vector<word_obj> &word_list, std::vector<std::vector<size_t>> &output_list,
                      std::vector<size_t> &acceptable_neighbors, std::vector<size_t> &prev_indices,
                      int problem_letter_count, int depth = 1)
{
	bool needs_more_letters = problem_letter_count < TARGET_CHARS_COUNT - (ALPHABET_LEN - (NUM_WORDS * WORD_LEN));

	for (auto neighbor: acceptable_neighbors) {
		if (needs_more_letters && !word_list[neighbor].num_target_chars) {
			break;
		}

		// found a result, add it to the output list
		std::vector<size_t> prev_indices_copy = prev_indices;
		if (depth == NUM_WORDS - 1) {
			prev_indices_copy.push_back(neighbor);
			output_list.push_back(prev_indices_copy);
		} else {
			num_comparisons++;

			std::vector<size_t> neighbor_intersection;

			// neighbor indices are already sorted
			std::set_intersection(acceptable_neighbors.begin(), acceptable_neighbors.end(),
			                      word_list[neighbor].neighbors.begin(),
			                      word_list[neighbor].neighbors.end(),
			                      std::back_inserter(neighbor_intersection));

			if (neighbor_intersection.size()) {
				prev_indices_copy.push_back(neighbor);
				find_more_connections(word_list, output_list, neighbor_intersection, prev_indices_copy,
				                      problem_letter_count + word_list[neighbor].num_target_chars,
				                      depth + 1);
			}
		}
	}
}

int
main()
{
	std::size_t word_len = WORD_LEN;
	std::vector<std::vector<size_t>> output_list;

	std::unordered_map<std::bitset<ALPHABET_LEN>, std::vector<std::string>> anagrams;
	std::vector<word_obj> word_list;

	std::unordered_map<char, int> char_frequency = init_char_frequency();

	std::ifstream wordsf("./words/words_alpha.txt");

	std::string word;
	while (getline(wordsf, word)) {
		if (word.length() == word_len) {
			std::bitset<ALPHABET_LEN> bcharset = charset_as_bitset<ALPHABET_LEN>(word);

			if (bcharset.count() == word_len) {
				word_list.push_back({0, word, bcharset, {}});
				anagrams[bcharset].push_back(word);

				for (auto c: word) {
					char_frequency[c]++;
				}
			}
		}
	};

	// sort char frequency ascending
	std::vector<std::pair<char, int>> sorted_char_frequency(char_frequency.begin(), char_frequency.end());
	std::sort(sorted_char_frequency.begin(), sorted_char_frequency.end(), sort_by_second);
	std::string target_chars;
	std::transform(sorted_char_frequency.begin(), sorted_char_frequency.begin() + TARGET_CHARS_COUNT,
	               std::back_inserter(target_chars), [](auto const &pair) { return pair.first; });
	std::bitset<ALPHABET_LEN> btarget_chars = charset_as_bitset<ALPHABET_LEN>(target_chars);

	for (auto &word: word_list) {
		word.num_target_chars = (word.bcharset & btarget_chars).count();
	}

	// sort word list by how many target chars they contain descending
	std::sort(word_list.begin(), word_list.end(), sort_word_list_by_target_chars);

	for (ssize_t i = word_list.size() - 1; i >= 0; --i) {
		for (size_t k = i + 1; k < word_list.size(); ++k) {
			// true if no bits match
			if ((word_list[i].bcharset & word_list[k].bcharset) == 0) {
				// save index of word that has none of the same characters
				word_list[i].neighbors.push_back(k);
			}
		}

		// if the word has any neighbors, keep searching
		if (word_list[i].neighbors.size()) {
			std::vector<size_t> prev_indices = {(size_t) i};
			find_more_connections(word_list, output_list, word_list[i].neighbors, prev_indices,
			                      word_list[i].num_target_chars);
		}
	}
	std::cout << "comparisons: " << num_comparisons << std::endl;

	std::filesystem::create_directories("./output/");
	std::ofstream outputf("./output/output.csv");

	for (auto five_words: output_list) {
		for (auto word: five_words) {
			outputf << word_list[word].word << ", ";
		}

		outputf << '\n';
	}

	return 0;
}
