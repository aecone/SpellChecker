# CS214 Systems Programming Project Name: SpellChecker; Names & NetIDs: Natalie Chow (njc151) and Andrea Kim (alk203)

## OVERVIEW:

This spell checker program is designed to validate the spelling of words in text files against a provided dictionary. It handles a variety of cases such as words with hyphens, spaces, and different capitalization styles. It uses a Trie data structure to efficiently store and search for words in the dictionary.

## FEATURES:

Trie Data Structure: Utilizes a Trie for efficient dictionary word storage and lookup.
Case Sensitivity Handling: Checks words in their original form, all lowercase, and with the first letter capitalized.
Hyphenated Words: Correctly handles hyphenated words, checking each component word against the dictionary.
Space-separated Words: Processes space-separated words, validating each word individually.
Punctuation Ignoring: Trims leading and trailing punctuation from words before checking their spelling.

## HOW IT WORKS:

The program starts by loading the provided dictionary into two Trie data structures—one for the words as they appear and another for their lowercase versions. It then reads the input files or directories recursively, processing each text file found. Each line in a text file is checked word by word. Words are validated against the dictionary, with special handling for case sensitivity, hyphenation, and space separation.

## Running Program

To use the spell checker (spchk), enter 'make' to compile the program. After specify the dictionary file followed by the text file or directory you wish to check. For example:

```bash
./spchk dictionary.txt input.txt
```

In this command, `dictionary.txt` is the path to the dictionary file, and `input.txt` is the text file to check. You may also specify a directory in place of a single file to recursively check all `.txt` files within that directory.

## Test Cases

### Successful Test Case

**Command:**

```bash
./spchk dict.txt testGood
```

**Expected Outcome:**

- Exits with `EXIT_SUCCESS`.
- The `testGood` directory contains files that open successfully and contain no spelling errors.

**Test Files in `testGood` Directory:**

- `capWord_Apostrophe.txt`:

  - Tests correct regular words with variations in capitalization including words with apostrophes (dictionary words: "zoos" and "zoo's").
  - Contains 6 variations in capitalization with all caps, first letter capitalized, and all lower case words.

- `capHyphen.txt`:

  - Tests correct hyphenated words with variations in capitalization (dictionary word: "worse" and "octopus").
  - 9 variations in capitalization with all caps, first letter capitalized, and all lower case of worse-octopus.

- `okPunctuation.txt`:
  - Tests correct words with trailing punctuations at end of word and potential leading quotation marks or brackets (dictionary word: "budget").

### Failure Test Case

**Command:**

```bash
./spchk dict.txt testFail
```

**Expected Outcome:**

- Exits with `EXIT_FAILURE`.
- The `testFail` directory contains files and any directory's file that open successfully but include spelling errors.

**Test Files in `testFail` Directory:**

- All the files in `testFail` fail since they contain incorrect words.

- `capsStayCaps.txt`:

  - Contains variations that have lowercase letters when supposed to be uppercase and vice versa (excluded the first letter). (dictionary word: "NATO" and "MacBride")
  - We also combined variations of "NATO" and "MacBride" with a hyphen.

- `weirdChar.txt`:

  - Contains words with added numbers or special characters inside. (dictionary word: "buffalo").

- `wrongPunct.txt`:

  - Contains words with punctation that isn't ' " ( { [ at start of word (dictionary word: "budget").

- `spaces.txt`:
  - Tests the correct column numbers output for words with incorrect words, such as having < / at start of word or having numbers within the word. (dictionary word:, "buffalo", "bug", "poop").

**Test Directory in `testFail` Directory:**

**Command:**

```bash
./spchk dict.txt testFail/testTraverse
```

**Expected Outcome:**

- Exits with `EXIT_FAILURE`.
- The `testTraverse` directory contains the fail.txt file that open successfully but include spelling errors ( contains a nonexistent dictionary word)
- Only shows the fail.txt error, nothing else from grandparent directory

### Error Handling

**Command:**

```bash
./spchk dict.txt fake_dir
```

**Expected Outcome:**

- Prints an error message indicating the directory cannot be opened (e.g., `fake_dir` is not a valid directory).

**Command:**

```bash
./spchk dict.txt deadFile.txt
```

**Expected Outcome:**

- Prints an error message indicating the file cannot be opened (e.g., `deadFile.txt` is not a valid file name).
