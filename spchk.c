#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "spchk.h"

#define BUFFER_SIZE 4096
// Expand the alphabet size to handle both lowercase and uppercase letters
#define ALPHABET_SIZE (26*2) 

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    bool isEndOfWord;
} TrieNode;

// Trie-related operations
TrieNode* getNewTrieNode(void);
void insertWord(TrieNode* root, const char* word);
bool searchWord(TrieNode* root, const char* word);
void freeTrie(TrieNode* root);

// Utility functions
bool isAllUpperCase(const char *word);
bool hasOnlyFirstLetterCapitalized(const char *word);
int charToIndex(char c);

// Processing functions
bool checkAndReportWord(char *word, TrieNode *root, const char* filePath, long lineNo, int columnNo);
bool processLine(char *line, TrieNode *root, const char* filePath, long lineNo);
bool processFile(const char *filePath, TrieNode *root);
bool traverseDirectory(const char *dirPath, TrieNode *root);
bool isValidWordChar(char c, bool start);
// Main entry
int main(int argc, char *argv[]);

// SECTION 4 ----------------------------------------------------------------
// Checking whether a word is contained in the dictionary

// Check if word is all capitalized
bool isAllUpperCase(const char *word) {
    while (*word) {
        if (isalpha(*word) && islower(*word)) return false;
        ++word;
    }
    return true;
}

// Check if word has only first letter captilized 
bool hasOnlyFirstLetterCapitalized(const char *word) {
    if (!isupper(word[0])) return false;
    for (int i = 1; word[i]; ++i) {
        if (isupper(word[i])) return false;
    }
    return true;
}

// RULE: hello (OG), Hello (first cap), HELLO (all caps)-> correct 
// RULE: MacDonald (OG), MACDONALD (all caps)-> correct
bool searchWord(TrieNode* root, const char* word) {
    // First check: if the word is all caps. HELLO == hEllO
    if (isAllUpperCase(word)) {
        TrieNode* crawl = root;
        for (int i = 0; word[i]; i++) {
            int index = charToIndex(toupper(word[i])); // Convert to uppercase and then find the index
            if (!crawl->children[index]) return false;
            crawl = crawl->children[index];
        }
        if (crawl != NULL && crawl->isEndOfWord) return true;
    }

    // Second check: if the word has only the first letter capitalized. Hello == hello
    if (hasOnlyFirstLetterCapitalized(word)) {
        TrieNode* crawlFirstCapital = root;
        // Convert the first letter of the word to lowercase for comparison
        char lowerFirstLetter = tolower(word[0]);
        // Get the index corresponding to the lowercase first letter
        int index = charToIndex(lowerFirstLetter);
        if (!crawlFirstCapital->children[index]) {
            return false;
        }
    }

    // Third check: exact match (with any capital letters or no capital letters) Hello == Hello, HEllo == HEllo, hello == hello
    TrieNode* crawlExact = root;
    for (int i = 0; word[i]; i++) {
        int index = charToIndex(word[i]);
        if (!crawlExact->children[index]) {
            // If any character of the word is not found in the trie, return false
            return false;
        }
        crawlExact = crawlExact->children[index];
    }
    // Check if the traversal reaches the end of a word in the trie
    if (crawlExact != NULL && crawlExact->isEndOfWord) return true;

    // If none of the conditions are met, the word is not in the dictionary
    return false;
}

//number all lines (vertically) and each line has columns (horizontally)
//track word col and line number. col # is word's first char
//words are sequences of non-whitespace characters

// Check if a character is a valid word character
//RULE: ignore punctation at end of word
//RULE: ignore ' " ( { [ at start of word
bool isValidWordChar(char c, bool start) {
    if (isalpha(c) || c == '-') return true;
    if (start && (c == '\'' || c == '"' || c == '(' || c == '{' || c == '[')) return false;
    return !start; // If not the start, it's a valid character (handles end punctuation)
}

