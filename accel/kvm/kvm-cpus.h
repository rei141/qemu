/*
 * Accelerator CPUS Interface
 *
 * Copyright 2020 SUSE LLC
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef KVM_CPUS_H
#define KVM_CPUS_H

#include "sysemu/cpus.h"
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
#define COVER_SIZE (64 << 14)

#define KCOV_TRACE_PC 0
#define KCOV_TRACE_CMP 1


extern unsigned long kvm_intel_base;
extern unsigned long kvm_base;

#define MAX_KVM_INTEL 0xc7000
#define MAX_KVM 0x1b2000

typedef struct {
    int enable;
    int kcov_fd;
    unsigned long *kcov_cover;
} kcov_t;

int kvm_init_vcpu(CPUState *cpu, Error **errp);
int kvm_cpu_exec(CPUState *cpu);
uint16_t hash_int_to_16b(int val);
int get_cov_kvm_cpu_exec(CPUState *cpu);
int afl_shm_get_cov_kvm_cpu_exec(CPUState *cpu);
void kvm_destroy_vcpu(CPUState *cpu);
void kvm_cpu_synchronize_post_reset(CPUState *cpu);
void kvm_cpu_synchronize_post_init(CPUState *cpu);
void kvm_cpu_synchronize_pre_loadvm(CPUState *cpu);
bool kvm_supports_guest_debug(void);
int kvm_insert_breakpoint(CPUState *cpu, int type, hwaddr addr, hwaddr len);
int kvm_remove_breakpoint(CPUState *cpu, int type, hwaddr addr, hwaddr len);
void kvm_remove_all_breakpoints(CPUState *cpu);

#endif /* KVM_CPUS_H */
