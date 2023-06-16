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
#include "sysemu/kvm.h"
#include "sysemu/kvm_int.h"
#include "sysemu/runstate.h"
#include "sysemu/cpus.h"
#include "qemu/guest-random.h"
#include "qapi/error.h"

#include <linux/kvm.h>
#include "kvm-cpus.h"

#include <time.h>

uint8_t bitmap[65536];
extern uint8_t *current_intel_coverage;
extern uint8_t *current_kvm_coverage;
// extern struct kcov_t kcov_list[0xfffff];
extern pthread_key_t resource_key;
static void *kvm_vcpu_thread_fn(void *arg)
{
    CPUState *cpu = arg;
    int r;

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

    int kcov_fd;
    unsigned long * kcov_cover;

    kcov_fd = open("/sys/kernel/debug/kcov", O_RDWR);
    if (kcov_fd == -1)
        perror("open"), exit(1);


    /* Setup trace mode and trace size. */
    if (ioctl(kcov_fd, KCOV_INIT_TRACE, COVER_SIZE)){
            DEBUG_PRINT("ioctl\n");
            perror("ioctl"), exit(1);
        }

        /* Mmap buffer shared between kernel- and user-space. */
    kcov_cover = (unsigned long *)mmap(NULL, COVER_SIZE * sizeof(unsigned long),PROT_READ | PROT_WRITE, MAP_SHARED, kcov_fd, 0);
    if ((void *)kcov_cover == MAP_FAILED)
        perror("mmap"), exit(1);

    kcov_t *resource = (kcov_t *) malloc(sizeof(kcov_t));
    resource->enable = 1;
    resource->kcov_fd = kcov_fd;
    resource->kcov_cover = kcov_cover;
    pthread_setspecific(resource_key, resource);

    // kcov_list[tid].enable = 1;
    // kcov_list[tid].kcov_fd = kcov_fd;
    // kcov_list[tid].kcov_cover = kcov_cover;

    do {
        if (cpu_can_run(cpu)) {
            r = afl_shm_get_cov_kvm_cpu_exec(cpu);
            if (r == EXCP_DEBUG) {
                cpu_handle_guest_debug(cpu);
            }
        }
        qemu_wait_io_event(cpu);
    } while (!cpu->unplug || cpu_can_run(cpu));


    kvm_destroy_vcpu(cpu);
    cpu_thread_signal_destroyed(cpu);
    qemu_mutex_unlock_iothread();
    rcu_unregister_thread();

    if (munmap(kcov_cover, COVER_SIZE * sizeof(unsigned long)))
        perror("munmap"), exit(1);
    if (close(kcov_fd))
        perror("close"), exit(1);
    resource->enable = 0;
    free(resource);

    // printf("%f\n",(double)(end_time-start_time)/CLOCKS_PER_SEC);
    return NULL;
}

static void kvm_start_vcpu_thread(CPUState *cpu)
{
    char thread_name[VCPU_THREAD_NAME_SIZE];

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

#ifdef KVM_CAP_SET_GUEST_DEBUG
    ops->supports_guest_debug = kvm_supports_guest_debug;
    ops->insert_breakpoint = kvm_insert_breakpoint;
    ops->remove_breakpoint = kvm_remove_breakpoint;
    ops->remove_all_breakpoints = kvm_remove_all_breakpoints;
#endif
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