// Report a word if it's not found in the trie. Returns true if an incorrect word was found, false otherwise
//RULE: every time finds an incorrect word, report the word along with the file, the line, and column number
bool checkAndReportWord(char *word, TrieNode *root, const char* filePath, long lineNo, int columnNo) {
    if (!searchWord(root, word)) {
        printf("%s (%ld:%d): %s\n", filePath, lineNo, columnNo, word);
        return true; // Incorrect word found
    }
    return false; // No incorrect word found
}

// Process a line from a file, checking each word against a trie data structure. Returns true if an incorrect word was found, false otherwise
// RULE: hyphenated words are correct if all component words are correctly spelled
// Returns true if any incorrect word was found in the line, false otherwise
bool processLine(char *line, TrieNode *root, const char* filePath, long lineNo) {
    char word[BUFFER_SIZE];
    int wordIndex = 0;
    int columnNo = 1;
    int startColumn = 0;
    bool foundIncorrectWord = false;

    for (int i = 0; line[i] != '\0'; i++) {
        if (isValidWordChar(line[i], wordIndex == 0)) {
            if (wordIndex == 0) startColumn = columnNo;
            word[wordIndex++] = line[i];
        } else if (wordIndex > 0) {
            word[wordIndex] = '\0';
            
            char *component = strtok(word, "-");
            int componentColumn = startColumn;
            while (component != NULL) {
                if (checkAndReportWord(component, root, filePath, lineNo, componentColumn)) {
                    foundIncorrectWord = true;
                }
                component = strtok(NULL, "-");
                if (component != NULL) componentColumn += strlen(component) + 1;
            }
            wordIndex = 0;
        }
        columnNo++;
    }

    if (wordIndex > 0) {
        word[wordIndex] = '\0';
        if (checkAndReportWord(word, root, filePath, lineNo, startColumn)) {
            foundIncorrectWord = true;
        }
    }

    return foundIncorrectWord;
}


// SECTION 3 ----------------------------------------------------------------
//Reading the file and generating a sequence of position-annotated words

//RULE: Must print error if file can't be opened. 
// Returns true if any incorrect word was found in the file, false otherwise
bool processFile(const char *filePath, TrieNode *root) {
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return true; // Treat inability to open file as an error
    }

    char buf[BUFFER_SIZE];
    int bytes_read;
    char *line = malloc(BUFFER_SIZE * sizeof(char));
    if (!line) {
        perror("Memory allocation failed for line buffer");
        close(fd);
        return true; // Memory allocation failure is treated as an error
    }
    size_t line_capacity = BUFFER_SIZE;
    size_t line_length = 0;
    long lineNo = 1;
    bool foundIncorrectWord = false;

    while ((bytes_read = read(fd, buf, sizeof(buf)-1)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buf[i] == '\n') {
                line[line_length] = '\0';
                if (processLine(line, root, filePath, lineNo++)) {
                    foundIncorrectWord = true;
                }
                line_length = 0;
            } else {
                line[line_length++] = buf[i];
            }
        }
    }
    if (line_length > 0) {
        line[line_length] = '\0';
        if (processLine(line, root, filePath, lineNo)) {
            foundIncorrectWord = true;
        }
    }
    free(line);
    close(fd);

    return foundIncorrectWord;
}



// SECTION 2 ----------------------------------------------------------------
//Finding and opening all the specified files, including directory traversal

//Check spelling in all files end w .txt (in any order)
//Ignore files/dir who begin w . 
// Traverse a directory recursively
bool traverseDirectory(const char *dirPath, TrieNode *root) {
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("Failed to open directory");
        return true; // Indicate an error
    }

    struct dirent *entry;
    char fullPath[1024];
    struct stat entryStat;
    bool foundIncorrectWord = false; // Flag to track if any incorrect words are found

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files and directories

        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
        if (stat(fullPath, &entryStat)) {
            perror("Failed to get file status");
            continue;
        }

        if (S_ISDIR(entryStat.st_mode)) {
            if (traverseDirectory(fullPath, root)) {
                foundIncorrectWord = true;
            }
        } else if (S_ISREG(entryStat.st_mode) && strstr(fullPath, ".txt")) {
            if (processFile(fullPath, root)) {
                foundIncorrectWord = true;
            }
        }
    }

    closedir(dir);
    return foundIncorrectWord; // Return whether any incorrect words were found
}

