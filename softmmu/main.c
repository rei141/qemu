/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2020 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu-main.h"
#include "sysemu/sysemu.h"
#include "../accel/kvm/kvm-cpus.h"
#ifdef CONFIG_SDL
#include <SDL.h>
#endif
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
// #define KCOV_INIT_TRACE _IOR('c', 1, unsigned long)
// #define KCOV_ENABLE _IO('c', 100)
// #define KCOV_DISABLE _IO('c', 101)
// #define COVER_SIZE (64 << 14)

// #define KCOV_TRACE_PC 0
// #define KCOV_TRACE_CMP 1
unsigned long kvm_intel_base;
unsigned long kvm_base;
int kcov_fd;
unsigned long * kcov_cover;
uint8_t total_coverage[MAX_KVM_INTEL];
uint8_t kvm_coverage[MAX_KVM];
int qemu_main(int argc, char **argv, char **envp)
{
    FILE * fkvm_intel = fopen("/sys/module/kvm_intel/sections/.text","r");
    if (fkvm_intel == NULL)
        perror("fopen"), exit(1);

    FILE * fkvm = fopen("/sys/module/kvm/sections/.text","r");
    if (fkvm == NULL)
        perror("fopen"), exit(1);

    // start point of .text of kvm/kvm-intel
    char kvm_intel_str[18];
    char kvm_str[18];
    // int count = 0;
    int n = fread(kvm_intel_str, sizeof(char),18,fkvm_intel);
    if(n != 18)
        perror("fread"), exit(1);
    kvm_intel_base = strtoul(kvm_intel_str, NULL,0);
    // fprintf(kvm_intel_coverage_file, "0x%lx\n", kvm_intel_base);

    n = fread(kvm_str, sizeof(char),18,fkvm);
    if(n != 18)
        perror("fread"), exit(1);
    kvm_base = strtoul(kvm_str, NULL,0);
    // fprintf(kvm_coverage_file, "0x%lx\n", kvm_base);

    if (fclose(fkvm_intel) == EOF)
        perror("fclose"), exit(1);
    if (fclose(fkvm) == EOF)
        perror("fclose"), exit(1);

    kcov_fd = open("/sys/kernel/debug/kcov", O_RDWR);
    if (kcov_fd == -1)
        perror("open"), exit(1);
    /* Setup trace mode and trace size. */
    if (ioctl(kcov_fd, KCOV_INIT_TRACE, COVER_SIZE))
        perror("ioctl"), exit(1);
    FILE * total_cov_file;
    // int n;

        /* Mmap buffer shared between kernel- and user-space. */
    kcov_cover = (unsigned long *)mmap(NULL, COVER_SIZE * sizeof(unsigned long),PROT_READ | PROT_WRITE, MAP_SHARED, kcov_fd, 0);
    // printf("hello  COVER_SIZE %p\n", kcov_cover);
    if ((void *)kcov_cover == MAP_FAILED)
        perror("mmap"), exit(1);

    if ((total_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/total_kvm_intel_coverage","rb")) != NULL){
        // memset(total_coverage, 0, sizeof(total_coverage));
        int n = fread(total_coverage, sizeof(uint8_t), MAX_KVM_INTEL, total_cov_file);
        if (n) {
            fclose(total_cov_file);
        }
        else {
            perror("fread error");
        }
    }
    FILE * kvm_cov_file;
    if ((kvm_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/total_kvm_coverage","rb")) != NULL){
        // memset(kvm_coverage, 0, sizeof(total_coverage));
        int n = fread(kvm_coverage, sizeof(uint8_t), MAX_KVM, kvm_cov_file);
        if (n) {
            fclose(kvm_cov_file);
        }
        else {
            perror("fread error");
        }
    }
    pid_t pid = getpid();
    FILE *f_pid;
    if ((f_pid = fopen("/home/ishii/nestedFuzz/VMXbench/qemu_pid","w")) != NULL){
        // memset(kvm_coverage, 0, sizeof(total_coverage));
        // int n = fread(kvm_coverage, sizeof(uint8_t), MAX_KVM, kvm_cov_file);
        fprintf(f_pid,"%d", pid);
        fclose(f_pid);

    }
    qemu_init(argc, argv, envp);
    qemu_main_loop();
    qemu_cleanup();

    return 0;
}

#ifndef CONFIG_COCOA
int main(int argc, char **argv)
{
    return qemu_main(argc, argv, NULL);
}
#endif
