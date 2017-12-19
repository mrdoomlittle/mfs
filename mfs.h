# ifndef __mfs__h
# define __mfs__h
# define MFS_NULL 0xFFFF
# include <mdlint.h>
# define __debug_enabled
typedef mdl_u16_t mfs_addr_t;
typedef mdl_u16_t mfs_off_t;
typedef mdl_u16_t mfs_uint_t;
struct _mfs {
	void(*write)(void*, mfs_addr_t, mfs_uint_t);
	void(*read)(void*, mfs_addr_t, mfs_uint_t);
	mfs_off_t off;
	mfs_addr_t free, top;
	mfs_addr_t rootdir;
};

struct mfs_dirinfo {
	mdl_u8_t name[100];
	mfs_uint_t no_subdirs;
};


void mfs_write(struct _mfs*, mfs_addr_t, void*, mfs_uint_t, mfs_off_t);
void mfs_read(struct _mfs*, mfs_addr_t, void*, mfs_uint_t, mfs_off_t);
void mfs_del(struct _mfs*, char*, char*);
void mfs_creat(struct _mfs*, char*, char*);
mfs_addr_t mfs_open(struct _mfs*, char*, char*);
void mfs_readdir(struct _mfs*, mfs_addr_t, struct mfs_dirinfo*);
void mfs_init(struct _mfs*, void(*)(void*, mfs_addr_t, mfs_uint_t), void(*)(void*, mfs_addr_t, mfs_uint_t));
void mfs_de_init(struct _mfs*);
void mfs_creatdir(struct _mfs*, char*);
void mfs_deldir(struct _mfs*, char*);
mfs_addr_t mfs_opendir(struct _mfs*, char*);
# endif /*__mfs__h*/
