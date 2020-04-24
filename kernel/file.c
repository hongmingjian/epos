#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "kernel.h"

int sys_open(char *path, int mode)
{
	int ifs;

	if(strlen(path) < 3)
		return -1;
	if(path[1] != ':' || path[2] != '/')
		return -1;
	if(!isalpha(path[0]) && path[0] != '$')
		return -1;

	if(path[0] == '$')
		ifs = 0;
	else
		ifs = toupper(path[0]) - 'A' + 1;

	if(ifs >= NR_FILE_SYSTEM)
		return -1;
	if(g_fs_vector[ifs] == NULL)
		return -1;

	int fd;
	for(fd = 0; fd < NR_OPEN_FILE; fd++) {
		if(g_file_vector[fd] != NULL)
			continue;
		break;
	}
	if(fd == NR_OPEN_FILE) {
		return -1;
	}

	struct file *fp;
	if(0 == g_fs_vector[ifs]->open(g_fs_vector[ifs], &path[3], mode, &fp)) {
		fp->refcnt++;
		g_file_vector[fd] = fp;
		return fd;
	}

	return -1;
}

int sys_close(int fd)
{
	if(fd < 0 || fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
		return -1;
	g_file_vector[fd]->refcnt--;
	if(g_file_vector[fd]->refcnt == 0)
		g_file_vector[fd]->fs->close(g_file_vector[fd]);
	g_file_vector[fd] = NULL;
	return 0;
}

int sys_read(int fd, uint8_t *buffer, size_t size)
{
	if(fd < 0 || fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
		return -1;
	return g_file_vector[fd]->fs->read(g_file_vector[fd], buffer, size);
}

int sys_write(int fd, uint8_t *buffer, size_t size)
{
	if(fd < 0 || fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
		return -1;
	return g_file_vector[fd]->fs->write(g_file_vector[fd], buffer, size);
}

int sys_seek(int fd, off_t offset, int whence)
{
	if(fd < 0 || fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
		return -1;
	return g_file_vector[fd]->fs->seek(g_file_vector[fd], offset, whence);
}

int sys_ioctl(int fd, uint32_t cmd, void *arg)
{
	if(fd < 0 || fd >= NR_OPEN_FILE || g_file_vector[fd] == NULL)
		return -1;
	return g_file_vector[fd]->fs->ioctl(g_file_vector[fd], cmd, arg);
}
