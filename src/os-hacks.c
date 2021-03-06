
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef __linux__
#include <linux/limits.h> // for PATH_MAX
#else
#include <limits.h>
#endif

#ifdef __MINGW32__
#include <windows.h>
#include <shfolder.h>
#else
#include <pwd.h>
#endif

#include "os-hacks.h"

int get_home_dir(char* buffer) {
#ifndef __MINGW32__
        struct passwd *pw = getpwuid(getuid());
        strcpy(buffer, pw->pw_dir);
        return 0;
#else
        //char buf[PATH_MAX];
        //SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, buf);
        //printf("get_home_dir() : '%s'\n", buf);
        return (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, buffer)));
#endif
}

void my_dirname (char *path) {
	int n;
	int len = strlen(path);
	for (n = len - 1; n >= 0; n--) {
		if (path[n] == '/' || path[n] == '\\') {
			path[n] = 0;
			break;
		}
	}
}

size_t get_executable_path (char *argv, char* buffer, size_t len) {
#ifdef __MINGW32__
	GetModuleFileName(NULL, buffer, len);
#else
#ifdef __APPLE__
	uint32_t ilen = (uint32_t)len;
	if (_NSGetExecutablePath(buffer, &ilen) == 0) {
		fprintf(stderr, "executable path is %s\n", buffer);
		char *res = realpath(buffer, NULL);
		if (res == NULL) {
			fprintf(stderr, "realpath() failed\n");
		} else {
			snprintf(buffer, len, "%s", res);
			free(res);
		}
	}
#else
	char *res = realpath(argv, NULL);
	if (res == NULL) {
#ifdef TARGET_FREEBSD
			res = realpath("/proc/curproc/file", NULL);
#else
			res = realpath("/proc/self/exe", NULL);
#endif
			if (res == NULL) {
				fprintf(stderr, "realpath() failed\n");
				return -1;
			} else {
				snprintf(buffer, len, "%s", res);
				free(res);
			}
	} else {
		snprintf(buffer, len, "%s", res);
		free(res);
	}
#endif
#endif
	fprintf(stderr, "%s - %s\n", argv, buffer);
	my_dirname(buffer);
	strcat(buffer, "/");
	fprintf(stderr, "%s - %s\n", argv, buffer);
	return (size_t)strlen(buffer);
}

ssize_t getdelim(char **linep, size_t *n, int delim, FILE *fp){
        int ch;
        size_t i = 0;
        if(!linep || !n || !fp){
                errno = EINVAL;
                return -1;
        }
        if(*linep == NULL){
                if(NULL==(*linep = malloc(*n=128))){
                        *n = 0;
                        errno = ENOMEM;
                        return -1;
                }
        }
        while((ch = fgetc(fp)) != EOF){
                if(i + 1 >= *n){
                        char *temp = realloc(*linep, *n + 128);
                        if(!temp){
                                errno = ENOMEM;
                                return -1;
                        }
                        *n += 128;
                        *linep = temp;
                }
                (*linep)[i++] = ch;
                if(ch == delim)
                        break;
        }
        (*linep)[i] = '\0';
        return !i && ch == EOF ? -1 : i;
}

ssize_t getline(char **linep, size_t *n, FILE *fp) {
        return getdelim(linep, n, '\n', fp);
}

char *path_real (char *file) {
	char *realpath = malloc(PATH_MAX + strlen(file) + 2);
	if (program_path[0] == 0) {
		snprintf(realpath, PATH_MAX, "%s", file);
	} else {
		snprintf(realpath, PATH_MAX, "%s%s%s", program_path, DIR_SEP, file);
	}
	return realpath;
}

