#include "vm/page.h"

void vm_init (struct hash *vm)
{
	hash_init(vm, vm_hash_func, vm_less_func, NULL);
}
void vm_destroy (struct hash *vm)
{
	hash_destroy(vm, vm_destroy_func);
}
static unsigned vm_hash_func (const struct hash_elem *e, void *aux)
{
	struct vm_entry *vm = hash_entry(e, struct vm_entry, elem);

	return hash_int(vm->vaddr);
}
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	return hash_entry(a, struct vm_entry, elem)->vaddr < hash_entry(b, struct vm_entry, elem)->vaddr;
}
static void vm_destroy_func (struct hash_elem *e, void *aux)
{
	struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);

	if (vme->is_loaded)
	{
		/*void *pte = pagedir_get_page(&thread_current()->pagedir, vme->vaddr);
		palloc_free_page(pte);*/
		palloc_free_page(pagedir_get_page(thread_current()->pagedir, vme->vaddr));
		pagedir_clear_page(&thread_current()->pagedir, vme->vaddr);
	}

	free(vme);
}
struct vm_entry *find_vme (void *vaddr)
{
	struct vm_entry vm;
	struct hash_elem *h;
	
	vm.vaddr = pg_round_down(vaddr);

	h = hash_find(&thread_current()->vm, &vm.elem);

	if (h == NULL)
	{
		return h;
	}

	return hash_entry(h, struct vm_entry, elem);
}
bool insert_vme (struct hash *vm, struct vm_entry *vme)
{
	return (hash_insert(vm, &vme->elem) == NULL) ? true : false;
}
bool delete_vme (struct hash *vm, struct vm_entry *vme)
{
	return (hash_delete(vm, &vme->elem) != NULL) ? true : false;
}
