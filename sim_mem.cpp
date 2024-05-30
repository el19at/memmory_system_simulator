#ifndef sim_mem_cpp
#define sim_mem_cpp
#include "sim_mem.h"

char main_memory[MEMORY_SIZE];

/**
 * This function is used to initialize the memory.
 * 
 * @param exe_file_name1 The name of the executable file for process 1.
 * @param exe_file_name2 The name of the executable file for the second process.
 * @param swap_file_name The name of the swap file.
 * @param text_size size of the text segment
 * @param data_size size of the data segment
 * @param bss_size size of the bss segment
 * @param heap_stack_size The size of the heap and stack combined.
 * @param num_of_pages number of pages in the physical memory
 * @param page_size The size of each page in char.
 * @param num_of_process number of processes (max 2)
 */
sim_mem::sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name[], int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size, int num_of_process) {
    this -> text_size = text_size;
    this -> data_size = data_size;
    this -> bss_size = bss_size;
    this -> heap_stack_size = heap_stack_size;
    this -> num_of_pages = num_of_pages;
    this -> page_size = page_size;
    this -> num_of_proc = num_of_process;
    this -> swap_size = num_of_proc*(page_size *num_of_pages - text_size);
    this -> page_table = (page_descriptor ** ) malloc(this -> num_of_proc * sizeof(page_descriptor * ));
    if (page_table == NULL) {
        perror("malloc");
        exit(1);
    }
    this -> swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT, 0644);
    if (this -> swapfile_fd < 0) {
        perror("swap file");
        free(this -> page_table);
        exit(1);
    }
    for (int i = 0; i < swap_size; i++) {
        write(this -> swapfile_fd, "0", 1);
        if (i % page_size == 0)
            free_swap_indexs.push(i); // store the free swap emplacement
    }
    this -> program_fd[0] = open(exe_file_name1, O_RDWR);
    if (program_fd[0] < 0) {
        perror("exe file 1");
        close(this -> swapfile_fd);
        free(this -> page_table);
        exit(1);
    }
    this -> page_table[0] = (page_descriptor * ) malloc(num_of_pages * sizeof(page_descriptor));
    if (this -> page_table[0] == NULL) {
        perror("malloc");
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table);
        exit(1);
    }
    for (int i = 0; i < num_of_pages; i++)
        if (i < text_size / page_size)
            set_page_info(0, i, 0, 0, 0, -1, -1);
        else
            set_page_info(0, i, 0, 1, 0, -1, -1);

    if (this -> num_of_proc == 2) {
        this -> program_fd[1] = open(exe_file_name2, O_RDWR);
        if (program_fd[1] < 0) {
            perror("exe file 2");
            close(this -> swapfile_fd);
            close(this -> program_fd[0]);
            free(this -> page_table[0]);
            free(this -> page_table);
            exit(1);
        }
        this -> page_table[1] = (page_descriptor * ) malloc(num_of_pages * sizeof(page_descriptor)); //
        if (this -> page_table[0] == NULL) {
            perror("malloc");
            close(this -> swapfile_fd);
            close(this -> program_fd[0]);
            close(this -> program_fd[1]);
            free(this -> page_table[0]);
            free(this -> page_table);
            exit(1);
        }
        for (int i = 0; i < num_of_pages; i++)
            if (i < text_size / page_size)
                set_page_info(1, i, 0, 0, 0, -1, -1);
            else
                set_page_info(1, i, 0, 1, 0, -1, -1);
    }
    //clear main memory
    for (int i = 0; i < MEMORY_SIZE; i++) {
        main_memory[i] = '0';
        if (i % page_size == 0)
            free_memory.push(i);
    }
}
sim_mem::~sim_mem() {
    close(this -> swapfile_fd);
    close(this -> program_fd[0]);
    free(this -> page_table[0]);
    if (this -> num_of_proc == 2) {
        close(this -> program_fd[1]);
        free(this -> page_table[1]);
    }
    free(this -> page_table);
}

/**
 * It loads the value at the address of the process_id.
 * 
 * @param process_id The process id of the process that is requesting the memory access.
 * @param address The address of the memory location to be loaded.
 */
