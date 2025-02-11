#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csse2310a1.h>
#include <ctype.h>

#include "main.h"

#define DEFAULT_DICT_PATH "/usr/share/dict/words"
#define LENGTH_FLAG "-len"
#define MAX_FLAG "-max"
#define FILE_PREFIX_DOT '.'
#define FILE_PREFIX_SLASH '/'
#define HIDDEN_LETTER '-'
#define NEW_LINE '\n'
#define NULL_CHARACTER '\0'

#define MAX_GUESS_LENGTH 50
#define DEFAULT_DICT_LENGTH 10
#define DEFAULT_GUESSES 6
#define DEFAULT_WORD_LENGTH 5

int main(int argc, char* argv[]) {
    // Checking number of arguments given is in bounds
    if (argc > 6) {
        usage_error();
    }

    // Checking individual arguments
    int len = 0, max = 0;
    FILE* wordList = NULL;

    for (int i = 1; i < argc; i++) {
        if (!argv[i]) {
            usage_error();
        }

        // -len flag checks
        if (strcmp(argv[i], LENGTH_FLAG) == 0) {
            check_numerical_flag(argc, argv, &len, &i);

        // -max flag checks
        } else if (strcmp(argv[i], MAX_FLAG) == 0) {
            check_numerical_flag(argc, argv, &max, &i);

        // wordList argument checking
        // Checking the dictonary path begins with '.' or '/'.
        } else if ((strchr(argv[i], FILE_PREFIX_DOT) != NULL) || (
                strchr(argv[i], FILE_PREFIX_SLASH) != NULL)) {
            wordList = fopen(argv[i], "r");

            // Checking exists and can be read
            if (wordList == NULL) {
                fprintf(stderr, "wordle: dictionary file \"%s\" cannot be"
                        " opened\n", argv[i]);
                return 2;
            }

        // Checking all other arguments passed
        } else {
            usage_error();
        }
    }

    // Using default wordlist if none has been specified
    if (wordList == NULL) {
        wordList = fopen(DEFAULT_DICT_PATH, "r");
    }

    init_game(len, max, wordList);
    return 1;
}

int init_game(int wordLen, int maxGuesses, FILE* wordList) {
    int length = (wordLen == 0) ? DEFAULT_WORD_LENGTH : wordLen;
    int attemptsRemaining = (maxGuesses == 0) ? DEFAULT_GUESSES : maxGuesses;
    char* answer = get_random_word(length);
    char* guess;
    size_t guessLength;
    int dictLength = 0;
    char** dictionary = generate_dictionary(wordList, &dictLength, length);
    
    printf("Welcome to Wordle!\n");
    while (attemptsRemaining > 0) {
        if (attemptsRemaining == 1) {
            printf("Enter a %d letter word (last attempt):\n", length);
        } else {
            printf("Enter a %d letter word (%d attempts remaining):\n", 
                    length, attemptsRemaining);
        }

        // Reading user input
        if (getline(&guess, &guessLength, stdin) == -1) {
            free_dictionary(dictionary, dictLength);
            free(guess);
            game_end_error(answer);
        }

        // Cleaning guess
        remove_new_line(guess);

        if (!check_guess(guess, length, dictionary, dictLength)) {
            continue;
        }

        // Exiting if 'guess' contains the correct word
        if (strcmp(guess, answer) == 0) {
            printf("Correct!\n");
            free_dictionary(dictionary, dictLength);
            free(guess);
            free(answer);
            exit(0);
        } else {
            print_letter_matching(answer, length, guess);
        }
        attemptsRemaining--;
    }

    // Exiting if user has reached the max number of attempts
    free_dictionary(dictionary, dictLength);
    free(guess);
    game_end_error(answer);
    return 1;
}

//
// HELPERS
//

// Error functions
void usage_error(void) {
    fprintf(stderr, "Usage: wordle [-len word-length] [-max max-guesses]"
            " [dictionary]\n");
    exit(1);
}

void game_end_error(char* answer) {
    fprintf(stderr, "Bad luck - the word is \"%s\".\n", answer);
    free(answer);
    exit(3);
}

// Mutator functions
void to_lower_word(char* word) {
    for (int i = 0; word[i]; i++) {
        word[i] = tolower(word[i]);
    }
}