// SECTION 1 ----------------------------------------------------------------
//Reads dictionary file into trie 

// Create a new trie node
TrieNode* getNewTrieNode(void) {
    TrieNode* pNode = (TrieNode*)malloc(sizeof(TrieNode));
    if (!pNode) {
        fprintf(stderr, "Memory allocation failed for new TrieNode.\n");
        exit(EXIT_FAILURE);
    }
        pNode->isEndOfWord = false;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            pNode->children[i] = NULL;
    }
    return pNode;
}

// Function to convert character to index
int charToIndex(char c) {
    if (isupper(c)) return c - 'A' + 26; // Adjust index for uppercase letters
    else return c - 'a';
}

// Insert a word into the trie
void insertWord(TrieNode* root, const char* word) {
    TrieNode* crawl = root;
    bool isValidWord = true;

    for (int i = 0; word[i] && isValidWord; i++) {
        int index = charToIndex(word[i]);

        if (index < 0 || index >= ALPHABET_SIZE) {
            isValidWord = false;
            break;
        }

        if (!crawl->children[index]) {
            crawl->children[index] = getNewTrieNode();
        }
        crawl = crawl->children[index];
    }

    // Only mark the end of the word if it is valid
    if (isValidWord && crawl != NULL) {
        crawl->isEndOfWord = true;
    }
}


// Free the trie memory recursively
void freeTrie(TrieNode* root) {
    if (root == NULL) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) freeTrie(root->children[i]);
    }
    free(root);
}

// Load the dictionary
void loadDictionary(TrieNode* root, const char* dictionaryPath) {
    int fd = open(dictionaryPath, O_RDONLY);
    if (fd < 0) {
        perror("Unable to open the dictionary file");
        exit(EXIT_FAILURE);
    }

    char buf[BUFFER_SIZE];
    int bytes_read;
    int word_index = 0;
    char word[BUFFER_SIZE] = {0};

    while ((bytes_read = read(fd, buf, sizeof(buf)-1)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buf[i] == '\n' || buf[i] == '\r') {
                if (word_index > 0) { // We have a complete word to process
                    word[word_index] = '\0'; // Null-terminate the word
                    insertWord(root, word);
                    word_index = 0; // Reset for the next word
                }
            } else {
                word[word_index++] = buf[i];
            }
        }
    }
    if (word_index > 0) { // Insert last word if exists
        word[word_index] = '\0';
        insertWord(root, word);
    }
    close(fd);
}

// RULE:first arg is dictionary file path
// RULE: other args after are txt files/directories paths
// RULE: exist with status EXIT_SUCCESS if all files could be opened and contained no incorrect words or EXIT_FAILURE otherwise 

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dictionary> <file/directory> [...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    TrieNode* root = getNewTrieNode();
    loadDictionary(root, argv[1]);

    bool foundError = false;
    for (int i = 2; i < argc; i++) {
        struct stat pathStat;
        if (stat(argv[i], &pathStat)) {
            perror("Failed to get path status");
            foundError = true;
            continue;
        }

        if (S_ISDIR(pathStat.st_mode)) {
            // If traverseDirectory returns true, set foundError to true
            if (traverseDirectory(argv[i], root)) {
                foundError = true;
            }
        } else {
            if (processFile(argv[i], root)) {
                foundError = true;
            }
        }
    }

    freeTrie(root);
    return foundError ? EXIT_FAILURE : EXIT_SUCCESS;
}
