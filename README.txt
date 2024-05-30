memory simulator.

--------------important note--------------
--- dont forget to include sim_mem.cpp ---
------------------------------------------
input: 
 define the input in the main:
 * init with sim_mem, constructor
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
accepted symbol: '+'(addition), '^'(power), digits(integer only), ','(to delimit polynom from x), x(the desired value) 

 * load function:
 * 
 * @param process_id The process id of the process that is requesting the memory access.
 * @param address The address of the memory location to be loaded.

 * store function:
 * 
 * @param process_id The process that is requesting the store.
 * @param address The address to store the value at.
 * @param value The value to be stored in memory


output: 
	page table map
	main memory
	swap file

files:
	main.cpp
	sim_mem.cpp
	sim_mem.h
	[swap file] (created if not exists)	
compilation: 
		g++ main.cpp -o ex4a main
run: 
		./main

by Elya Athlan.