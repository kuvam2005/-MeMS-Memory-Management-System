/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this
macro to make the output of all system same and conduct a fair evaluation.
*/
#define PAGE_SIZE 4096

struct sub_chain
{
    size_t size;
    int type;
    struct sub_chain *prev;
    struct sub_chain *next;
};

struct main_chain
{
    int pages;
    int vaddr;
    void *paddr;
    struct sub_chain *chain;
    struct main_chain *prev;
    struct main_chain *next;
};

struct main_chain *head;
int starting_addr;
void* nodes_array[100000];
int num_main_chain;
void* node_address=NULL;
int count_node_page=0;
int address_pointer=0;


/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_init()
{
    head = NULL;
    starting_addr = 1000;
    num_main_chain = 0;
}

/*
This function will be called at the end of the MeMS system and its main job is to unmap the
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void free_sub_chain(struct sub_chain *sub_chain)
{
    if (sub_chain == NULL)
    {
        return;
    }
    while(count_node_page<0){
        if (munmap(nodes_array[count_node_page], PAGE_SIZE)) { 
            perror("munmapfailed"); 
        }
        count_node_page--; 
    }
}

void free_main_chain(struct main_chain *head)
{
    if (head == NULL)
    {
        return;
    }
    free_main_chain(head->next);
    if(munmap(head->paddr, PAGE_SIZE * head->pages)){
        perror("munmapfailed"); 
    }
    // free_sub_chain(head->chain);
    // munmap(head, sizeof(struct main_chain));
}

void mems_finish()
{
    free_main_chain(head);
}

/*
Allocates memory of the specified size by reusing a segment from the free list if
a sufficiently large segment is available.

Else, uses the mmap system call to allocate more memory on the heap and updates
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/

void split_sub_chain(struct sub_chain *chain, size_t size)
{
    if (size != 0)
    {
        struct sub_chain *next = chain->next;
        chain->next = node_address;
        node_address+=sizeof(struct sub_chain);
        address_pointer+=sizeof(struct sub_chain);
        if(address_pointer+sizeof(struct sub_chain)>4096){
            node_address=mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
            if (node_address == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }
            nodes_array[count_node_page]=node_address;
            count_node_page++;
        }
        chain->next->size = size;
        if (next)
        {
            next->prev = chain->next;
        }
        chain->next->prev = chain;
        chain->next->next = next;
    }
}

struct main_chain *allocate_main_chain(size_t size)
{
    struct main_chain *new_element;
    new_element = node_address;
    node_address+=sizeof(struct main_chain);
    address_pointer+=sizeof(struct main_chain);
    if(address_pointer+sizeof(struct main_chain)>4096){
        node_address=mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
        if (node_address == MAP_FAILED)
        {
            perror("mmap");
            exit(1);
        }
        nodes_array[count_node_page]=node_address;
        count_node_page++;
    }
    int num_pages = (size % PAGE_SIZE) == 0 ? size / PAGE_SIZE : ((size / PAGE_SIZE) + 1);
    new_element->pages = num_pages;
    void *paddr = mmap(NULL, num_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
        if (paddr == MAP_FAILED)
        {
            perror("mmap");
            exit(1);
        }
    new_element->chain = node_address;
    node_address+=sizeof(struct sub_chain);
        address_pointer+=sizeof(struct sub_chain);
        if(address_pointer+sizeof(struct sub_chain)>4096){
            node_address=mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
            if (node_address == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }
            nodes_array[count_node_page]=node_address;
            count_node_page++;
        }
    new_element->chain->size = num_pages * PAGE_SIZE;
    new_element->paddr = paddr;

    if (head == NULL)
    {
        new_element->vaddr = starting_addr;
        head = new_element;
        return new_element;
    }

    struct main_chain *current;
    for (current = head; current->next; current = current->next);
    current->next = new_element;
    new_element->prev = current;
    new_element->vaddr = current->vaddr + (current->pages * PAGE_SIZE);
    return head;
}

void *get_address(size_t size)
{
    size_t temp_size;
    for (struct main_chain *current = head; current; current = current->next)
    {
        temp_size = 0;
        for (struct sub_chain *sub = current->chain; sub; sub = sub->next)
        {
            if ((sub->type == 0) && (sub->size >= size))
            {
                split_sub_chain(sub, sub->size - size);
                sub->size = size;
                sub->type = 1;
                return (void *)(current->vaddr + temp_size);
            }
            temp_size += sub->size;
        }
    }
    return NULL;
}

void *mems_malloc(size_t size)
{

    if(node_address==NULL){
        node_address=mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
        nodes_array[count_node_page]=node_address;
        count_node_page++;
    }
    void *temp_addr = get_address(size);
    if (temp_addr)
    {
        return temp_addr;
    }

    head = allocate_main_chain(size);
    num_main_chain++;

    temp_addr = get_address(size);
    if (temp_addr == NULL)
    {
        printf("should not happen\n");
    }
    return temp_addr;
}

/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats()
{
    int pages = 0;
    int unused_memory = 0;
    int *lengths = (int*) mmap(NULL, num_main_chain * sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    int lengths_count = 0;

    printf("----- MeMS SYSTEM STATS ----\n");
    for (struct main_chain *current = head; current; current = current->next)
    {
        pages += current->pages;
        printf("MAIN[%d:%d]-> ", current->vaddr, current->vaddr + (current->pages * PAGE_SIZE) - 1);
        int i = 0;
        int sizes = 0;
        for (struct sub_chain *sub = current->chain; sub; sub = sub->next)
        {
            i++;
            if (sub->type == 0)
            {
                unused_memory += sub->size;
            }
            printf("%c[%d:%ld] <-> ", sub->type ? 'P' : 'H', current->vaddr + sizes, current->vaddr + sizes + sub->size - 1);
            sizes += sub->size;
        }
        lengths[lengths_count++] = i;
        printf("NULL\n");
    }
    printf("Pages Used:     %d\n", pages);
    printf("Space unused:   %d\n", unused_memory);
    printf("Main Chain Length:      %d\n", num_main_chain);
    printf("Sub-Chain Length nodes_arrayay: [");
    for (int i = 0; i < lengths_count; i++) {
        printf("%d, ", lengths[i]);
    }
    printf("]\n");
    munmap(lengths, num_main_chain * sizeof(int));
    printf("-----------------------------\n");
}

/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void *v_ptr)
{
    struct main_chain *current;
    for (current = head; (current) && ((current->vaddr + (current->pages * PAGE_SIZE)) <= (size_t)v_ptr); current = current->next)
        ;
    if (current == NULL)
    {
        printf("address not found\n");
        return NULL;
    }
    return (void *)(current->paddr + ((size_t)v_ptr - current->vaddr));
}

/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: nothing
*/

void mems_free(void *v_ptr)
{
    size_t v_addr = (size_t)v_ptr;
    struct main_chain *current;
    for (current = head; (current) && ((current->vaddr + (current->pages * PAGE_SIZE)) <= (size_t)v_ptr); current = current->next)
        ;
    if (current == NULL)
    {
        printf("address not found\n");
        return;
    }
    struct sub_chain *sub;
    size_t size_sum = 0;
    for (sub = current->chain; (sub) && (sub->size + size_sum + current->vaddr <= (size_t)v_ptr); sub = sub->next)
        size_sum += sub->size;
    if (sub->type == 0)
    {
        printf("unallocated\n");
        return;
    }
    sub->type = 0;
    struct sub_chain *next;
    for (sub = current->chain; sub; sub = sub->next)
    {
        if (sub->type == 0)
        {
            while ((sub->next) && (sub->next->type == 0))
            {
                next = sub->next;
                sub->size += sub->next->size;
                sub->next = next->next;
                if (next->next)
                {
                    next->next->prev = sub;
                }
                munmap(next, sizeof(struct sub_chain));
            }
        }
    }
}