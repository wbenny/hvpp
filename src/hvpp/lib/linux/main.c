#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/mm.h>

//
// ia32/linux/memory.cpp
//

uint64_t ia32_detail_pa_from_va(void* va)
{
    return PFN_PHYS(vmalloc_to_pfn(va));
}

void* ia32_detail_va_from_pa(uint64_t pa)
{
    return __va(pa);
}

struct memory_range
{
    uint64_t begin;
    uint64_t end;
};

#define MAX_RANGES 32

extern struct resource iomem_resource;

static void iter_resource(struct memory_range *ranges,
			  struct resource *resource,
			  const char *match,
			  int *curr)
{
	struct resource *tmp;
	if (*curr >= MAX_RANGES)
		return;

	for (tmp = resource; tmp && *curr < MAX_RANGES; tmp = tmp->child) {
		if (strcmp(tmp->name, match) == 0) {
			ranges[*curr].begin = tmp->start;
			ranges[*curr].end = tmp->end;
			++*curr;
		}

		if (tmp->sibling)
			iter_resource(ranges, tmp->sibling, match, curr);
	}
}

int mm_cache_ram_ranges(struct memory_range *ranges, int *range_count)
{
	iter_resource(ranges, &iomem_resource, "System RAM", range_count);
	return 0;
}

uint64_t ia32_detail_check_physical_memory(struct memory_range* range_list, int range_list_size, int* count)
{
    mm_cache_ram_ranges(range_list, count);
    return 0;
}

//
// lib/linux/mm.cpp
//

#define PRIx64       "llx"

void* memory_manager_detail_system_allocate(size_t size)
{
    return vmalloc(size);
}

void memory_manager_detail_system_free(void* address)
{
    vfree(address);
}

//
// lib/linux/mp.cpp
//

uint32_t mp_detail_cpu_count(void)
{
    return num_online_cpus();
}

uint32_t mp_detail_cpu_index(void)
{
    return smp_processor_id();
}

void mp_detail_sleep(uint32_t milliseconds)
{
    msleep(milliseconds);
}

void mp_detail_ipi_call(void(*callback)(void*), void* context)
{
    #if 0
    int cpu;
    for_each_online_cpu(cpu)
    {
        smp_call_function_single(cpu, callback, context, true);
    }
    #else
    callback(context);
    #endif
}

//
// lib/linux/log.cpp
//

void logger_detail_do_print(const char* message)
{
    printk(KERN_INFO "%s\n", message);
}

void logger_detail_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    vsnprintf(buf, size, fmt, args);
}

//
// lib/linux/driver.cpp
//

extern void driver_entry(void);
extern void driver_unload(void);

//
// Main entry-point.
//

static int __init hvpp_start(void)
{
    driver_entry();
    return 0;
}

static void __exit hvpp_cleanup(void)
{
    driver_unload();
}

module_init(hvpp_start);
module_exit(hvpp_cleanup);

MODULE_AUTHOR("Petr Benes");
MODULE_LICENSE("MIT");
