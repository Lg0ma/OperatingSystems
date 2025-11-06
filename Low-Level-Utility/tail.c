#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
//
// Function to compare two strings
int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

// Function to convert a string to an integer
int my_atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
            return -1; // Return -1 if the string is not a valid number
        }
    }
    return res;
}

int main(int argc, char *argv[]) {
    int input_fd = STDIN_FILENO; // Default input is stdin
    char *file_name = NULL;
    int line_count = 10; // Default number of lines
    int rc = 0;
    char buffer[4096];
    ssize_t bytes_read;

    // Loop to find -n flag
    for (int i = 1; i < argc; i++) {
        if (my_strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                ++i;
                if (my_atoi(argv[i]) < 0) {
                    fprintf(stderr, "Incorrect value for flag\n");
                    rc = 1;
                    goto cleanup_and_return;
                }
                line_count = my_atoi(argv[i]);
                if (line_count < 0) {
                    fprintf(stderr, "Incorrect value for flag\n");
                    rc = 1;
                    goto cleanup_and_return;
                }
            } else {
                fprintf(stderr, "Option -n requires a number\n");
                rc = 1;
                goto cleanup_and_return;
            }
        } else if (file_name == NULL) {
            // First non-option argument is treated as the file name
            file_name = argv[i];
        } else {
            fprintf(stderr, "Error: Unexpected argument '%s'\n", argv[i]);
            rc = 1;
            goto cleanup_and_return;
        }
    }

    // Open the file if a file name was provided
    if (file_name) {
        input_fd = open(file_name, O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "Error opening input file: %s\n", strerror(errno));
            rc = 1;
            goto cleanup_and_return;
        }
    }

    // Allocating memory for when we nned to store the lines
    char **all_lines = (char **)malloc(line_count * sizeof(char *));
    size_t *all_lines_lengths = (size_t *)malloc(line_count * sizeof(size_t));
    size_t all_lines_size = 0;

    // Variables to store current line being read
    char *current_line = NULL;
    size_t current_line_size = 0;

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                current_line = realloc(current_line, current_line_size + 1);
                current_line[current_line_size] = '\0';

                /* Circular buffer logic: Loop through the content of the file
		   with a size of n given from the flag until it reaches the 
		   end of the file.
		*/
                if (all_lines_size < line_count) {
                    all_lines[all_lines_size] = current_line;
                    all_lines_lengths[all_lines_size] = current_line_size;
                    all_lines_size++;
                } else {
                    free(all_lines[0]);
                    for (size_t j = 0; j < line_count - 1; j++) {
                        all_lines[j] = all_lines[j + 1];
                        all_lines_lengths[j] = all_lines_lengths[j + 1];
                    }
                    all_lines[line_count - 1] = current_line;
                    all_lines_lengths[line_count - 1] = current_line_size;
                }

                // Reset for the next input
                current_line = NULL;
                current_line_size = 0;
            } else {
                current_line = realloc(current_line, current_line_size + 1);
                current_line[current_line_size] = buffer[i];
                current_line_size++;
            }
        }
    }
    // Handle error where we cant read 
    if (bytes_read < 0) {
        fprintf(stderr, "Error reading input file: %s\n", strerror(errno));
        rc = 1;
        goto cleanup_and_return;
    }

    // If we have started a current line but that line did not end in a newline character,
    // we still need to put the current line into the all lines array.
    if (current_line != NULL) {
        if (all_lines_size < line_count) {
            all_lines[all_lines_size] = current_line;
            all_lines_lengths[all_lines_size] = current_line_size;
            all_lines_size++;
        } else {
            free(all_lines[0]);
            for (size_t j = 0; j < line_count - 1; j++) {
                all_lines[j] = all_lines[j + 1];
                all_lines_lengths[j] = all_lines_lengths[j + 1];
            }
            all_lines[line_count - 1] = current_line;
            all_lines_lengths[line_count - 1] = current_line_size;
        }
    }

    // Output the stored lines
    for (size_t i = 0; i < all_lines_size; i++) {
        write(STDOUT_FILENO, all_lines[i], all_lines_lengths[i]);
        write(STDOUT_FILENO, "\n", 1);
    }

cleanup_and_return:
    // Free allocated memory
    for (size_t i = 0; i < all_lines_size; i++) {
        free(all_lines[i]);
    }
    free(all_lines);
    free(all_lines_lengths);

    // Close file descriptor if opened
    if (input_fd != STDIN_FILENO) {
        close(input_fd);
    }

    return rc;
}
