
#include <stdlib.h>
#include "vector.h"




struct _Vector {
	char *buf;
	size_t end;
	size_t ptr;
	size_t lim;
	size_t size;
	size_t recsize;
};



Vector NewVector( size_t recsize )
{	
	Vector v = malloc(sizeof(struct _Vector));
	v->recsize = ((recsize + 7 )/ 8) * 8;
	if( recsize > 1024 ) 
	{
		v->size = ((recsize + 1023) / 1024 ) * 1024;
	}
	v->size = 1024;
	v->buf = malloc( v->size );
	v->end = 0;
	v->ptr = 0;
	v->lim = v->size - v->recsize;
	if( v->buf == NULL )
		return NULL;
	else
		return v;
}

void *VectorAppend( Vector v, void *rec )
{
	void *loc;
	if( v->end >= v->lim )
	{
		v->size = v->size + v->size;
		v->lim = v->size - v->recsize;
		v->buf = realloc( v->buf, v->size );
		if( v->buf == NULL )
			return NULL;
	}
	memcpy(v->buf+v->end, rec, v->recsize );
	loc = &v->buf[(v->end)];
	v->end += v->recsize;
	return loc;
}

void VectorRewind( Vector v )
{
	v->ptr = 0;
}

void *VectorNext( Vector v )
{
	void *loc;
	if( v->ptr >= v->end )
		return NULL;
	loc = &v->buf[v->ptr];
	(v->ptr) += v->recsize;
	return loc;
}

void *VectorLookAhead( Vector v, int lookahead )
{
	void *loc;
	int look = (lookahead-1) * v->recsize;
	if( v->ptr+look >= v->end )
		return NULL;
	loc = &v->buf[v->ptr+look];
	return loc;
}
