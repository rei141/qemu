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

// #define KCOV_TRACE_CMP 1


unsigned long kcov_n;


int cflag;
int wflag;
int kflag;



// unsigned int * kcov_intel_cover;

// char kvm_intel_coverd[MAX_KVM_INTEL];
// char kvm_coverd[MAX_KVM];

uint8_t bitmap[65536];
extern uint8_t total_coverage[MAX_KVM_INTEL];
extern uint8_t kvm_coverage[MAX_KVM];
extern uint8_t current_intel_coverage[MAX_KVM_INTEL];
extern uint8_t current_kvm_coverage[MAX_KVM];

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
    // printf("hello\n");
    r = kvm_init_vcpu(cpu, &error_fatal);
        

    kvm_init_cpu_signals(cpu);

    /* signal CPU creation */
    cpu_thread_signal_created(cpu);
    qemu_guest_random_seed_thread_part2(cpu->random_seed);


    // clock_t start_time, end_time;
    // start_time = clock();

    // printf("percpu start\n");

    // kcov_intel_cover = (unsigned int *)malloc(COVER_SIZE * sizeof(unsigned int));
    // if (kcov_intel_cover == NULL)
    //     perror("malloc"), exit(1);
    // const char *afl_shm_id_str = getenv("__AFL_SHM_ID");
    // if (afl_shm_id_str != NULL) {
    //     FILE * fuga = fopen("fuga","w");
    //     fclose(fuga);
    // }

    // unsigned long kcov_n;
    // int cov;
    do {
        if (cpu_can_run(cpu)) {
            // printf("%d\n",++count);
            // if (afl_shm_id_str != NULL) {
                r = afl_shm_get_cov_kvm_cpu_exec(cpu);
                // r = kvm_cpu_exec(cpu);
            // }
            // else {
                // r = kvm_cpu_exec(cpu);
        // kcov_n = __atomic_load_n(&kcov_cover[0], __ATOMIC_RELAXED);
        // printf("hello %ld\n",kcov_n);
        // for (int i = 0; i < kcov_n; i++) {
        //     cov = (int)(kcov_cover[i+1]-kvm_intel_base);
        //     if (cov >= 0 && cov < MAX_KVM_INTEL){
        //         // cur_location = hash_int_to_16b(cov);
        //         // if(afl_area_ptr[(cur_location ^ prev_location)] != 255){
        //         // if(afl_area_ptr[(cur_location)] != 255){
        //         //     afl_area_ptr[(cur_location)]++;
        //         //     // afl_area_ptr[(cur_location ^ prev_location)]++;
        //         //     }
        //         // prev_location = cur_location >> 1;
        //         // if (kvm_intel_coverd[cov] == 0){
        //         //     kvm_intel_coverd[cov] = 1;
        //         //     fprintf(kvm_intel_coverage_file,"0x%x\n",cov);
        //         // }
        //         // if (wflag != 1 && total_coverage[cov] == 0){
        //         if (total_coverage[cov] == 0){
        //             total_coverage[cov] = 1;
        //             // wflag = 1;
        //         }
        //     }
        // }
        // struct timeval tv;
        // struct tm *tm;

        // gettimeofday(&tv, NULL);

        // tm = localtime(&tv.tv_sec);
        // char f_name[100];
        // sprintf(f_name,"record/n_intel_%02d_%02d_%02d_%02d_%02d_%06ld",tm->tm_mon+1, tm->tm_mday,
        //  tm->tm_hour, tm->tm_min, tm->tm_sec,tv.tv_usec);
        // FILE * record = fopen(f_name,"w");
        // fwrite(total_coverage,sizeof(uint8_t),MAX_KVM_INTEL,record);
        // fclose(record);
            // }
            if (r == EXCP_DEBUG) {
                cpu_handle_guest_debug(cpu);
            }
        }
    
    //         end_time=clock();
    // FILE * tmp = fopen("/home/ishii/nestedFuzz/VMXbench/tmp","a");
    // fprintf(tmp,"%f\n",(double)(end_time-start_time)/CLOCKS_PER_SEC);
    // start_time=clock();
        qemu_wait_io_event(cpu);
    } while (!cpu->unplug || cpu_can_run(cpu));
    // printf("hello3\n");
    // FILE * tmp = fopen("/home/ishii/work/VMXbench/hello","w");
    // fprintf(tmp,"hello\n");
    kvm_destroy_vcpu(cpu);
    cpu_thread_signal_destroyed(cpu);
    qemu_mutex_unlock_iothread();
    rcu_unregister_thread();
    // if (cflag != 0){
    //         FILE * current_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/record/current_intel_coverage","w");
    //         fwrite(current_intel_coverage,sizeof(uint8_t),MAX_KVM_INTEL,current_cov_file);
    //         fclose(current_cov_file);
            
    //         current_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/record/current_kvm_coverage","w");
    //         fwrite(current_kvm_coverage,sizeof(uint8_t),MAX_KVM,current_cov_file);
    //         fclose(current_cov_file);
    //         cflag = 0;
    //     }
    if (wflag != 0 ){
        FILE * total_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/total_kvm_intel_coverage","w");
        fwrite(total_coverage,sizeof(uint8_t),MAX_KVM_INTEL,total_cov_file);
        fclose(total_cov_file);
        
        // time_t型は基準年からの秒数
        // time_tのままでは使いにくい．time_tはtm構造体に相互に変換できる
        struct timeval tv;
        struct tm *tm;

        gettimeofday(&tv, NULL);

        tm = localtime(&tv.tv_sec);
        char f_name[100];
        sprintf(f_name,"/home/ishii/nestedFuzz/VMXbench/record/n_intel_%02d_%02d_%02d_%02d_%02d_%06ld",tm->tm_mon+1, tm->tm_mday,\
         tm->tm_hour, tm->tm_min, tm->tm_sec,tv.tv_usec);
        FILE * record = fopen(f_name,"w");
        fwrite(total_coverage,sizeof(uint8_t),MAX_KVM_INTEL,record);
        fclose(record);
        wflag=0;
    }
    if (kflag != 0 ){
        FILE * total_cov_file = fopen("/home/ishii/nestedFuzz/VMXbench/total_kvm_coverage","w");
        fwrite(kvm_coverage,sizeof(uint8_t),MAX_KVM,total_cov_file);
        fclose(total_cov_file);
        
        // time_t型は基準年からの秒数
        // time_tのままでは使いにくい．time_tはtm構造体に相互に変換できる
        struct timeval tv;
        struct tm *tm;

        gettimeofday(&tv, NULL);

        tm = localtime(&tv.tv_sec);
        char f_name[100];
        sprintf(f_name,"/home/ishii/nestedFuzz/VMXbench/record/n_kvm_%02d_%02d_%02d_%02d_%02d_%06ld",tm->tm_mon+1, tm->tm_mday,\
         tm->tm_hour, tm->tm_min, tm->tm_sec,tv.tv_usec);
        FILE * record = fopen(f_name,"w");
        fwrite(kvm_coverage,sizeof(uint8_t),MAX_KVM,record);
        fclose(record);
        kflag=0;
    }
    // if (ioctl(kcov_fd, KCOV_DISABLE, 0))
    //     perror("ioctl"), exit(1);
    if (munmap(kcov_cover, COVER_SIZE * sizeof(unsigned long)))
        perror("munmap"), exit(1);
    if (close(kcov_fd))
        perror("close"), exit(1);


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
