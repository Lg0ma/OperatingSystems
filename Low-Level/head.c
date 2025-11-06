#define _DEFAULT_SOURCE 

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>

ssize_t better_write(int fd, const char *buf, size_t count) {
  size_t already_written, to_be_written, written_this_time, max_count;
  ssize_t res_write;

  if (count == ((size_t) 0)) return (ssize_t) count;
  
  already_written = (size_t) 0;
  to_be_written = count;
  while (to_be_written > ((size_t) 0)) {
    max_count = to_be_written;
    if (max_count > ((size_t) 8192)) {
      max_count = (size_t) 8192;
    }
    res_write = write(fd, &(((const char *) buf)[already_written]), max_count);
    if (res_write < ((size_t) 0)) {
      /* Error */
      return res_write;
    }
    if (res_write == ((ssize_t) 0)) {
      /* Nothing written, stop trying */
      return (ssize_t) already_written;
    }
    written_this_time = (size_t) res_write;
    already_written += written_this_time;
    to_be_written -= written_this_time;
  }
  return (ssize_t) already_written;
}

int my_strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (*(unsigned char*)s1) - (*(unsigned char*)s2);
}

int my_atoi(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;

    // Hard-coded max and min values for 32-bit integers
    const int INT_MAX = 2147483647;
    const int INT_MIN = -2147483648;

    // Handle whitespace
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }

    // Handle sign
    if (str[i] == '-' || str[i] == '+') {
        sign = (str[i] == '-') ? -1 : 1;
        i++;
    }

    // Process digits
    while (str[i] >= '0' && str[i] <= '9') {
        // Check for overflow
        if (result > INT_MAX / 10 || (result == INT_MAX / 10 && str[i] - '0' > INT_MAX % 10)) {
            return (sign == 1) ? INT_MAX : INT_MIN;
        }

        result = result * 10 + (str[i] - '0');
        i++;
    }

    return sign * result;
}

