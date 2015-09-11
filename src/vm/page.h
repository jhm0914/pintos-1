#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <hash.h>
#include "threads/thread.h"
#include "threads/vaddr.h"

#define VM_BIN 0
#define VM_FILE 1
#define VM_ANON 2

struct vm_entry
{
	uint8_t type;			// VM_BIN, VM_FILE, VM_ANON
	void *vaddr;			// virtual address that is operated by vm_entry
	bool writable;			// write flag
	bool is_loaded;			// flag that inform whether loaded to physical memory
	bool pinned;
	struct file *file;		// file mapped with vaddr
	struct list_elem mmap_elem;	// mmap list element
	size_t offset;			// offset to read in file
	size_t read_bytes;		// read_bytes (often it's 4KB)
	size_t zero_bytes;		// zero_bytes (often it's 0)
	size_t swap_slot;
	struct hash_elem elem;
};

void vm_init (struct hash *vm);
void vm_destroy (struct hash *vm);
static unsigned vm_hash_func (const struct hash_elem *e, void *aux);
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);
static void vm_destroy_func (struct hash_elem *e, void *aux);
struct vm_entry *find_vme (void *vaddr);
bool insert_vme (struct hash *vm, struct vm_entry *vme);
bool delete_vme (struct hash *vm, struct vm_entry *vme);

#endif