char sim_mem::load(int process_id, int address) {
    int index_prog = process_id - 1; //select the desired program
    int page_index = address / page_size, offset = address % page_size;
    if (page_table[index_prog][page_index].V == 1) //the page is in memory
        return main_memory[page_table[index_prog][page_index].frame + offset];
    //page not in memory
    if (page_table[index_prog][page_index].P == 0) { //text case read from the exe
        char * page_to_load = get_page_from_file(program_fd[index_prog], page_index, this -> page_size);
        store_page_in_memory(index_prog, page_index, page_to_load);
        free(page_to_load);
        return main_memory[page_table[index_prog][page_index].frame + offset];
    }
    if (page_table[index_prog][page_index].D == 0) {
        if(address>this->text_size+this->data_size+this->bss_size){//fist access to heap stak
            printf("cannot access heap_stack area (unitialized)\n");
            return '\0';
        }
        if(address>this->text_size+this->data_size){// first access to bss
            char* new_page = get_new_page();
            store_page_in_memory(index_prog,page_index,new_page);
            free(new_page);
            return main_memory[page_table[index_prog][page_index].frame + offset];
        }
        char * page_to_load = get_page_from_file(program_fd[index_prog], page_index, this -> page_size);
        store_page_in_memory(index_prog, page_index, page_to_load);
        free(page_to_load);
        return main_memory[page_table[index_prog][page_index].frame + offset];
    }
    char * page_to_load = get_page_from_file(swapfile_fd, page_table[index_prog][page_index].swap_index, this -> page_size);
    this->free_swap_indexs.push(page_table[index_prog][page_index].swap_index);
    store_page_in_memory(index_prog, page_index, page_to_load);
    free(page_to_load);
    return main_memory[page_table[index_prog][page_index].frame + offset];
}

/**
 * 
 * 
 * @param process_id The process that is requesting the store.
 * @param address The address to store the value at.
 * @param value The value to be stored in memory
 */
void sim_mem::store(int process_id, int address, char value) {
    int index_prog = process_id - 1; //select the desired program
    int page_index = address / page_size, offset = address % page_size;
    if (page_table[index_prog][page_index].P == 0) { //text case cant store
        printf("text area you cant write here!\n");
        return;
    }
    if (page_table[index_prog][page_index].V == 1) { //the page is in memory
        main_memory[page_table[index_prog][page_index].frame + offset] = value;
        return;
    }
    //page not in memory
    if (page_table[index_prog][page_index].D == 0) { //write to memery (import page from file)
        if(address>this->text_size+this->data_size){// first access to bss or heap_stack
            char* new_page = get_new_page();
            store_page_in_memory(index_prog,page_index,new_page);
            free(new_page);
        }
        else{
            char * page_to_load = get_page_from_file(program_fd[index_prog], page_index, this -> page_size);
            store_page_in_memory(index_prog, page_index, page_to_load);
            free(page_to_load);
        }
        main_memory[page_table[index_prog][page_index].frame + offset] = value;
        set_page_info(index_prog, page_index, 1, 1, 1, page_table[index_prog][page_index].frame, -1);
        return;
    }
    //write to memery (import page from swap)
    char * page_to_load = get_page_from_file(swapfile_fd, page_table[index_prog][page_index].swap_index, this -> page_size);
    this->free_swap_indexs.push(page_table[index_prog][page_index].swap_index);
    store_page_in_memory(index_prog, page_index, page_to_load);
    free(page_to_load);
    main_memory[page_table[index_prog][page_index].frame + offset] = value;
    return;
}
/**
 * 
 * 
 * @param fd The file descriptor of the file you want to read from.(file/swap)
 * @param index the index of the page in the file
 * @param size the size of the page to read
 */
char* sim_mem:: get_new_page(){
    char* res = (char*)malloc(sizeof(char)*(this->page_size+1));
    if (res == NULL) {
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table[0]);
        if (this -> num_of_proc == 2) {
            close(this -> program_fd[1]);
            free(this -> page_table[1]);
        }
        free(this -> page_table);
    }
    int i=0;
    for(;i<this->page_size;i++)
        res[i]='0';
    res[i]='\0';
    return res;
}
char * sim_mem::get_page_from_file(int fd, int index, int size) {
    char * res = (char * ) malloc((size + 1) * sizeof(char));
    if (res == NULL) {
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table[0]);
        if (this -> num_of_proc == 2) {
            close(this -> program_fd[1]);
            free(this -> page_table[1]);
        }
        free(this -> page_table);
    }
    lseek(fd, index * size, SEEK_SET);
    read(fd, res, size);
    if (fd == this -> swapfile_fd) { //clean page in the swap 
        char * cleaner = (char * ) malloc((page_size + 1) * sizeof(char));
        if (cleaner == NULL) {
            close(this -> swapfile_fd);
            close(this -> program_fd[0]);
            free(this -> page_table[0]);
            if (this -> num_of_proc == 2) {
                close(this -> program_fd[1]);
                free(this -> page_table[1]);
            }
            free(this -> page_table);
        }
        for (int i = 0; i < page_size; i++)
            cleaner[i] = '0';
        cleaner[page_size] = '\0';
        replace_str_in_file(swapfile_fd, index, swap_size, cleaner);
        free(cleaner);
    }
    return res;
}
/**
 * This function stores the page in the main memory.
 * 
 * @param process The process number of the process that is requesting the storing.
 * @param page_index The index of the page in the page table of the process.
 * @param to_store The page (values) to be stored in memory
 */
