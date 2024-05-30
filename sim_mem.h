#ifndef sim_mem_h
#define sim_mem_h
#define MEMORY_SIZE 10
#include <iostream>

#include <fstream>

#include <stdio.h>

#include <unistd.h>

#include <sys/types.h>

#include <fcntl.h>

#include <string.h>

#include <queue>
 //extern char main_memory[MEMORY_SIZE];

typedef struct page_descriptor {
  int V; // valid
  int D; // dirty
  int P; // permission
  int frame; //the number of a frame if in case it is page-mapped
  int swap_index; // where the page is located in the swap file.
}
page_descriptor;

class sim_mem {
  int swapfile_fd; //swap file fd
  int program_fd[2]; //executable file fd
  int text_size;
  int data_size;
  int bss_size;
  int heap_stack_size;
  int num_of_pages;
  int page_size;
  int num_of_proc;
  int swap_size;
  std::queue < int > queues[2];
  std::queue < int > free_swap_indexs, free_memory;
  page_descriptor ** page_table; //pointer to page table
  void store_page_in_memory(int process, int page_index, char * to_store);
  void set_page_info(int proccess, int page_index, int v, int p, int d, int fra, int swap_ind);
  void swap_out(int process);
  char * get_page_from_file(int fd, int index, int size);
  void replace_str_in_file(int fd, int target, int file_size, char * value);
  bool is_bss_stack_heap(int page_index);
  char* get_new_page();
  public:

    sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name2[], int text_size,
      int data_size, int bss_size, int heap_stack_size,
      int num_of_pages, int page_size, int num_of_process);

  ~sim_mem();

  char load(int process_id, int address);

  void store(int process_id, int address, char value);

  void print_memory();

  void print_swap();

  void print_page_table();

};
#endif