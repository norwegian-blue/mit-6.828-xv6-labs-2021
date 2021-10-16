#include "kernel/types.h"
#include "user/user.h"

int main() {
	int num, prime;
	int p[2];

	pipe(p);

	if (fork() == 0) {
		// Parent process --> list all numbers for the sieve
		close(p[0]);
		for (int i = 2; i <= 35; i++) {
			write(p[1], &i, sizeof(i));
		}
		close(p[1]);
		wait((int*) 0);
		exit(0);
	}

	// Child process(es) --> recursively print first prime, sieve, and pipe the rest
	while (1) {

		int pleft = dup(p[0]);
		close(p[0]);
		close(p[1]);
		pipe(p);
	
		// Print prime number if any left, exit otherwise
		if (pleft < 0 || read(pleft, &prime, sizeof(prime)) == 0) {
			exit(0);
		}
		printf("prime %d\n", prime);

		// Read and sieve the rest
		if (fork() == 0) {
			close(p[0]);
			while (read(pleft, &num, sizeof(num))) {
				if ((num % prime) != 0) {
					write(p[1], &num, sizeof(num));
				}	
			}
			close(pleft);
			close(p[1]);
			wait((int*) 0);
			exit(0);
		}
	}

}