void sim_mem::store_page_in_memory(int process, int page_index, char * to_store) {
    if (free_memory.empty())
        swap_out(process);
    int index_store = free_memory.front();
    free_memory.pop();
    
    int p = page_table[process][page_index].P, d = page_table[process][page_index].D;
    set_page_info(process, page_index, 1, p, d, index_store, -1);
    for (int i = index_store; i < index_store + page_size; i++)
        main_memory[i] = to_store[i - index_store];
    queues[process].push(page_index);
}
/**
 * 
 * 
 * @param process the process number
 * @param page_index the index of the page in the page table
 * @param v valid bit
 * @param p page index
 * @param d dirty bit
 * @param fra frame index
 * @param swap_ind the index of the page in the swap file
 */
void sim_mem::set_page_info(int process, int page_index, int v, int p, int d, int fra, int swap_ind) {
    page_table[process][page_index].V = v;
    page_table[process][page_index].P = p;
    page_table[process][page_index].D = d;
    page_table[process][page_index].frame = fra;
    page_table[process][page_index].swap_index = swap_ind;
}
/**
 * It checks if the page is bss, stack or heap.
 * 
 * @param page_index The index of the page in the page table.
 */
bool sim_mem::is_bss_stack_heap(int page_index) {
    if (page_index > text_size + data_size)
        return true;
    return false;
}
/**
 * 
 * 
 * @param process the process number of the process that request swap out
 */
void sim_mem::swap_out(int process) {
    int to_swap_out = queues[process].front();
    queues[process].pop();
    int index_clean = page_table[process][to_swap_out].frame;
    if (page_table[process][to_swap_out].D == 0) { //page not dirty can be deleted
        for (int i = index_clean; i < index_clean + page_size; i++)
            main_memory[i] = '0';
        free_memory.push(index_clean);
        set_page_info(process, to_swap_out, 0, page_table[process][to_swap_out].P, page_table[process][to_swap_out].D, -1, -1);
        return;
    }
    if (free_swap_indexs.empty()) {
        perror("RAM and swap full");
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table[0]);
        if (this -> num_of_proc == 2) {
            close(this -> program_fd[1]);
            free(this -> page_table[1]);
        }
        free(this -> page_table);
        exit(1);
    }
    int ind_swap = free_swap_indexs.front();
    free_swap_indexs.pop();
    char * to_store_in_swap = (char * ) malloc(page_size * sizeof(char) + 1);
    if (to_store_in_swap == NULL) {
        perror("malloc");
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table[0]);
        if (this -> num_of_proc == 2) {
            close(this -> program_fd[1]);
            free(this -> page_table[1]);
        }
        free(this -> page_table);
        exit(1);
    }
    for (int i = index_clean; i < index_clean + page_size; i++)
        to_store_in_swap[i - index_clean] = main_memory[i];
    for (int i = index_clean; i < index_clean + page_size; i++)
        main_memory[i] = '0';
    to_store_in_swap[page_size] = '\0';
    replace_str_in_file(swapfile_fd, ind_swap, swap_size, to_store_in_swap);
    free(to_store_in_swap);
    set_page_info(process, to_swap_out, 0, 1, 1, -1, ind_swap);
    free_memory.push(index_clean);
}

/**
 * Prints the memory
 */
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for (i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}

/**
 * print the swap file
 */
void sim_mem::print_swap() {
    char * str = (char * ) malloc(this -> page_size * sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this -> page_size) == this -> page_size) {
        for (i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}
/**
 * Prints the page table.
 */
void sim_mem::print_page_table() {
    int i;
    for (int j = 0; j < num_of_proc; j++) {
        printf("\n page table of process: %d \n", j);
        printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
        for (i = 0; i < num_of_pages; i++) {
            printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
                page_table[j][i].V,
                page_table[j][i].D,
                page_table[j][i].P,
                page_table[j][i].frame,
                page_table[j][i].swap_index);
        }
    }
}
/**
 * Replace the chars at target-index in the file with the value string.
 * 
 * @param fd file descriptor
 * @param target the target address to be replaced
 * @param file_size the size of the file
 * @param value the string to be replace
 */
void sim_mem::replace_str_in_file(int fd, int target, int file_size, char * value) {
    char * file_str = (char * ) malloc(file_size * sizeof(char) + 1);
    if (file_str == NULL) {
        close(this -> swapfile_fd);
        close(this -> program_fd[0]);
        free(this -> page_table[0]);
        if (this -> num_of_proc == 2) {
            close(this -> program_fd[1]);
            free(this -> page_table[1]);
        }
        free(this -> page_table);
    }
    lseek(fd, 0, SEEK_SET); // go to the start of the file
    read(fd, file_str, file_size);
    file_str[file_size] = '\0';
    for (int i = target; i < target + strlen(value); i++)
        file_str[i] = value[i - target];
    lseek(fd, 0, SEEK_SET);
    write(fd, file_str, file_size);
    free(file_str);
}

#endif