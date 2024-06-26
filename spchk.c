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
#define ALPHABET_SIZE (128) 

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    bool isEndOfWord;
} TrieNode;

// Trie-related operations
TrieNode* getNewTrieNode(void);
void insertWord(TrieNode* root, const char* word);
bool searchWord(TrieNode* root, TrieNode* lowercaseRoot, char* word);
void freeTrie(TrieNode* root);

// Utility functions
bool isAllUpperCase(const char *word);
bool hasOnlyFirstLetterCapitalized(const char *word);
bool hasHyphen(const char* word);
bool hasSpace(const char* word);
bool checkLowercase(TrieNode* lowercaseRoot, const char* word);
int isLeadingPunctuation(char c);
int isTrailingPunctuation(char c);
void trimPunctuation(char* word);
bool processHyphenatedWord(TrieNode* root, TrieNode* lowercaseRoot, const char* hyphenatedWord);
bool processSpaceSeparatedWords(TrieNode* root, TrieNode* lowercaseRoot, const char* spaceSeparatedWords);


// Processing functions
bool checkAndReportWord(char *word, TrieNode *root, TrieNode* lowercaseRoot, const char* filePath, long lineNo, int columnNo);
bool processLine(char *line, TrieNode *root,TrieNode* lowercaseRoot, const char* filePath, long lineNo);
bool processFile(const char *filePath, TrieNode *root, TrieNode* lowercaseRoot);
bool traverseDirectory(const char *dirPath, TrieNode *root, TrieNode* lowercaseRoot);
bool isValidWordChar(char c, bool start);
// Main entry
int main(int argc, char *argv[]);

// SECTION 4 ----------------------------------------------------------------
// Checking whether a word is contained in the dictionary

