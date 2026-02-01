#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
  if(argc != 5) {
      printf("Usage: ./additional [num 1] [num 2] [num 3] [num 4]\n");
      return EXIT_FAILURE;
  }
  
  int nums[4];

  for(int i = 1; i < 5; i++)
  {
  	nums[i-1] = atoi(argv[i]);
  }

  printf("%d %d\n", fibonacci(nums[0]), max_of_four_int(nums[0], nums[1], nums[2], nums[3]));

  return EXIT_SUCCESS;
}
