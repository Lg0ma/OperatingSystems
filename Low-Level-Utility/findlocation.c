#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>


// Function to compare two strings                                     
int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

size_t my_strlen(char *str) {
    size_t size = 0;
    int i = 0;
    while (*(str + i) != '\0') {
        size++;
        i++;
    }
    return size;
}

void my_print_error(size_t errorNum) {
  char *errMessage;
  
  // Map errors based on errorNum received
  switch(errorNum) {
  case 1:
    errMessage = "Error opening file.\n";
    break;
  case 2:
    errMessage = "Error: string not valid.\n";
    break;
  case 3:
    errMessage = "Error not found.\n";
    break;
  case 4:
    errMessage = "Error reading from standard input.\n";
    break;
  case 5:
    errMessage = "Error: No input from pipe allowed.\n";
    break;
  case 6:
    errMessage = "Error mapping file.\n";
  }
  write(2, errMessage, my_strlen(errMessage));
}

// Function to convert a string to an integer                                                                                                                 
int my_atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
          // ERROR NOT VALID                                                                                                                                  
          my_print_error(2);
          return 1;
        }
    }
    return res;
}

char *my_strncpy(char *dest, const char *src, size_t n){
  size_t i;
  
  for (i = 0; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
  for ( ; i < n; i++)
    dest[i] = '\0';
  
  return dest;
}

char *linear_search(int fd, ssize_t line_count, char *target) {
  char substring[6];
  ssize_t bytes_read;
  char string[32];
  char *result_string;
  
  while((bytes_read = read(fd, string, 32)) > 0) {
    // Copy the numbers from address that will be compared
    my_strncpy(substring , string, 6);
    if(my_strcmp(substring, target) == 0) {
      result_string = malloc(32);
      my_strncpy(result_string, string, 32);
      return result_string;
    }
  }

  // ERROR NOT FOUND
  my_print_error(3);
  return NULL;
}

char *binary_search(char * mapped_file, ssize_t line_count, char *target) {
  size_t low, mid, high;
  char *resultString = malloc(33);
  char mid_string[7];

  low = 0;
  high = line_count;

  while(low <= high) {
    mid = (low + (high - low) / 2);
    my_strncpy(mid_string, mapped_file + (mid * 32), 6);
    mid_string[6] = '\0';
    
    if (my_strcmp(mid_string, target) == 0) {
      my_strncpy(resultString, mapped_file + (mid * 32), 33);
      resultString[32] = '\0';
      // Return string that was found
      return resultString;
    }

    if (my_atoi(mid_string) < my_atoi(target)) {
      low = mid + 1;

    }else{
      high = mid - 1;
    }
  }
  // Target was not found
  free(resultString);
  my_print_error(3);
  return NULL;
}


int main(int argc, char **argv) {
  ssize_t file_size, line_count = 0;
  int fd;
  char *string_to_find = malloc(7);
  char *mapped_file;

  if(my_strlen(argv[1]) < 10 || my_strlen(argv[1]) > 10){
    // ERROR INPUT IS INVALID
    my_print_error(2);
    return 1;
  }
  
  if (argc <= 1 || argc == 1) {
    my_print_error(1);
    return 1;
  }
  else if(argc == 2){
    my_strncpy(string_to_find, argv[1], 6);
    fd = STDIN_FILENO;
  }
  else {
    my_strncpy(string_to_find, argv[1], 6);
    
    fd = open(argv[2], O_RDONLY);
  
    if(fd < 0) {
      // ERROR OPENING FILE
      my_print_error(1);
      return 1;
    }
  }
  
  // Find size of file that will be read using lseek
  file_size = lseek(fd, 0, SEEK_END);

  /* If it fails print the error and instead of doing binary 
     search, we do linear search
   */
  if (file_size < 0) {
    char *target_string = linear_search(fd, line_count, string_to_find);
    write(2, target_string, 33);
    free(target_string);
  }
  else {
  // Get amount of lines from file_size(bytes)
  line_count = file_size / 32;
  // Set offset to start of file
  lseek(fd, 0, SEEK_SET);

  // Map the file using mmap to be able to read it
  mapped_file = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
  if(mapped_file == MAP_FAILED) {
    // ERROR TO MAP TO FUNCTION
    my_print_error(6);
    return 1;
  }
  
  char *target_string = binary_search(mapped_file, line_count, string_to_find);
  write(2, target_string, 33);
  free(target_string);
  }

  free(string_to_find);
  // Have to close file that ws used for call
  if(fd != 0 || fd != STDIN_FILENO) {
    close(fd);
  }
  if(mapped_file && mapped_file != MAP_FAILED){
    munmap(mapped_file, file_size);
  }
}
