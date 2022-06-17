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
#define COVER_SIZE (64 << 12)

#define KCOV_TRACE_PC 0
#define KCOV_TRACE_CMP 1

extern int kcov_fd;
extern unsigned long * kcov_cover;
extern FILE * kvm_intel_coverage_file;
extern FILE * kvm_coverage_file;
extern unsigned long kvm_intel_base;
extern unsigned long kvm_base;

int kvm_init_vcpu(CPUState *cpu, Error **errp);
int kvm_cpu_exec(CPUState *cpu);
uint16_t hash_int_to_16b(int val);
int get_cov_kvm_cpu_exec(CPUState *cpu);
int afl_shm_get_cov_kvm_cpu_exec(CPUState *cpu,const char *afl_shm_id_str);
void kvm_destroy_vcpu(CPUState *cpu);
void kvm_cpu_synchronize_post_reset(CPUState *cpu);
void kvm_cpu_synchronize_post_init(CPUState *cpu);
void kvm_cpu_synchronize_pre_loadvm(CPUState *cpu);

#endif /* KVM_CPUS_H */