void remove_new_line(char* word) {
    int length = strlen(word);
    if (word[length - 1] == NEW_LINE) {
        word[length - 1] = NULL_CHARACTER;
    }
}

// Check functions
int is_alphabetical(char string[]) {
    int length = strlen(string);
    for (int i = 0; i < length; i++) {
        if (isalpha(string[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

int check_guess(char* guess, int length, char** dictionary, int dictLength) {
    // Checking 'guess' contains only alphabetical characters
    if (!is_alphabetical(guess)) {
        printf("Words must contain only letters - try again.\n");
        return 0;
    }

    // Checking 'guess' is of the correct length
    if (strlen(guess) != length) {
        printf("Words must be %d letters long - try again.\n", length);
        return 0;
    };

    // Ensuring guess is case insensitive
    to_lower_word(guess);

    // Checking guess exists is the dictionary.
    if (check_word_exists(dictionary, dictLength, guess) == 0) {
        printf("Word not found in the dictionary - try again.\n");
        return 0;
    }
    return 1;
}

void check_numerical_flag(int argc, char* argv[], int* flag, int* iterator) {
    if (*flag) {
        usage_error();
    }

    // Checking argument after the '-len' flag is a single integer
    if (!((*iterator + 1) < argc) || (strlen(argv[*iterator + 1]) != 1) || 
            !isdigit(*argv[*iterator + 1])) {
        usage_error();
    }

    *flag = atoi(argv[++*iterator]);

    // Checking len argument is in the selected range
    if (*flag < 3 || *flag > 9) {
        usage_error();
    } 
}

int check_word_exists(char** dictionary, int numWords, char* word) {
    // Doing a linear search on dictionary.
    for (int i = 0; i < numWords; i++) {
        if (strcmp(dictionary[i], word) == 0) {
            return 1;
        }
    }
    return 0;
}

// Other functions
char** generate_dictionary(FILE* file, int* numWords, int answerLength) {
    char** dictionary;
    int maxDictWords = DEFAULT_DICT_LENGTH;
    char tmpWord[MAX_GUESS_LENGTH];

    dictionary = calloc(DEFAULT_DICT_LENGTH, sizeof(char*));

    while (fgets(tmpWord, MAX_GUESS_LENGTH, file)) {
        
        remove_new_line(tmpWord);

        // Only adding words that are of length 'answerLength'
        if (strlen(tmpWord) != answerLength) {
            continue;
        }

        dictionary[(*numWords)++] = strdup(tmpWord);

        // Increasing size of dictionary by 'DEFAULT_DICT_LENGTH' if
        // reached size limit
        if (*numWords == maxDictWords) {
            dictionary = realloc(dictionary, 
                    (maxDictWords + DEFAULT_DICT_LENGTH) * sizeof(char*));
            maxDictWords += DEFAULT_DICT_LENGTH;
        }
    }
    fclose(file);
    return dictionary;
}

void print_letter_matching(char* answer, int length, char* guess) {
    // Defining the final string to be printed
    char* output = (char*)calloc(length, sizeof(char));
    // Sequence of character flags so that a letter match is only printed once
    char* lettersFound = (char*)calloc(length, sizeof(char));
    int lettersFoundLength = 0;

    // First iteration: Finding correct letters
    for (int i = 0; i < length; i++) {
        if (guess[i] == answer[i]) {
            output[i] = toupper(guess[i]);
            lettersFound[lettersFoundLength++] = guess[i];
        }
    }

    // Second iteration: Finding other existing letters
    for (int i = 0; i < length; i++) {
        // Skipping if element exists in 'answer' position 'i'
        if (output[i] != 0) {
            continue;
        
        // Showing letter if it exists in the answer and has not been seen
        } else if ((strchr(answer, guess[i]) != NULL) && 
                (strchr(lettersFound, guess[i]) == NULL)) {
            output[i] = guess[i];
            lettersFound[lettersFoundLength++] = guess[i];
        } else {
            output[i] = HIDDEN_LETTER;
        }
    }
    printf("%s\n", output);
    free(output);
    free(lettersFound);
}

void free_dictionary(char** dictionary, int numWords) {
    for (int i = 0; i < numWords; i++) {
        free(dictionary[i]);
    }
    free(dictionary);
}