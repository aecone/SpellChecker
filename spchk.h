#ifndef SPCHK_H
#define SPCHK_H

#include <stdbool.h>

#define BUFFER_SIZE 4096
#define ALPHABET_SIZE (26*2) // Expand the alphabet size to handle both lowercase and uppercase letters

// typedef struct TrieNode {
//     struct TrieNode* children[ALPHABET_SIZE];
//     bool isEndOfWord;
// } TrieNode;

// // Trie-related operations
// TrieNode* getNewTrieNode(void);
// void insertWord(TrieNode* root, const char* word);
// bool searchWord(TrieNode* root, const char* word);
// void freeTrie(TrieNode* root);

// // Utility functions
// bool isAllUpperCase(const char *word);
// bool hasOnlyFirstLetterCapitalized(const char *word);
// int charToIndex(char c);

// // Processing functions
// bool checkAndReportWord(char *word, TrieNode *root, const char* filePath, long lineNo, int columnNo);
// bool processLine(char *line, TrieNode *root, const char* filePath, long lineNo);
// bool processFile(const char *filePath, TrieNode *root);
// bool traverseDirectory(const char *dirPath, TrieNode *root);
// bool isValidWordChar(char c, bool start);

#endif // SPCHK_H
