#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#define SYS_ASSIGN_NCORES 467
#define GRR_DEFAULT 1

int main() {
    printf("Testing Syscall 467...\n");
    long ret = syscall(SYS_ASSIGN_NCORES, 1, GRR_DEFAULT);

    if (ret == 0) {
        printf("SUCCESS! Syscall executed correctly.\n");
    } else {
        perror("FAIL");
        printf("Error code: %d\n", errno);
    }
    return 0;
}
