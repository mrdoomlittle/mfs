# include "mfs.h"
# include <stdlib.h>
mfs_addr_t mfs_alloc(struct _mfs*, mfs_uint_t);
void mfs_free(struct _mfs*, mfs_addr_t);
mdl_uint_t mdl_strlen(char const*);
mdl_i8_t mdl_strcmp(char const*, char const*);
mfs_addr_t _mfs_creatdir(struct _mfs*, mfs_addr_t, char*);
void mdl_memcpy(void*, void*, mdl_uint_t);

void mfs_cpystr(struct _mfs*, mfs_addr_t, char const*);
mdl_i8_t mfs_cmpstr(struct _mfs*, mfs_addr_t, char const*);
mfs_addr_t _mfs_opendir(struct _mfs*, mfs_addr_t, char*);

mfs_uint_t mfs_strlen(struct _mfs *__mfs, mfs_addr_t __s) {
	mfs_addr_t itr = __s;
	char c;
	__mfs->read(&c, itr, 1);
	for(;c != '\0';itr++)
		__mfs->read(&c, itr, 1);
	return itr-__s;
}

struct mfs_blkd {
	mfs_uint_t size;
	mfs_addr_t prev, next;
	mfs_addr_t fd, bk;
	mdl_u8_t inuse;
};

struct mfs_file {
	mfs_addr_t name;
	mfs_addr_t prev, next;
	mfs_addr_t p;
	mfs_uint_t size;
};

struct mfs_dir {
	mfs_addr_t name;
	mfs_addr_t files, subdirs;
	mfs_uint_t no_files, no_subdirs;
	mfs_addr_t prev, next, root;
};

# ifdef __debug_enabled
# include <stdio.h>
void pr_blks(struct _mfs *__mfs) {
	struct mfs_blkd blk;
	mfs_addr_t itr = __mfs->top;
	mdl_uint_t blkno = 0;
	while(itr != MFS_NULL) {
		__mfs->read(&blk, itr, sizeof(struct mfs_blkd));
		fprintf(stdout, "blk{%u}, inuse: %s, size: %u\n", blkno++, blk.inuse? "yes":"no", blk.size);
		itr = blk.next;
	}
}

void pr_fr(struct _mfs *__mfs) {
	struct mfs_blkd blk;
	mfs_addr_t itr = __mfs->free;
	while(itr != MFS_NULL) {
		__mfs->read(&blk, itr, sizeof(struct mfs_blkd));
		fprintf(stdout, "blk, size: %u\n", blk.size);
		itr = blk.fd;
	}
}

void pr_pad(mdl_uint_t __pad) {
	while(__pad-- != 0)
		fprintf(stdout, " ");
}

mdl_uint_t pad = 0;
void _pr_dirs(struct _mfs *__mfs, mfs_addr_t __dir) {
	struct mfs_dir dir, sub;
	__mfs->read(&dir, __dir, sizeof(struct mfs_dir));
	char name[100];
	mfs_uint_t l = mfs_strlen(__mfs, dir.name);
	__mfs->read(name, dir.name, l);
	mfs_addr_t itr = dir.subdirs;
	pr_pad(pad);
	fprintf(stdout, "dir: %s, l: %u\n", name, l);
	struct mfs_file file;
	mfs_addr_t fi = dir.files;
	while(fi != MFS_NULL) {
		__mfs->read(&file, fi, sizeof(struct mfs_file));
		l = mfs_strlen(__mfs, file.name);
		__mfs->read(name, file.name, l);
		pr_pad(pad);
		fprintf(stdout, "file: %s\n", name);
		fi = file.next;
	}

	pad++;
	while(itr != MFS_NULL) {
		pr_pad(pad);
		fprintf(stdout, "subdirs:\n");
		__mfs->read(&sub, itr, sizeof(struct mfs_dir));
		_pr_dirs(__mfs, itr);
		itr = sub.next;
	}
	pad--;
}

void pr_dirs(struct _mfs *__mfs) {
	_pr_dirs(__mfs, __mfs->rootdir);
}
# endif

void mfs_init(struct _mfs *__mfs, void(*__wr)(void*, mfs_addr_t, mfs_uint_t), void(*__rd)(void*, mfs_addr_t, mfs_uint_t)) {
	__mfs->write = __wr;
	__mfs->read = __rd;
	__mfs->top = MFS_NULL;
	__mfs->free = MFS_NULL;

	struct mfs_dir root = {
		.files = MFS_NULL, .subdirs = MFS_NULL,
		.prev = MFS_NULL, .next = MFS_NULL
	};
	__mfs->rootdir = mfs_alloc(__mfs, sizeof(struct mfs_dir));
	root.name = mfs_alloc(__mfs, 2);
	mfs_cpystr(__mfs, root.name, "/");
	__mfs->write(&root, __mfs->rootdir, sizeof(struct mfs_dir));
}

