#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

void strcat(char* s1, char* s2) {
	char* p1 = s1+strlen(s1);
	for (; *s2; s2++) {
		*p1 = *s2;
		p1++;
	}
	*p1 = 0;
}

char* getFileName(char* path) {
	
	char *p;
	// Find first character after last slash
	for(p = path+strlen(path); p >= path && *p != '/'; p--) 
		;
	p++;

	return p;
}

void find(char *path, char *expr) {

	int fd;
	struct stat st;
	struct dirent de;
	char buf[512];

	if ((fd = open(path, 0)) < 0) {
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type) {
	case T_FILE:
		if (strcmp(expr, getFileName(path)) == 0) {
			printf("%s\n", path);
		}
		break;

	case T_DIR:
		while(read(fd, &de, sizeof(de)) == sizeof(de)) {
			if (de.inum == 0 || de.name[0] == '.')
				continue;
			strcpy(buf, path);
			strcat(buf, "/");
			strcat(buf, de.name);
			find(buf, expr);
		}	
	}
	close(fd);
}

int main(int argc, char *argv[]) {
	
	if (argc == 2) {
		find(".", argv[1]);
	} else if (argc == 3) {
		find(argv[1], argv[2]);
	} else {
		fprintf(2, "usage: find [path] expression\n");
	}
	exit(0);
}
