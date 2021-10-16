#include "kernel/types.h"
#include "user/user.h"

int main() {

	int p_p2c[2];
	int p_c2p[2];
	
	char ball[1];

	pipe(p_p2c);
	pipe(p_c2p);

	if (fork() == 0) {
		// Child process
		close(p_p2c[1]);
		close(p_c2p[0]);
		read(p_p2c[0], ball, 1);
		printf("%d: received ping\n", getpid());
		write(p_c2p[1], ball, 1);
		close(p_p2c[0]);
		close(p_c2p[1]);
	} else {
		// Parent process
		close(p_p2c[0]);
		close(p_c2p[1]);
		write(p_p2c[1], "0", 1);
		read(p_c2p[0], ball, 1);
		printf("%d: received pong\n", getpid());
		close(p_p2c[1]);
		close(p_c2p[0]);
	}
	exit(0);
}
