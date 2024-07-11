#include <metal/shmem.h>

int metal_shmem_open(const char *name, size_t size,
		     struct metal_io_region **io)
{
	return metal_shmem_open_generic(name, size, io);
}
