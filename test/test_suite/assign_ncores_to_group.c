#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define GRR_DEFAULT 0
#define GRR_PERFORMANCE 1
#define __NR_sched_assign_ncores_to_group 467

int main(int argc, char *argv[])
{
	int ret;
	int ncores = 0;
	char *group = 0;
	int groupNum = 0;

	if (argc != 3) {
		fprintf(stderr, "<%s> <ncores> <group>\n", *argv);
		return 0;
	}

	ncores = atoi(argv[1]);
	group = argv[2];
	if (group[0] == 'd' || group[0] == 'D') {
		groupNum = GRR_DEFAULT;
	} else if (group[0] == 'p' || group[0] == 'P') {
		groupNum = GRR_PERFORMANCE;
	} else {
		printf("wrong input: %s\n", group);
		return 0;
	}

	ret = syscall(__NR_sched_assign_ncores_to_group, ncores, groupNum);
	if (ret < 0)
		perror("syscall");
	
	return ret;
}
