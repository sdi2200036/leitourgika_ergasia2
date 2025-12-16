#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define GRR_DEFAULT 0
#define GRR_PERFORMANCE	1
#define __NR_sched_assign_processes_to_group 468

int main(int argc, char *argv[])
{
	int groupNum, ret;
	pid_t pid;

	if (argc != 3) {
		fprintf(stderr, "%s: <pid> <group>\n", *argv);
		return -1;
	}

	pid = atoi(argv[1]);
	switch (argv[2][0]) {
		case 'd':
		case 'D':
			groupNum = GRR_DEFAULT;
			break;
		case 'p':
		case 'P':
			groupNum = GRR_PERFORMANCE;
			break;
		default:
			printf("Group \"%s\" does not exist\n", argv[2]);
			break;
	}

	ret = syscall(__NR_sched_assign_processes_to_group, pid, groupNum);
	if (ret < 0)
		perror("assign_process_to_group");

	return ret;
}

