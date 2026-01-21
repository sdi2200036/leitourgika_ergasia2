#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define REPS (1ULL << 31)
#define GRR_DEFAULT 1
#define GRR_PERFORMANCE 2
#define __NR_assign_process_to_group 468

pthread_mutex_t start_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
int start = 0;

void *thread_fn(void *data)
{
	pthread_mutex_lock(&start_mtx);
	while (start == 0)
		pthread_cond_wait(&start_cond, &start_mtx);
	
	pthread_mutex_unlock(&start_mtx);

	struct timespec start, end;
	double a = 1.13;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (uint64_t i = 0; i < REPS; i++)
		a = a * 1.005;
	clock_gettime(CLOCK_MONOTONIC, &end);

	a = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

	printf("Time: %.3fs\n", a);
	return NULL;
}

int main(int argc, char *argv[])
{
	int threads, groupNum;
	pthread_t *t_arr;

	if (argc < 2) {
		printf("Not enough arguments\n");
		return 1;
	}

	switch (argv[1][0]) {
		case 'p':
		case 'P':
			groupNum = GRR_PERFORMANCE;
			break;
		case 'd':
		case 'D':
			groupNum = GRR_DEFAULT;
			break;
		default:
			printf("Group \"%s\" does not exist\n", argv[0]);
			return 1;
	}

	threads = atoi(argv[2]);
	t_arr = malloc(sizeof(pthread_t) * threads);
	if (t_arr == NULL) {
		perror("malloc");
		return 1;
	}

	for (int i = 0; i < threads; i++)
		pthread_create(t_arr + i, NULL, thread_fn, NULL);

	if (syscall(__NR_assign_process_to_group, getpid(), groupNum)) {
		perror("syscall");
	}

	start = 1;
	pthread_cond_broadcast(&start_cond);

	for (int i = 0; i < threads; i++)
		pthread_join(t_arr[i], NULL);

	free(t_arr);
	return 0;
}