void mfs_de_init(struct _mfs *__mfs) {

}

void _mfs_del(struct _mfs *__mfs, mfs_addr_t __dir, mfs_addr_t __file) {
	struct mfs_file file;
	__mfs->read(&file, __file, sizeof(struct mfs_file));
	struct mfs_dir dir;
	__mfs->read(&dir, __dir, sizeof(struct mfs_dir));

	if (__file == dir.files)
		dir.files = file.next;
	if (file.next != MFS_NULL) {
		struct mfs_file next;
		__mfs->read(&next, file.next, sizeof(struct mfs_file));
		next.prev = file.prev;
		__mfs->write(&next, file.next, sizeof(struct mfs_file));
	}
	if (file.prev != MFS_NULL) {
		struct mfs_file prev;
		__mfs->read(&prev, file.prev, sizeof(struct mfs_file));
		prev.next = file.next;
		__mfs->write(&prev, file.prev, sizeof(struct mfs_file));
	}

	mfs_free(__mfs, file.name);
	mfs_free(__mfs, __file);
	if (file.p != MFS_NULL)
		mfs_free(__mfs, file.p);
	__mfs->write(&dir, __dir, sizeof(struct mfs_dir));
}

void mfs_del(struct _mfs *__mfs, char *__dir, char *__name) {
	mfs_addr_t f = mfs_open(__mfs, __dir, __name);
	mfs_addr_t d = mfs_opendir(__mfs, __dir);
	_mfs_del(__mfs, d, f);
}

void mfs_cpystr(struct _mfs *__mfs, mfs_addr_t __dst, char const *__s) {
	char *itr = (char*)__s;
	for(;*itr != '\0';itr++)
		__mfs->write(itr, __dst+(itr-__s), 1);
	__mfs->write(itr, __dst+(itr-__s), 1);
}

mdl_i8_t mfs_cmpstr(struct _mfs *__mfs, mfs_addr_t __p, char const *__s) {
	char *itr = (char*)__s;
	char c;
	while(*itr != '\0') {
		__mfs->read(&c, __p+(itr-__s), 1);
		fprintf(stdout, "%c ? %c\n", c, *itr);
		if (c == '\0') return -1;
		if (c != *itr) return 1;
		itr++;
	}
	return 0;
}

void mfs_creat(struct _mfs *__mfs, char *__dir, char *__name) {
	mfs_addr_t d = mfs_opendir(__mfs, __dir);
	struct mfs_dir dir;
	struct mfs_file file = {
		.prev = MFS_NULL, .next = MFS_NULL, .p = MFS_NULL
	};
	mfs_addr_t f = mfs_alloc(__mfs, sizeof(struct mfs_file));
	__mfs->read(&dir, d, sizeof(struct mfs_dir));

	if (dir.files == MFS_NULL)
		dir.files = f;
	else {
		struct mfs_file old;
		__mfs->read(&old, dir.files, sizeof(struct mfs_file));
		old.prev = f;
		__mfs->write(&old, dir.files, sizeof(struct mfs_file));
		file.next = dir.files;
		dir.files = f;
	}

	file.name = mfs_alloc(__mfs, mdl_strlen(__name)+1);
	mfs_cpystr(__mfs, file.name, __name);

	__mfs->write(&dir, d, sizeof(struct mfs_dir));
	__mfs->write(&file, f, sizeof(struct mfs_file));
}

mfs_addr_t mfs_open(struct _mfs *__mfs, char *__dir, char *__name) {
	mfs_addr_t d = _mfs_opendir(__mfs, __mfs->rootdir, __dir);
	struct mfs_dir dir;
	__mfs->read(&dir, d, sizeof(struct mfs_dir));
	struct mfs_file file;
	mfs_addr_t itr = dir.files;
	while(itr != MFS_NULL) {
		__mfs->read(&file, itr, sizeof(struct mfs_file));
		if (!mfs_cmpstr(__mfs, file.name, __name)) return itr;
		else
			fprintf(stdout, "no match.\n");
		itr = file.next;
	}
	return MFS_NULL;
}

