#include <linux/module.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/delay.h>

//
// ia32/linux/memory.cpp
//

uint64_t ia32_detail_pa_from_va(void* va)
{
    return __pa(va);
}

void* ia32_detail_va_from_pa(uint64_t pa)
{
    return __va(pa);
}

uint64_t ia32_detail_check_physical_memory(void* range_list, int range_list_size, int* count)
{
    return 0;
}

//
// lib/linux/mm.cpp
//

void* memory_manager_detail_system_allocate(size_t size)
{
    return kmalloc(size, GFP_KERNEL | GFP_ATOMIC | __GFP_ZERO);
}

void memory_manager_detail_system_free(void* address)
{
    kfree(address);
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
    int cpu;
    for_each_online_cpu(cpu)
    {
        smp_call_function_single(cpu, callback, context, true);
    }
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