// Check if word is all capitalized
bool isAllUpperCase(const char *word) {
    while (*word) {
        if (islower(*word)) return false;
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

//Check if word has a hyphen in it
bool hasHyphen(const char* word) {
    for (int i = 0; word[i] != '\0'; i++) {
        if (word[i] == '-') {
            return true;
        }
    }
    return false;
}

//Check if word has a space in it
bool hasSpace(const char* word) {
    for (int i = 0; word[i] != '\0'; i++) {
        if (word[i] == ' ') {
            return true;
        }
    }
    return false;
}

//Check if the word's all lowercase version exists: ex: for Apple, check if apple exists
bool checkLowercase(TrieNode* lowercaseRoot, const char* word) {
    TrieNode* crawl = lowercaseRoot;
    for (int i = 0; word[i]; i++) {
        int index = tolower(word[i]); // Always convert to lowercase
        if (!crawl->children[index]) {
            // printf("Check lowercase: Word not found: Character is %c\n", word[i]);
            return false; // Word not found
        }
        crawl = crawl->children[index];
    }
    return (crawl != NULL && crawl->isEndOfWord); // Word found
}

// Check if a character is a punctuation that should be ignored at the start
int isLeadingPunctuation(char c) {
    return c == '\'' || c == '"' || c == '(' || c == '{' || c == '[';
}

// Check if a character is a punctuation that should be ignored at the end
int isTrailingPunctuation(char c) {
    return ispunct(c) || isLeadingPunctuation(c) || c == ')' || c == '}' || c == ']';
}

// Trims leading and ending punctuation from a word
void trimPunctuation(char* word) {
    int length = strlen(word);
    int start = 0, end = length - 1;

    while(start < length && isLeadingPunctuation(word[start])) {
        start++;
    }

    while(end > start && isTrailingPunctuation(word[end])) {
        end--;
    }

    int keepCount = end - start + 1;

    for (int i = 0; i < keepCount; i++) {
        word[i] = word[start + i];
    }
    word[keepCount] = '\0';
    
}

// Processes and checks each segment of a hyphenated word
bool processHyphenatedWord(TrieNode* root, TrieNode* lowercaseRoot, const char* hyphenatedWord) {
    char tempWord[256]; 
    int j = 0;
    bool isValid = true; 
    for (int i = 0; hyphenatedWord[i] != '\0'; ++i) {
        if (hyphenatedWord[i] != '-') {
            tempWord[j++] = hyphenatedWord[i];
        }

        if (hyphenatedWord[i] == '-' || hyphenatedWord[i + 1] == '\0') {
            tempWord[j] = '\0'; 

            // Check the segment against the trie
            if (!searchWord(root, lowercaseRoot, tempWord)) {
                isValid = false; 
                break;
            }

            j = 0;
        }
    }

    return isValid;
}

// Processes and checks each segment of words separated by one or more spaces
bool processSpaceSeparatedWords(TrieNode* root, TrieNode* lowercaseRoot, const char* spaceSeparatedWords) {
    char tempWord[256]; 
    int j = 0; 
    bool isValid = true; 

    for (int i = 0; spaceSeparatedWords[i] != '\0'; ++i) {
        if (spaceSeparatedWords[i] == ' ') {
            if (j > 0) { 
                tempWord[j] = '\0'; 
                if (!searchWord(root, lowercaseRoot, tempWord)) {
                    isValid = false; 
                    break;
                }
                j = 0; 
            }
            // Skip consecutive spaces
            while (spaceSeparatedWords[i+1] == ' ') i++;
        } else {
            tempWord[j++] = spaceSeparatedWords[i];
        }
    }

    // Check the last word if there was no trailing space
    if (j > 0) {
        tempWord[j] = '\0'; 
        if (!searchWord(root, lowercaseRoot, tempWord)) {
            isValid = false;
        }
    }

    return isValid;
}

// MAIN SEARCH FUNCTION: performs checks on words to see if they are in dictionary/valid
// RULE: hello (OG), Hello (first cap), HELLO (all caps)-> correct 
// RULE: MacDonald (OG), MACDONALD (all caps)-> correct
bool searchWord(TrieNode* root, TrieNode* lowercaseRoot, char* word) {
    trimPunctuation(word); //trims the word to ignore certain punctuation
    
    //If the line includes a space, it processes the words separately by space
    if(hasSpace(word)){
        if(processSpaceSeparatedWords(root, lowercaseRoot, word)){
            return true;
        }
    }

    //If the line includes a hyphen, it processes the words separately by hyphen
    if(hasHyphen(word)){
        if(processHyphenatedWord(root, lowercaseRoot, word)){
            return true;
        }
    }

    // If the word is all caps, it checks if any other variation of the word exists
    if (isAllUpperCase(word)) {
        TrieNode* crawl = root;
        for (int i = 0; word[i]; i++) {
            int index = word[i]; 
            if (!crawl->children[index]) {
                if(!checkLowercase(lowercaseRoot, word)){
                    return false;
                }
                else{
                    return true;
                }
            }
            crawl = crawl->children[index];
        }
        if (crawl != NULL && crawl->isEndOfWord) return true;
    }

    // If the word has only the first letter capitalized, it checks if the all lowercase version of the word exists
    if (hasOnlyFirstLetterCapitalized(word)) { 
        TrieNode* crawl = root;
        for(int i = 0; word[i]; i++){
            int lowerLetterIndex = tolower(word[i]);
            if (!crawl->children[lowerLetterIndex]) {
                return false;
            }
            crawl = crawl->children[lowerLetterIndex];
        }
        if (crawl != NULL && crawl->isEndOfWord) return true;
    }

    // If other checks don't return true, checks exact match (with any capital letters or no capital letters) Hello == Hello, HEllo == HEllo, hello == hello
    TrieNode* crawlExact = root;
    for (int iter = 0; word[iter]; iter++) {
        int index = word[iter];
        if (!crawlExact->children[index]) {
            return false;
        }
        crawlExact = crawlExact->children[index];
    }
    if (crawlExact != NULL && crawlExact->isEndOfWord) return true;

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
    if (start && ispunct(c)) return true;
    return !start; // If not the start, it's a valid character (handles end punctuation)
}

// Report a word if it's not found in the trie. Returns true if an incorrect word was found, false otherwise
//RULE: every time finds an incorrect word, report the word along with the file, the line, and column number
bool checkAndReportWord(char *word, TrieNode *root, TrieNode* lowercaseRoot, const char* filePath, long lineNo, int columnNo) {
    if (!searchWord(root, lowercaseRoot, word)) {
        printf("%s (%ld:%d): %s\n", filePath, lineNo, columnNo, word);
        return true; // Incorrect word found
    }
    return false; // No incorrect word found
}

// Process a line from a file, checking each word against a trie data structure. Returns true if an incorrect word was found, false otherwise
// RULE: hyphenated words are correct if all component words are correctly spelled
// Returns true if any incorrect word was found in the line, false otherwise
bool processLine(char *line, TrieNode *root, TrieNode* lowercaseRoot, const char* filePath, long lineNo) {
    char word[BUFFER_SIZE];
    int wordIndex = 0;
    int columnNo = 1;
    int startColumn = 1; // Initialize startColumn to 1 for accurate column tracking
    bool foundIncorrectWord = false;

    for (int i = 0; line[i] != '\0'; i++) {
        if (isValidWordChar(line[i], wordIndex == 0) || line[i] == ' ') {
            if (line[i] == ' ') {
                if (wordIndex > 0) {
                    word[wordIndex] = '\0';
                    if (checkAndReportWord(word, root, lowercaseRoot, filePath, lineNo, startColumn)) {
                        foundIncorrectWord = true;
                    }
                    wordIndex = 0; 
                }
                startColumn = columnNo + 1;
            } else {
                if (wordIndex == 0) startColumn = columnNo;
                word[wordIndex++] = line[i];
            }
        } else if (wordIndex > 0) {
            word[wordIndex] = '\0';
            if (checkAndReportWord(word, root, lowercaseRoot, filePath, lineNo, startColumn)) {
                foundIncorrectWord = true;
            }
            wordIndex = 0;
            startColumn = columnNo + 1;
        }
        columnNo++; 
    }

    if (wordIndex > 0) {
        word[wordIndex] = '\0';
        if (checkAndReportWord(word, root, lowercaseRoot, filePath, lineNo, startColumn)) {
            foundIncorrectWord = true;
        }
    }

    return foundIncorrectWord;
}



// SECTION 3 ----------------------------------------------------------------
//Reading the file and generating a sequence of position-annotated words

//RULE: Must print error if file can't be opened. 
// Returns true if any incorrect word was found in the file, false otherwise
bool processFile(const char *filePath, TrieNode *root, TrieNode* lowercaseRoot) {
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
    size_t line_length = 0;
    long lineNo = 1;
    bool foundIncorrectWord = false;

    while ((bytes_read = read(fd, buf, sizeof(buf)-1)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buf[i] == '\n') {
                line[line_length] = '\0';
                if (processLine(line, root, lowercaseRoot, filePath, lineNo++)) {
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
        if (processLine(line, root, lowercaseRoot, filePath, lineNo)) {
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
bool traverseDirectory(const char *dirPath, TrieNode *root, TrieNode* lowercaseRoot) {
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
            if (traverseDirectory(fullPath, root, lowercaseRoot)) {
                foundIncorrectWord = true;
            }
        } else if (S_ISREG(entryStat.st_mode) && strstr(fullPath, ".txt")) {
            if (processFile(fullPath, root, lowercaseRoot)) {
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

// Insert a word into the trie
void insertWord(TrieNode* root, const char* word) {
    TrieNode* crawl = root;
    bool isValidWord = true;

    for (int i = 0; word[i] && isValidWord; i++) {
        int index = word[i];

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

// Load the dictionary and create two tries: one as is and another with all words in lowercase
void loadDictionary(TrieNode* root, TrieNode* lowercaseRoot, const char* dictionaryPath) {
    int fd = open(dictionaryPath, O_RDONLY);
    if (fd < 0) {
        perror("Unable to open the dictionary file");
        exit(EXIT_FAILURE);
    }

    char buf[BUFFER_SIZE];
    int bytes_read;
    int word_index = 0; // Declare and initialize word_index here
    char word[BUFFER_SIZE] = {0};
    char lowerWord[BUFFER_SIZE] = {0}; // For storing the lowercase version

    while ((bytes_read = read(fd, buf, sizeof(buf)-1)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buf[i] == '\n' || buf[i] == '\r') {
                if (word_index > 0) { // We have a complete word to process
                    word[word_index] = '\0'; // Null-terminate the word
                    strcpy(lowerWord, word); // Copy original word to lowerWord
                    for (int j = 0; lowerWord[j]; ++j) {
                        lowerWord[j] = tolower(lowerWord[j]); // Convert to lowercase
                    }
                    insertWord(root, word); // Insert original word
                    insertWord(lowercaseRoot, lowerWord); // Insert lowercase word
                    word_index = 0; // Reset for the next word
                }
            } else {
                word[word_index++] = buf[i];
            }
        }
    }
    if (word_index > 0) { // Insert last word if exists
        word[word_index] = '\0'; // Null-terminate the word
        strcpy(lowerWord, word); // Copy original word to lowerWord
        for (int j = 0; lowerWord[j]; ++j) {
            lowerWord[j] = tolower(lowerWord[j]); // Convert to lowercase
        }
        insertWord(root, word); // Insert original word
        insertWord(lowercaseRoot, lowerWord); // Insert lowercase word
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
    TrieNode* lowercaseRoot = getNewTrieNode();
    loadDictionary(root, lowercaseRoot, argv[1]);

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
            if (traverseDirectory(argv[i], root, lowercaseRoot)) {
                foundError = true;
            }
        } else {
            if (processFile(argv[i], root, lowercaseRoot)) {
                foundError = true;
            }
        }
    }

    freeTrie(root);
    return foundError ? EXIT_FAILURE : EXIT_SUCCESS;
}