char *mfs_dirname(struct _mfs *__mfs, struct mfs_dir *__dir) {
	static char name[100];
	char c;

	mfs_addr_t itr = __dir->name;
	__mfs->read(&c, itr, 1);
	while(c != '\0') {
		*(name+(itr-__dir->name)) = c;
		fprintf(stdout, "c: %c\n", c);
		__mfs->read(&c, ++itr, 1);
	}
	*(name+(itr-__dir->name)) = '\0';
	return name;
}

void mfs_readdir(struct _mfs *__mfs, mfs_addr_t __dir, struct mfs_dirinfo *__info) {
	struct mfs_dir dir;
	__mfs->read(&dir, __dir, sizeof(struct mfs_dir));
	char *name = mfs_dirname(__mfs, &dir);
	mdl_memcpy(__info->name, name, mdl_strlen(name));
	__info->no_subdirs = dir.no_subdirs;
}

mfs_addr_t _mfs_opendir(struct _mfs *__mfs, mfs_addr_t __root, char *__dir) {
	char name[100];
	char *itr = __dir+1; // ignore root
	struct mfs_dir dir, sub;
	mfs_addr_t p;
	__mfs->read(&dir, __root, sizeof(struct mfs_dir));
	_next:
	p = dir.subdirs;
	while(*itr != '\0' && *itr != '/')
		*(name+((itr-__dir)-1)) = *(itr++);
	*(name+((itr-__dir)-1)) = '\0';

	fprintf(stdout, "'%s' %u\n", name, dir.subdirs);
	while(p != MFS_NULL) {
		__mfs->read(&sub, p, sizeof(struct mfs_dir));
		if (!mfs_cmpstr(__mfs, sub.name, name)) break;
		else
			fprintf(stdout, "dir name did not match.\n");
		p = sub.next;
	}

	dir = sub;
	if (*itr != '\0') {
		itr++;
		goto _next;
	}
	return p;
}

mfs_addr_t mfs_opendir(struct _mfs *__mfs, char *__dir) {
	return _mfs_opendir(__mfs, __mfs->rootdir, __dir);
}

void mfs_write(struct _mfs *__mfs, mfs_addr_t __file, void *__p, mfs_uint_t __bc, mfs_off_t __off) {
	struct mfs_file file;
	__mfs->read(&file, __file, sizeof(struct mfs_file));
	if (file.p == MFS_NULL) {
		file.p = mfs_alloc(__mfs, __bc);
		file.size = __bc;
	} else {
		if (file.size != __bc) {
			mfs_free(__mfs, file.p);
			file.p = mfs_alloc(__mfs, __bc);
			file.size = __bc;
		}
	}

	__mfs->write(__p, file.p+__off, __bc);
	__mfs->write(&file, __file, sizeof(struct mfs_file));
}

void mfs_read(struct _mfs *__mfs, mfs_addr_t __file, void *__p, mfs_uint_t __bc, mfs_off_t __off) {
	struct mfs_file file;
	__mfs->read(&file, __file, sizeof(struct mfs_file));
	if (file.p == MFS_NULL) return;
	__mfs->read(__p, file.p+__off, __bc);
}

mfs_addr_t _mfs_creatdir(struct _mfs *__mfs, mfs_addr_t __root, char *__name) {
	struct mfs_dir root;
	__mfs->read(&root, __root, sizeof(struct mfs_dir));

	struct mfs_dir dir = {
		.files = MFS_NULL, .subdirs = MFS_NULL,
		.prev = MFS_NULL, .next = MFS_NULL,
		.root = __root
	};

	dir.name = mfs_alloc(__mfs, mdl_strlen(__name)+1);
	mfs_cpystr(__mfs, dir.name, __name);
	mfs_addr_t p = mfs_alloc(__mfs, sizeof(struct mfs_dir));
	__mfs->write(&dir, p, sizeof(struct mfs_dir));
	if (root.subdirs == MFS_NULL)
		root.subdirs = p;
	else {
		struct mfs_dir dir, old;
		__mfs->read(&dir, p, sizeof(struct mfs_dir));
		__mfs->read(&old, root.subdirs, sizeof(struct mfs_dir));
		old.prev = p;
		__mfs->write(&old, root.subdirs, sizeof(struct mfs_dir));
		dir.next = root.subdirs;
		root.subdirs = p;
		__mfs->write(&dir, p, sizeof(struct mfs_dir));
	}

	root.no_subdirs++;
	__mfs->write(&root, __root, sizeof(struct mfs_dir));
	return p;
}