int main(int argc, char **argv) {
  int rc;
  char buf[4096];
  ssize_t read_res;
  size_t bytes_read_in, i;
  char byte;
  char *current_line;
  size_t current_line_size, current_line_alloc, new_size;
  void *ptr;
  char **all_lines;
  size_t all_lines_size, all_lines_alloc;
  size_t *all_lines_lengths;
  size_t line_count;
  char *file_name = NULL; // To store the file name if provided
  int input_fd = 0; 
  
  rc = 0;
  current_line = NULL;
  current_line_size = (size_t) 0;
  current_line_alloc = (size_t) 0;
  all_lines = NULL;
  all_lines_lengths = NULL;
  all_lines_size = (size_t) 0;
  all_lines_alloc = (size_t) 0;
  line_count = 9;  

  
  // Loop to find -n flag
  for (int i = 1; i < argc; i++) {
    if (my_strcmp(argv[i], "-n") == 0) {
      if (i + 1 < argc) {
	++i;
	if(my_atoi(argv[i]) < 0) {
	  fprintf(stderr, "Incorrect value for flag\n");
	  rc = 1;
          goto cleanup_and_return;
	}
	line_count = my_atoi(argv[i]);
	line_count--;
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
  
  
  for (;;) {
    if(all_lines_size == line_count) {
      break;
    }
    read_res = read(input_fd, buf, sizeof(buf));
    if (read_res < ((ssize_t) 0)) {
      fprintf(stderr, "Error with read(): %s\n",
	      strerror(errno));
      rc = 1;
      goto cleanup_and_return;
    }
    if (read_res == ((ssize_t) 0)) {
      /* We have hit the EOF condition */
      break;
    }
    
    bytes_read_in = (size_t) read_res;

    for (i=(size_t) 0; i<bytes_read_in; i++) {
      byte = buf[i];
      /* Handle the byte named byte that is part of what we just read
	 in. 

	 Put the byte into the current line, if it is a newline or
	 not.

      */
      if (current_line == NULL) {
	/* This is the very first byte of the current line */
	current_line_alloc = (size_t) 1;
	ptr = calloc(current_line_alloc, sizeof(char));
	if (ptr == NULL) {
	  fprintf(stderr, "Error: could not allocate memory\n");
	  rc = 1;
	  goto cleanup_and_return;
	}
	current_line = (char *) ptr;
      } else {
	/* This is a subsequent byte in the current line, which is
	   already allocated 
	*/
	if (current_line_size >= current_line_alloc) {
	  /* Here we need to reallocate the current line */
	  new_size = current_line_alloc + current_line_alloc;
	  if (new_size < current_line_alloc) {
	    fprintf(stderr, "Error: cannot handle memory that large\n");
	    rc = 1;
	    goto cleanup_and_return;
	  }
	  ptr = reallocarray(current_line, new_size, sizeof(char));
	  if (ptr == NULL) {
	    fprintf(stderr, "Error: could not allocate memory\n");
	    return 1;
	  }
	  current_line_alloc = new_size;
	  current_line = ptr;
	}
      }
      /* The current line, represented by current_line 
	 points to memory and there is at least 1 byte left for 
	 our current to be put in.
      */
      current_line[current_line_size] = byte;
      current_line_size++;

      /* If the byte we just stored was a newline character, we 
	 need to start a new current line.

	 So, we need to store away the current line first.
      */
      if (byte == '\n') {	
	if (all_lines == NULL) {
	  all_lines_alloc = (size_t) 1;
	  ptr = calloc(all_lines_alloc, sizeof(char *));
	  if (ptr == NULL) {
	    fprintf(stderr, "Error: could not allocate memory\n");
	    rc = 1;
	    goto cleanup_and_return;
	  }
	  all_lines = (char **) ptr;
	  ptr = calloc(all_lines_alloc, sizeof(size_t));
	  if (ptr == NULL) {
	    fprintf(stderr, "Error: could not allocate memory\n");
	    rc = 1;
	    goto cleanup_and_return;
	  }
	  all_lines_lengths = (size_t *) ptr;
	} else {
	  if (all_lines_size >= all_lines_alloc) {
	    new_size = all_lines_alloc + all_lines_alloc;
	    if (new_size < all_lines_alloc) {
	      fprintf(stderr, "Error: cannot handle memory that large\n");
	      rc = 1;
	      goto cleanup_and_return;
	    }
	    ptr = reallocarray(all_lines, new_size, sizeof(char *));
	    if (ptr == NULL) {
	      fprintf(stderr, "Error: could not allocate memory\n");
	      return 1;
	    }
	    all_lines = (char **) ptr;
	    ptr = reallocarray(all_lines_lengths, new_size, sizeof(size_t));
	    if (ptr == NULL) {
	      fprintf(stderr, "Error: could not allocate memory\n");
	      rc = 1;
	      goto cleanup_and_return;
	    }
	    all_lines_lengths = (size_t *) ptr;
	    all_lines_alloc = new_size;
	  }
	}
	
	all_lines[all_lines_size] = current_line;
	all_lines_lengths[all_lines_size] = current_line_size;
	all_lines_size++;
	if (all_lines_size == line_count) {
          /* Stop reading more lines*/
          break;
        }
	current_line = NULL;
	current_line_size = (size_t) 0;
	current_line_alloc = (size_t) 0;
      }
    }
  }

  /* Here, we have read all input.

     There is one special case that we need to handle:

     If we have started a current line but that line
     did not end in a newline character, we still need to
     put the current line into the all lines array.

  */
  if (current_line != NULL) {
    if (all_lines == NULL) {
      all_lines_alloc = (size_t) 1;
      ptr = calloc(all_lines_alloc, sizeof(char *));
      if (ptr == NULL) {
	fprintf(stderr, "Error: could not allocate memory\n");
	rc = 1;
	goto cleanup_and_return;
      }
      all_lines = (char **) ptr;
      ptr = calloc(all_lines_alloc, sizeof(size_t));
      if (ptr == NULL) {
	fprintf(stderr, "Error: could not allocate memory\n");
	rc = 1;
	goto cleanup_and_return;
      }
      all_lines_lengths = (size_t *) ptr;
    } else {
      if (all_lines_size >= all_lines_alloc) {
	new_size = all_lines_alloc + all_lines_alloc;
	if (new_size < all_lines_alloc) {
	  fprintf(stderr, "Error: cannot handle memory that large\n");
	  rc = 1;
	  goto cleanup_and_return;
	}
	ptr = reallocarray(all_lines, new_size, sizeof(char *));
	if (ptr == NULL) {
	  fprintf(stderr, "Error: could not allocate memory\n");
	  rc = 1;
	  goto cleanup_and_return;
	}
	all_lines = (char **) ptr;
	ptr = reallocarray(all_lines_lengths, new_size, sizeof(size_t));
	if (ptr == NULL) {
	  fprintf(stderr, "Error: could not allocate memory\n");
	  rc = 1;
	  goto cleanup_and_return;
	}
	all_lines_lengths = (size_t *) ptr;
	all_lines_alloc = new_size;
      }
    }
    all_lines[all_lines_size] = current_line;
    all_lines_lengths[all_lines_size] = current_line_size;
    all_lines_size++;
    current_line = NULL;
    current_line_size = (size_t) 0;
    current_line_alloc = (size_t) 0;
  }
  
  /* Here, we have an array all_lines with all the lines
     
     And we can output these lines again, using write()
     resp. better_write().

  */
  for (i=(size_t) 0; i<all_lines_size && i < line_count; i++) {
    if (better_write(STDOUT_FILENO, all_lines[i], all_lines_lengths[i]) < ((ssize_t) 0)) {
      fprintf(stderr, "Error with write(): %s\n",
	      strerror(errno));
      rc = 1;
      goto cleanup_and_return;
    }
  }

 cleanup_and_return:
  /*
    Check if the pointers are cleared before using
    them again and freeing memory to store new lines
*/
  if(input_fd != 0) {
    close(input_fd);
    // close file descriptor it was opened
  }
  
  if (current_line != NULL) {
    free(current_line);
    current_line = NULL;
  }
  if (all_lines != NULL) {
    for (i=(size_t) 0; i<all_lines_size; i++) {
      if(all_lines[1] != NULL) {
	free(all_lines[i]);
	all_lines[i] = NULL;
      }
    }
    free(all_lines);
    all_lines = NULL;
    free(all_lines_lengths);
    all_lines_lengths = NULL;
    }
  
  return rc;
}
