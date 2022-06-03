/*
 * QEMU KVM support
 *
 * Copyright IBM, Corp. 2008
 *           Red Hat, Inc. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *  Glauber Costa     <gcosta@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/main-loop.h"
#include "sysemu/kvm_int.h"
#include "sysemu/runstate.h"
#include "sysemu/cpus.h"
#include "qemu/guest-random.h"
#include "qapi/error.h"

#include "kvm-cpus.h"
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define KCOV_INIT_TRACE _IOR('c', 1, unsigned long)
#define KCOV_ENABLE _IO('c', 100)
#define KCOV_DISABLE _IO('c', 101)
#define COVER_SIZE (64 << 20)

#define KCOV_TRACE_PC 0
#define KCOV_TRACE_CMP 1

static void *kvm_vcpu_thread_fn(void *arg)
{
    CPUState *cpu = arg;
    int r;
    int kcov_fd;
    unsigned long *kcov_cover, kcov_n;
    FILE *coverage_file;
    FILE *wcoverage_file;

    rcu_register_thread();

    qemu_mutex_lock_iothread();
    qemu_thread_get_self(cpu->thread);
    cpu->thread_id = qemu_get_thread_id();
    cpu->can_do_io = 1;
    current_cpu = cpu;

    r = kvm_init_vcpu(cpu, &error_fatal);
    kvm_init_cpu_signals(cpu);

    /* signal CPU creation */
    cpu_thread_signal_created(cpu);
    qemu_guest_random_seed_thread_part2(cpu->random_seed);

    printf("percpu start\n");
    kcov_fd = open("/sys/kernel/debug/kcov", O_RDWR);
    if (kcov_fd == -1)
        perror("open"), exit(1);
    coverage_file = fopen("/home/ishii/work/VMXbench/coverage.bin", "wb");
    if (coverage_file == NULL)
        perror("fopen"), exit(1);
    wcoverage_file = fopen("/home/ishii/work/VMXbench/wcoverage", "w");
    if (coverage_file == NULL)
        perror("fopen"), exit(1);
    /* Setup trace mode and trace size. */
    if (ioctl(kcov_fd, KCOV_INIT_TRACE, COVER_SIZE))
        perror("ioctl"), exit(1);
    /* Mmap buffer shared between kernel- and user-space. */
    kcov_cover = (unsigned long *)mmap(NULL, COVER_SIZE * sizeof(unsigned long),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, kcov_fd, 0);
    if ((void *)kcov_cover == MAP_FAILED)
        perror("mmap"), exit(1);

    do {
        /* Enable coverage collection on the current thread. */
        if (ioctl(kcov_fd, KCOV_ENABLE, KCOV_TRACE_PC))
            perror("ioctl"), exit(1);
        /* Reset coverage from the tail of the ioctl() call. */
        __atomic_store_n(&kcov_cover[0], 0, __ATOMIC_RELAXED);

        if (cpu_can_run(cpu)) {
            r = kvm_cpu_exec(cpu);
            if (r == EXCP_DEBUG) {
                cpu_handle_guest_debug(cpu);
            }
        }
        qemu_wait_io_event(cpu);

        kcov_n = __atomic_load_n(&kcov_cover[0], __ATOMIC_RELAXED);
        if (fwrite(kcov_cover, sizeof(unsigned long), kcov_n, coverage_file) != kcov_n)
            perror("fwrite"), exit(1);
        for (unsigned long i = 0; i<kcov_n; i++)
            fprintf(wcoverage_file,"0x%lx\n",kcov_cover[i+1]);
        /* Disable coverage collection for the current thread. After this call
        * coverage can be enabled for a different thread.
        */
        if (ioctl(kcov_fd, KCOV_DISABLE, 0))
            perror("ioctl"), exit(1);

    } while (!cpu->unplug || cpu_can_run(cpu));

    if (munmap(kcov_cover, COVER_SIZE * sizeof(unsigned long)))
        perror("munmap"), exit(1);
    if (close(kcov_fd))
        perror("close"), exit(1);
    if (fclose(coverage_file) == EOF)
        perror("fclose"), exit(1);
    if (fclose(wcoverage_file) == EOF)
        perror("fclose"), exit(1);
    kvm_destroy_vcpu(cpu);
    cpu_thread_signal_destroyed(cpu);
    qemu_mutex_unlock_iothread();
    rcu_unregister_thread();
    return NULL;
}

static void kvm_start_vcpu_thread(CPUState *cpu)
{
    char thread_name[VCPU_THREAD_NAME_SIZE];
    FILE * tmp;
    tmp = fopen("/home/ishii/work/VMXbench/tmp","a");
    fprintf(tmp,"1\n");

    cpu->thread = g_malloc0(sizeof(QemuThread));
    cpu->halt_cond = g_malloc0(sizeof(QemuCond));
    qemu_cond_init(cpu->halt_cond);
    snprintf(thread_name, VCPU_THREAD_NAME_SIZE, "CPU %d/KVM",
             cpu->cpu_index);
    qemu_thread_create(cpu->thread, thread_name, kvm_vcpu_thread_fn,
                       cpu, QEMU_THREAD_JOINABLE);
}

static bool kvm_vcpu_thread_is_idle(CPUState *cpu)
{
    return !kvm_halt_in_kernel();
}

static bool kvm_cpus_are_resettable(void)
{
    return !kvm_enabled() || kvm_cpu_check_are_resettable();
}

static void kvm_accel_ops_class_init(ObjectClass *oc, void *data)
{
    AccelOpsClass *ops = ACCEL_OPS_CLASS(oc);

    ops->create_vcpu_thread = kvm_start_vcpu_thread;
    ops->cpu_thread_is_idle = kvm_vcpu_thread_is_idle;
    ops->cpus_are_resettable = kvm_cpus_are_resettable;
    ops->synchronize_post_reset = kvm_cpu_synchronize_post_reset;
    ops->synchronize_post_init = kvm_cpu_synchronize_post_init;
    ops->synchronize_state = kvm_cpu_synchronize_state;
    ops->synchronize_pre_loadvm = kvm_cpu_synchronize_pre_loadvm;
}

static const TypeInfo kvm_accel_ops_type = {
    .name = ACCEL_OPS_NAME("kvm"),

    .parent = TYPE_ACCEL_OPS,
    .class_init = kvm_accel_ops_class_init,
    .abstract = true,
};

static void kvm_accel_ops_register_types(void)
{
    type_register_static(&kvm_accel_ops_type);
}
type_init(kvm_accel_ops_register_types);