void mfs_creatdir(struct _mfs *__mfs, char *__dir) {
	char dir[100];
	mdl_uint_t l = mdl_strlen(__dir);
	mdl_memcpy(dir, __dir, l);
	char *itr = dir+l;
	while(*itr != '/')
		*(itr--) = '\0';
	if (itr != dir)
		*itr = '\0';

	fprintf(stdout, "-- dir: %s\n", dir);
	if (itr == dir)
		_mfs_creatdir(__mfs, __mfs->rootdir, __dir+1);
	else
		_mfs_creatdir(__mfs, _mfs_opendir(__mfs, __mfs->rootdir, dir), __dir+1);
}

void _mfs_deldir(struct _mfs *__mfs, mfs_addr_t __dir) {
	struct mfs_dir dir, sub;
	__mfs->read(&dir, __dir, sizeof(struct mfs_dir));
	mfs_addr_t itr = dir.subdirs;
	while(itr != MFS_NULL) {
		__mfs->read(&sub, itr, sizeof(struct mfs_dir));
		_mfs_deldir(__mfs, itr);
		itr = sub.next;
	}
	itr = dir.files;
	struct mfs_file file;
	while(itr != MFS_NULL) {
		__mfs->read(&file, itr, sizeof(struct mfs_file));
		_mfs_del(__mfs, __dir, itr);
		itr = file.next;
	}

	struct mfs_dir root;
	__mfs->read(&root, dir.root, sizeof(struct mfs_dir));
	if (root.subdirs == __dir) {
		root.subdirs = dir.next;
		__mfs->write(&root, dir.root, sizeof(struct mfs_dir));
	}

	if (dir.next != MFS_NULL) {
		struct mfs_dir next;
		__mfs->read(&next, dir.next, sizeof(struct mfs_dir));
		next.prev = dir.prev;
		__mfs->write(&next, dir.next, sizeof(struct mfs_dir));
	}

	if (dir.prev != MFS_NULL) {
		struct mfs_dir prev;
		__mfs->read(&prev, dir.prev, sizeof(struct mfs_dir));
		prev.next = dir.next;
		__mfs->write(&prev, dir.prev, sizeof(struct mfs_dir));
	}
}

void mfs_deldir(struct _mfs *__mfs, char *__dir) {
	mfs_addr_t d = mfs_opendir(__mfs, __dir);
	_mfs_deldir(__mfs, d);
}

void mfs_unchain_f(struct _mfs*, mfs_addr_t, struct mfs_blkd*);
mfs_addr_t mfs_alloc(struct _mfs *__mfs, mfs_uint_t __bc) {
	if (__mfs->free != MFS_NULL) {
		struct mfs_blkd blk;
		mfs_addr_t itr = __mfs->free;
		while(itr != MFS_NULL) {
			__mfs->read(&blk, itr, sizeof(struct mfs_blkd));
			if (blk.size >= __bc) {
				mfs_unchain_f(__mfs, itr, &blk);
				blk.inuse = 0x1;
				__mfs->write(&blk, itr, sizeof(struct mfs_blkd));
				if (blk.size > __bc+sizeof(struct mfs_blkd)) {
					struct mfs_blkd junk = {
						.size = blk.size-__bc-sizeof(struct mfs_blkd),
						.prev = itr, .next = blk.next,
						.fd = MFS_NULL, .bk = MFS_NULL,
						.inuse = 0x1
					};

					mfs_addr_t j = itr+sizeof(struct mfs_blkd)+__bc;
					__mfs->write(&junk, j, sizeof(struct mfs_blkd));

					blk.size = __bc;
					blk.next = j;
					__mfs->write(&blk, itr, sizeof(struct mfs_blkd));
					mfs_free(__mfs, j+sizeof(struct mfs_blkd));
				}
				return itr+sizeof(struct mfs_blkd);
			}

			itr = blk.fd;
		}
	}

	struct mfs_blkd blk = {
		.size = __bc,
		.prev = MFS_NULL, .next = __mfs->top,
		.fd = MFS_NULL, .bk = MFS_NULL,
		.inuse = 0x1
	};

	if (__mfs->top != MFS_NULL) {
		struct mfs_blkd blk;
		__mfs->read(&blk, __mfs->top, sizeof(struct mfs_blkd));
		blk.prev = __mfs->off;
		__mfs->write(&blk, __mfs->top, sizeof(struct mfs_blkd));
	}

	__mfs->write(&blk, __mfs->off, sizeof(struct mfs_blkd));
	__mfs->top = __mfs->off;
	__mfs->off+=sizeof(struct mfs_blkd)+__bc;
	return __mfs->off-__bc;
}

