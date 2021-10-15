所有线程只在一个CPU上运行，是不需要加锁的。可通过taskset -c <cpu_id> ./a.out验证。


