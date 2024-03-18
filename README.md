CS214 Systems Programming Project Name: My little malloc() Names & NetIDs: Natalie Chow (njc151) and Andrea Kim (alk203)

`./spchk dict.txt testGood` - exits with EXIT_SUCCESS since testGood directory contains all files that are able to open and contains no incorrect words

- - capWord_Apostrophe.txt - correct regular words with and without apostrophe containing 6 variations in capitalization with all caps, first letter capitalized, and all lower case of the dictionary word "zoos" and "zoo's"

- - capHyphen.txt - correct hyphenated words with 9 variations in capitalization with all caps, first letter capitalized, and all lower case of "worse-octopus", which in the dictionary are "worse" and "octopus"

- - okPunctuation.txt - correct words with trailing punctuations. We have correctly spelled dictionary word "budget" with punctuations at the end of the word. Some of them have quotation marks or brackets at the start of the word.

`./spchk dict.txt testFail` - exits with EXIT_FAILURE since testFail directory contains all files that are able to open, which have incorrect words

- - capsStayCaps.txt - contains all incorrect words. for the dictionary words "NATO" and "MacBride", we tested with variations that have lowercase letters when supposed to be uppercase and vice versa (excluded the first letter). We also combined variations of "NATO" and "MacBride" with a hyphen.

- - weirdChar.txt - contains all incorrect words. With the correct dictionary word "buffalo", we add numbers and special characters inside the word.

- - wrongPunct.txt - contains all incorrect words. With the correct dictionary word "budget" we add punctuation that isn't ' " ( { [ at start of word.

`./spchk dict.txt fake_dir` - prints error file can't be opened since fake_dir isn't a real directory or file name
