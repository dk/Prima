/***********************************************************/
/*                                                         */
/*  File system stuff                                      */
/*                                                         */
/***********************************************************/

#include <apricot.h>
#include <sys/stat.h>
#include "unix/guts.h"
#include "Application.h"
#include "File.h"

void
prima_rebuild_watchers( void)
{
	int i;
	PFile f;

	FD_ZERO( &guts.read_set);
	FD_ZERO( &guts.write_set);
	FD_ZERO( &guts.excpt_set);
	FD_SET( guts.connection, &guts.read_set);
	guts.max_fd = guts.connection;
	for ( i = 0; i < guts.files->count; i++) {
		f = (PFile)list_at( guts.files,i);
		if ( f-> eventMask & feRead) {
			FD_SET( f->fd, &guts.read_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
		if ( f-> eventMask & feWrite) {
			FD_SET( f->fd, &guts.write_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
		if ( f-> eventMask & feException) {
			FD_SET( f->fd, &guts.excpt_set);
			if ( f->fd > guts.max_fd)
				guts.max_fd = f->fd;
		}
	}
}

Bool
apc_file_attach( Handle self)
{
	if ( PFile(self)->fd >= FD_SETSIZE ) return false;

	if ( list_index_of( guts.files, self) >= 0) {
		prima_rebuild_watchers();
		return true;
	}
	protect_object( self);
	list_add( guts.files, self);
	prima_rebuild_watchers();
	return true;
}

Bool
apc_file_detach( Handle self)
{
	int i;
	if (( i = list_index_of( guts.files, self)) >= 0) {
		list_delete_at( guts.files, i);
		unprotect_object( self);
		prima_rebuild_watchers();
	}
	return true;
}

Bool
apc_file_change_mask( Handle self)
{
	return apc_file_attach( self);
}

PList
apc_getdir( const char *dirname, Bool is_utf8)
{
	DIR *dh;
	struct dirent *de;
	PList dirlist = NULL;
	char *type;
	char path[ 2048];
	struct stat s;

	if (( dh = opendir( dirname)) && (dirlist = plist_create( 50, 50))) {
		while (( de = readdir( dh))) {
			list_add( dirlist, (Handle)duplicate_string( de-> d_name));
#if defined(DT_REG) && defined(DT_DIR)
			switch ( de-> d_type) {
			case DT_FIFO:	type = "fifo";	break;
			case DT_CHR:	type = "chr";	break;
			case DT_DIR:	type = "dir";	break;
			case DT_BLK:	type = "blk";	break;
			case DT_REG:	type = "reg";	break;
			case DT_LNK:	type = "lnk";	break;
			case DT_SOCK:	type = "sock";	break;
#ifdef DT_WHT
			case DT_WHT:	type = "wht";	break;
#endif
			default:
#endif
				snprintf( path, 2047, "%s/%s", dirname, de-> d_name);
				type = NULL;
				if ( stat( path, &s) == 0) {
					switch ( s. st_mode & S_IFMT) {
					case S_IFIFO:        type = "fifo";  break;
					case S_IFCHR:        type = "chr";   break;
					case S_IFDIR:        type = "dir";   break;
					case S_IFBLK:        type = "blk";   break;
					case S_IFREG:        type = "reg";   break;
					case S_IFLNK:        type = "lnk";   break;
					case S_IFSOCK:       type = "sock";  break;
#ifdef S_IFWHT
					case S_IFWHT:        type = "wht";   break;
#endif
					}
				}
				if ( !type)     type = "unknown";
#if defined(DT_REG) && defined(DT_DIR)
			}
#endif
			list_add( dirlist, (Handle)duplicate_string( type));
		}
		closedir( dh);
	}
	return dirlist;
}

int
apc_fs_access(const char *name, Bool is_utf8, int mode, Bool effective)
{
	return effective ?
		eaccess(name, mode) :
		access(name, mode);
}

Bool
apc_fs_chdir(const char *path, Bool is_utf8 )
{
	return chdir(path) == 0;
}

Bool
apc_fs_chmod( const char *path, Bool is_utf8, int mode)
{
	return chmod(path, mode) == 0;
}

Bool
apc_fs_closedir( PDirHandleRec dh)
{
	return closedir(dh->handle) == 0;
}

char *
apc_fs_from_local(const char * text, int * len)
{
	return (char*) text;
}

char*
apc_fs_getcwd()
{
	char pwd[PATH_MAX];
	return getcwd(pwd, sizeof(pwd)) == NULL ? NULL : duplicate_string(pwd);
}

char *
apc_fs_to_local(const char * text, Bool fail_if_cannot, int * len)
{
	return (char*)text;
}

char*
apc_fs_getenv(const char * varname, Bool is_utf8, Bool * do_free)
{
	*do_free = false;
	return getenv(varname);
}

Bool
apc_fs_link( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	return link(oldname, newname) == 0;
}

Bool
apc_fs_mkdir( const char* path, Bool is_utf8, int mode)
{
	return mkdir(path, mode) == 0;
}

Bool
apc_fs_opendir( const char* path, PDirHandleRec dh)
{
	return (dh->handle = opendir(path)) != NULL;
}

int
apc_fs_open_file( const char* path, Bool is_utf8, int flags, int mode)
{
	return open(path, flags, mode);
}

Bool
apc_fs_readdir( PDirHandleRec dh, char * entry)
{
	struct dirent *de;
	if ( !( de = readdir(dh->handle)))
		return false;
	strncpy( entry, de->d_name, PATH_MAX_UTF8);
	return true;
}

Bool
apc_fs_rename( const char* oldname, Bool is_old_utf8, const char * newname, Bool is_new_utf8 )
{
	return rename(oldname, newname) == 0;
}

Bool
apc_fs_rewinddir( PDirHandleRec dh )
{
	rewinddir(dh->handle);
	return true;
}

Bool
apc_fs_rmdir( const char* path, Bool is_utf8 )
{
	return rmdir(path) == 0;
}

Bool
apc_fs_seekdir( PDirHandleRec dh, long position )
{
	seekdir(dh->handle, position);
	return true;
}

Bool
apc_fs_setenv(const char * varname, Bool is_name_utf8, const char * value, Bool is_value_utf8)
{
	return setenv(varname, value, true) == 0;
}

Bool
apc_fs_stat(const char *name, Bool is_utf8, Bool link, PStatRec statrec)
{
	struct stat statbuf;
	if ( link ) {
		if ( lstat(name, &statbuf) < 0 )
			return 0;
	} else {
		if ( stat(name, &statbuf) < 0 )
			return 0;
	}
	statrec-> dev     = statbuf. st_dev;
	statrec-> ino     = statbuf. st_ino;
	statrec-> mode    = statbuf. st_mode;
	statrec-> nlink   = statbuf. st_nlink;
	statrec-> uid     = statbuf. st_uid;
	statrec-> gid     = statbuf. st_gid;
	statrec-> rdev    = statbuf. st_rdev;
	statrec-> size    = statbuf. st_size;
	statrec-> blksize = statbuf. st_blksize;
	statrec-> blocks  = statbuf. st_blocks;
	statrec-> atim    = (float) statbuf.st_atim.tv_sec + (float) statbuf.st_atim.tv_nsec / 1000000000.0;
	statrec-> mtim    = (float) statbuf.st_mtim.tv_sec + (float) statbuf.st_mtim.tv_nsec / 1000000000.0;
	statrec-> ctim    = (float) statbuf.st_ctim.tv_sec + (float) statbuf.st_ctim.tv_nsec / 1000000000.0;
	return 1;
}

long
apc_fs_telldir( PDirHandleRec dh )
{
	return telldir(dh-> handle);
}

Bool
apc_fs_unlink( const char* path, Bool is_utf8 )
{
	return unlink(path) == 0;
}

Bool
apc_fs_utime( double atime, double mtime, const char* path, Bool is_utf8 )
{
	struct timeval tv[2];
	tv[0].tv_sec  = (long) atime;
	tv[0].tv_usec = (atime - (float) tv[0].tv_sec) * 1000000;
	tv[1].tv_sec  = (long) mtime;
	tv[1].tv_usec = (mtime - (float) tv[1].tv_sec) * 1000000;

	return utimes(path, tv) == 0;
}