void mfs_unchain_blk(struct _mfs *__mfs, mfs_addr_t __p, struct mfs_blkd *__blk) {
	if (__mfs->top == __p)
		__mfs->top = __blk->next;
	if (__blk->next != MFS_NULL) {
		struct mfs_blkd next;
		__mfs->read(&next, __blk->next, sizeof(struct mfs_blkd));
		next.prev = __blk->prev;
		__mfs->write(&next, __blk->next, sizeof(struct mfs_blkd));
	}
	if (__blk->prev != MFS_NULL) {
		struct mfs_blkd prev;
		__mfs->read(&prev, __blk->prev, sizeof(struct mfs_blkd));
		prev.next = __blk->next;
		__mfs->write(&prev, __blk->prev, sizeof(struct mfs_blkd));
	}
}

void mfs_rechain_blk(struct _mfs *__mfs, mfs_addr_t __p, struct mfs_blkd *__blk) {
	if (__blk->next == __mfs->top)
		__mfs->top = __p;
	if (__blk->next != MFS_NULL) {
		struct mfs_blkd next;
		__mfs->read(&next, __blk->next, sizeof(struct mfs_blkd));
		next.prev = __p;
		__mfs->write(&next, __blk->next, sizeof(struct mfs_blkd));
	}
	if (__blk->prev != MFS_NULL) {
		struct mfs_blkd prev;
		__mfs->read(&prev, __blk->prev, sizeof(struct mfs_blkd));
		prev.next = __p;
		__mfs->write(&prev, __blk->prev, sizeof(struct mfs_blkd));
	}
}

void mfs_unchain_f(struct _mfs *__mfs, mfs_addr_t __p, struct mfs_blkd *__blk) {
	if (__mfs->free == __p)
		__mfs->free = __blk->fd;
	if (__blk->fd != MFS_NULL) {
		struct mfs_blkd fd;
		__mfs->read(&fd, __blk->fd, sizeof(struct mfs_blkd));
		fd.bk = __blk->bk;
		__mfs->write(&fd, __blk->fd, sizeof(struct mfs_blkd));
	}
	if (__blk->bk != MFS_NULL) {
		struct mfs_blkd bk;
		__mfs->read(&bk, __blk->bk, sizeof(struct mfs_blkd));
		bk.fd = __blk->fd;
		__mfs->write(&bk, __blk->bk, sizeof(struct mfs_blkd));
	}
}

void mfs_free(struct _mfs *__mfs, mfs_addr_t __p) {
	mfs_addr_t blkp = __p-sizeof(struct mfs_blkd), p, top;
	top = blkp;

	struct mfs_blkd blk, prev, next;
	__mfs->read(&blk, blkp, sizeof(struct mfs_blkd));

	mfs_unchain_blk(__mfs, blkp, &blk);
	if (blk.prev != MFS_NULL) {
		__mfs->read(&prev, blk.prev, sizeof(struct mfs_blkd));
		p = blk.prev;
		while(!prev.inuse) {
			blk.prev = prev.prev;
			fprintf(stdout, "found free block, prev.\n");
			mfs_unchain_f(__mfs, p, &prev);
			mfs_unchain_blk(__mfs, p, &prev);
			blk.size+= prev.size+sizeof(struct mfs_blkd);
			top = p;
			if (prev.prev == MFS_NULL) break;
			__mfs->read(&prev, prev.prev, sizeof(struct mfs_blkd));
			p = prev.prev;
		}

		if (top != blkp) {
			blkp = top;
			__mfs->write(&blk, blkp, sizeof(struct mfs_blkd));
		}
	}

	if (blk.next != MFS_NULL) {
		__mfs->read(&next, blk.next, sizeof(struct mfs_blkd));
		p = blk.next;
		while(!next.inuse) {
			blk.next = next.next;
			fprintf(stdout, "found free block, next.\n");
			mfs_unchain_f(__mfs, p, &next);
			mfs_unchain_blk(__mfs, p, &next);
			blk.size+= next.size+sizeof(struct mfs_blkd);
			if (next.next == MFS_NULL) break;
			__mfs->read(&next, next.next, sizeof(struct mfs_blkd));
			p = next.next;
		}
	}

	blk.inuse = 0;
	mfs_rechain_blk(__mfs, blkp, &blk);
	if (__mfs->free == MFS_NULL)
		__mfs->free = blkp;
	else {
		blk.fd = __mfs->free;
		struct mfs_blkd free;
		__mfs->read(&free, __mfs->free, sizeof(struct mfs_blkd));
		free.bk = blkp;
		__mfs->write(&free, __mfs->free, sizeof(struct mfs_blkd));
		__mfs->free = blkp;
	}
	__mfs->write(&blk, blkp, sizeof(struct mfs_blkd));
}
