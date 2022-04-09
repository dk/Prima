#include "apricot.h"
#include "guts.h"

#ifdef __cplusplus
extern "C" {
#endif

void
list_create( PList slf, int size, int delta)
{
	if ( !slf) return;
	memset( slf, 0, sizeof( List));
	slf-> delta = ( delta > 0) ? delta : 1;
	if (( slf-> size = size) > 0) {
		if ( !( slf-> items = allocn( Handle, size)))
			slf-> size = 0;
	} else
		slf-> items = NULL;
}

PList
plist_create( int size, int delta)
{
	PList new_list = alloc1( List);
	if ( new_list != NULL) {
		list_create( new_list, size, delta);
	}
	return new_list;
}

PList
plist_dup( PList slf )
{
	PList n = plist_create( slf-> count, slf-> delta );
	if ( n ) {
		n-> count = slf->count;
		memcpy( n-> items, slf-> items, n->count * sizeof(Handle));
	}
	return n;
}

void
list_destroy( PList slf)
{
	if ( !slf) return;
	free( slf-> items);
	slf-> items = NULL;
	slf-> count = 0;
	slf-> size  = 0;
}

void
plist_destroy( PList slf)
{
	if ( slf != NULL) {
		list_destroy( slf);
		free( slf);
	}
}

int
list_add( PList slf, Handle item)
{
	if ( !slf) return -1;
	if ( slf-> count == slf-> size)
	{
		Handle * old = slf-> items;
		if ( !( slf-> items = allocn(Handle, ( slf-> size + slf-> delta))))
			return -1;
		if ( old) {
			memcpy( slf-> items, old, slf-> size * sizeof( Handle));
			free( old);
		}
		slf-> size += slf-> delta;
	}
	slf-> items[ slf-> count++] = item;
	return slf-> count - 1;
}

int
list_insert_at( PList slf, Handle item, int pos)
{
	int max, ret;
	Handle save;
	ret = list_add( slf, item);
	if ( ret < 0) return ret;
	max = slf-> count - 1;
	if ( pos < 0 || pos >= max) return ret;
	save = slf-> items[ max];
	memmove( &slf-> items[ pos + 1], &slf-> items[ pos], ( max - pos) * sizeof( Handle));
	slf-> items[ pos] = save;
	return pos;
}

int
list_index_of( PList slf, Handle item)
{
	int i;
	if ( !slf ) return -1;
	for ( i = 0; i < slf-> count; i++)
		if ( slf-> items[ i] == item) return i;
	return -1;
}

void
list_delete( PList slf, Handle item)
{
	list_delete_at( slf, list_index_of( slf, item));
}

void
list_delete_at( PList slf, int index)
{
	if ( !slf || index < 0 || index >= slf-> count) return;
	slf-> count--;
	if ( index == slf-> count) return;
	memmove( &slf-> items[ index], &slf-> items[ index + 1], ( slf-> count - index) * sizeof( Handle));
}

Handle
list_at( PList slf, int index)
{
	return (( index < 0 || !slf) || index >= slf-> count) ? NULL_HANDLE : slf-> items[ index];
}

int
list_first_that( PList slf, void * action, void * params)
{
	int toRet = -1, i, cnt;
	Handle * list;
	if ( !action || !slf || !slf->count) return -1;
	if ( !( list = allocn( Handle, slf-> count)))
		return -1;
	memcpy( list, slf-> items, slf-> count * sizeof( Handle));
	cnt = slf->count;
	for ( i = 0; i < cnt; i++)
		if ((( PListProc) action)( list[ i], params)) {
			toRet = i;
			break;
		}
	free( list);
	return toRet;
}

void
list_delete_all( PList slf, Bool kill)
{
	if ( !slf || ( slf-> count == 0)) return;
	if ( kill ) {
		int i;
		for ( i = 0; i < slf-> count; i++)
			free(( void*) slf-> items[ i]);
	}
	slf-> count = 0;
}

#ifdef __cplusplus
}
#endif
