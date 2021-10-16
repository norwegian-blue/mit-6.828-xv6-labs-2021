#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {

    int nticks;
    
    if (argc != 2) {
        fprintf(2, "usage: sleep nticks\n");
    }

    nticks = atoi(argv[1]);
    sleep(nticks);

    exit(0);
}
