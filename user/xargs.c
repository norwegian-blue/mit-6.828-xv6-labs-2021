#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
	
	char buf[512];
	char *p = buf;
	char *xargv[MAXARG];

	// Initialize xargv
	for (int i = 1; i < argc; i++) {
		xargv[i-1] = argv[i];
	}

	while(read(0, p++, sizeof(char)) == sizeof(char)) {
		// read until new line is found
		if (*(p-1) == '\n') {
			*(p-1) = 0;
			p = buf;
			
			xargv[argc-1] = buf;
			// fork to child execution
			if (fork()) {
				exec(argv[1], xargv);
			} else {
				wait((int*) 0);
			}
		}
	}

	exit(0);
}
