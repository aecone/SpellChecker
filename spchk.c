#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
// Expand the alphabet size to handle both lowercase and uppercase letters
#define ALPHABET_SIZE (26*2) 

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    bool isEndOfWord;
} TrieNode;

// Function declarations
TrieNode* getNewTrieNode(void);
void insertWord(TrieNode* root, const char* word);
bool searchWord(TrieNode* root, const char* word);
void freeTrie(TrieNode* root);
void loadDictionary(TrieNode* root, const char* dictionaryPath);
void processFile(const char *filePath, TrieNode* root);
void traverseDirectory(const char *dirPath, TrieNode* root);
void processLine(char *line, TrieNode *root, const char* filePath, long lineNo);

// Create a new trie node
TrieNode* getNewTrieNode(void) {
    TrieNode* pNode = (TrieNode*)malloc(sizeof(TrieNode));
    if (pNode) {
        pNode->isEndOfWord = false;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            pNode->children[i] = NULL;
        }
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
    for (int i = 0; word[i]; i++) {
        if (!isalpha(word[i])) continue; // Ignore non-alphabetical characters

        int index = charToIndex(word[i]);
        if (!crawl->children[index]) {
            crawl->children[index] = getNewTrieNode();
        }
        crawl = crawl->children[index];
    }
    crawl->isEndOfWord = true;
}

//ignore punctation at end of word
//ignore ' " ( { [ at start of word
//hyphenated words are correct if all component words are correctly spelled
// hello (OG), Hello (first cap), HELLO (all caps)-> correct 
// MacDonald (OG), MACDONALD (all caps)-> correct

// Search for a word in the trie
bool searchWord(TrieNode* root, const char* word) {
    TrieNode* crawl = root;
    for (int i = 0; word[i]; i++) {
        if (!isalpha(word[i])) continue; // Ignore non-alphabetical characters

        int index = charToIndex(word[i]);
        if (!crawl->children[index]) return false;
        crawl = crawl->children[index];
    }
    return (crawl != NULL && crawl->isEndOfWord);
}

// Free the trie memory
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


//number all lines (vertically) and each line has columns (horizontally)
//track word col and line number. col # is word's first char
//words are sequences of non-whitespace characters
//every time finds an incorrect word, report the word along with the file, the line, and column number

// Process a line from a file
void processLine(char *line, TrieNode *root, const char* filePath, long lineNo) {
    const char *delimiters = " \t\n.,;:!?'\"()[]{}";
    char *token, *rest = line;
    int tokenStart = 0;
    while ((token = strtok_r(rest, delimiters, &rest))) {
        int tokenLen = strlen(token);
        // Calculate the start position of the token in the line
        char *found = strstr(line + tokenStart, token);
        if (found) {
            int columnNo = found - line + 1; // Column numbers start at 1
            if (!searchWord(root, token)) {
                printf("%s (%ld:%d): %s\n", filePath, lineNo, columnNo, token);
            }
            tokenStart = columnNo + tokenLen - 1; // Update tokenStart to the end of the current token
        }
    }
}

// Process a file
//Must print error if file can't be opened 
void processFile(const char *filePath, TrieNode *root) {
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    char buf[BUFFER_SIZE];
    int bytes_read;
    char *line = malloc(BUFFER_SIZE * sizeof(char)); // Dynamically allocate line buffer
    size_t line_capacity = BUFFER_SIZE;
    size_t line_length = 0;
    long lineNo = 1;

    while ((bytes_read = read(fd, buf, sizeof(buf)-1)) > 0) {
        for (int i = 0; i < bytes_read; ++i) {
            if (buf[i] == '\n') {
                line[line_length] = '\0'; // Null-terminate the line
                processLine(line, root, filePath, lineNo++);
                line_length = 0; // Reset for the next line
            } else {
                if (line_length >= line_capacity - 1) { // Check if buffer is full
                    line_capacity *= 2; // Double the capacity
                    line = realloc(line, line_capacity * sizeof(char)); // Reallocate with new capacity
                }
                line[line_length++] = buf[i];
            }
        }
    }
    if (line_length > 0) { // Process last line if exists
        line[line_length] = '\0';
        processLine(line, root, filePath, lineNo);
    }
    free(line);
    close(fd);
}


//Check spelling in all files end w .txt (in any order)
//Ignore files/dir who begin w . 
// Traverse a directory recursively
void traverseDirectory(const char *dirPath, TrieNode *root) {
    DIR *dir = opendir(dirPath);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    char fullPath[1024];
    struct stat entryStat;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue; // Skip hidden files and directories

        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
        if (stat(fullPath, &entryStat)) {
            perror("Failed to get file status");
            continue;
        }

        if (S_ISDIR(entryStat.st_mode)) traverseDirectory(fullPath, root);
        else if (S_ISREG(entryStat.st_mode) && strstr(fullPath, ".txt")) processFile(fullPath, root); 
    }

    closedir(dir);
}

// first arg is dictionary file path
// other args after are txt files/directories paths
// exist with status EXIT_SUCCESS if all files could be opened and contained no incorrect words or EXIT_FAILURE otherwise 

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dictionary> <file/directory> [...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    TrieNode* root = getNewTrieNode();
    loadDictionary(root, argv[1]);

    for (int i = 2; i < argc; i++) {
        struct stat pathStat;
        if (stat(argv[i], &pathStat)) {
            perror("Failed to get path status");
            continue;
        }

        if (S_ISDIR(pathStat.st_mode)) traverseDirectory(argv[i], root);
        else processFile(argv[i], root);
    }

    freeTrie(root);
    return EXIT_SUCCESS;
}
