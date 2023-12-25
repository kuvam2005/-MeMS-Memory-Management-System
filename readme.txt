In this assignment, we have made 3 helper functions for Mems_malloc() function namely -: void split_sub_chain(struct sub_chain *chain, size_t size),struct main_chain *allocate_main_chain(size_t size),void *get_address(size_t size)
first we check if there is any node who is hole and have size greater than inputed size then we create a node process and allocate memory using split_sub_chain() method
if it return null then we create a main node using allocate_main_chain() method
we started mapping VA from 1000 
then in mems_init(), we are initialising these-:struct main_chain *head,int starting_addr,int num_main_chain,
in mems_free() we convert process type node into hole and also combines to consecutive holes into one hole
in mems_finish(), we are deallocating all the memory allocated
in mems_print_stat(), we are printing each detail of nodes as we are instructed 
in mems_get() we are successfully returning PA mapped to VA.