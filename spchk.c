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
    FILE *file = fopen(dictionaryPath, "r");
    if (!file) {
        perror("Unable to open the dictionary file");
        exit(EXIT_FAILURE);
    }

    char word[BUFFER_SIZE];
    while (fgets(word, BUFFER_SIZE, file)) {
        size_t len = strlen(word);
        if (len > 0 && (word[len - 1] == '\n' || word[len - 1] == '\r')) {
            word[len - 1] = '\0';
        }
        insertWord(root, word);
    }
    fclose(file);
}

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
void processFile(const char *filePath, TrieNode *root) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    long lineNo = 0;
    while ((read = getline(&line, &len, file)) != -1) {
        lineNo++;
        processLine(line, root, filePath, lineNo);
    }

    free(line);
    fclose(file);
}

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
