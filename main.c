# include <stdio.h>
# include "mfs.h"
# include <mdlint.h>
# include <malloc.h>
# include <string.h>
mdl_uint_t mdl_strlen(char const *__s) {
	return strlen(__s);
}

mdl_i8_t mdl_strcmp(char const *__s1, char const *__s2) {
	if (!strcmp(__s1, __s2)) return 0;
	return 1;
}

mdl_u8_t *_p;

void mdl_memcpy(void *__dst, void *__src, mdl_uint_t __bc) {
	mdl_u8_t *itr = (mdl_u8_t*)__src;
	while(itr != (mdl_u8_t*)__src+__bc)
		*((mdl_u8_t*)__dst+(itr-(mdl_u8_t*)__src)) = *(itr++);
}

void static write(void *__p, mfs_addr_t __addr, mfs_uint_t __bc) {
	fprintf(stdout, "----> %u\n", __addr);
	mdl_memcpy(_p+__addr, __p, __bc);
}

void static read(void *__p, mfs_addr_t __addr, mfs_uint_t __bc) {
	mdl_memcpy(__p, _p+__addr, __bc);
}


void mfs_free(struct _mfs*, mfs_addr_t);
mfs_addr_t mfs_alloc(struct _mfs*, mfs_uint_t);

void pr_fr(struct _mfs*);
void pr_blks(struct _mfs*);
void pr_dirs(struct _mfs*);
extern int usleep(__useconds_t);
int main(void) {
	_p = (mdl_u8_t*)malloc(400);
	char cuf[100];
	struct _mfs fs;

/*
	fs.free = MFS_NULL;
	fs.top = MFS_NULL;
	fs.off = 0;
	fs.write = write;
	fs.read = read;
	mfs_addr_t a = mfs_alloc(&fs, 3);
	mfs_addr_t b = mfs_alloc(&fs, 6);
	mfs_addr_t c = mfs_alloc(&fs, 7);
	mfs_addr_t d = mfs_alloc(&fs, 6);
	mfs_addr_t e = mfs_alloc(&fs, 33);

	mfs_free(&fs, a);
	mfs_free(&fs, b);
	mfs_free(&fs, c);
	mfs_free(&fs, d);
	mfs_free(&fs, e);

	pr_blks(&fs);
	pr_fr(&fs);

	free(_p);
	return 0;
*/

	mfs_init(&fs, write, read);

	mfs_creatdir(&fs, "/a");
	mfs_creatdir(&fs, "/b");
	mfs_creat(&fs, "/a", "hello.txt");
	mfs_creat(&fs, "/b", "work.txt");

	mfs_addr_t p = mfs_open(&fs, "/b", "work.txt");
	strcpy(cuf, "https://github.com/mrdoomlittle/mfs");
	mfs_write(&fs, p, cuf, sizeof(cuf), 0);

	memset(cuf, '\0', sizeof(cuf));
	mfs_read(&fs, p, cuf, sizeof(cuf), 0);

	fprintf(stdout, "-------- cuf: %s\n", cuf);
/*
	mdl_uint_t i = 0;
	while(i++ != 20) {
		mfs_creat(&fs, "/b", "work.txt");

		strcpy(cuf, "mrdoomlittle");
		mfs_addr_t p = mfs_open(&fs, "/b", "work.txt");
		mfs_write(&fs, p, cuf, 12, 0);
		mfs_write(&fs, p, cuf, 30, 0);

		mfs_del(&fs, "/b", "work.txt");
		fprintf(stdout, "-----------off: %u\n", fs.off);
	}
*/
/*
	struct mfs_dirinfo info;
	mfs_readdir(&fs, p, &info);
	fprintf(stdout, "-> name: %s\n", info.name);
*/
	pr_blks(&fs);
	pr_fr(&fs);
	pr_dirs(&fs);
	mfs_de_init(&fs);
	free(_p);
}
