#include "sim_mem.h"

#include "sim_mem.cpp"

int main() {
    char * e1 = "exec_file";
    char * e2 = "";
    char * s = "swap_file";
    sim_mem mem_sm(e1, e2, s, 5, 50, 25, 25, 25, 5, 1);

    mem_sm.store(1, 9, 'X');
    mem_sm.store(1, 10, 'X');
    mem_sm.load(1, 1);
    mem_sm.store(1, 8, 'X');
    mem_sm.print_memory();
    mem_sm.print_swap();
    mem_sm.print_page_table();

}