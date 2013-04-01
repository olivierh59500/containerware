#ifndef PTRLIST_NAME
# error You must define PTRLIST_NAME to the name of the pointer-list structure
#endif

#ifndef PTRLIST_TYPE
# error You must define PTRLIST_TYPE to the type of entry
#endif

#undef PTRLIST_IDENT
#undef PTRLIST_paste
#undef PTRLIST_paste2
#define PTRLIST_IDENT(name)            PTRLIST_paste(PTRLIST_NAME, name)
#define PTRLIST_paste(a, b)            PTRLIST_paste2(a, b)
#define PTRLIST_paste2(a, b)            a ## _ ## b

typedef struct PTRLIST_IDENT(struct) PTRLIST;

int
PTRLIST_IDENT(init)(PTRLIST *list, size_t blocksize)
{
	if(!blocksize)
	{
		blocksize = 8;
	}
	memset(list, 0, sizeof(PTRLIST));
	pthread_rwlock_init(&(list->lock), NULL);
	list->blocksize = blocksize;
	return 0;
}

int
PTRLIST_IDENT(destroy)(PTRLIST *list)
{
	PTRLIST_IDENT(wrlock)(list);
	free(list->list);
	list->allocated = 0;
	list->count = 0;
	list->blocksize = 0;
	PTRLIST_IDENT(unlock)(list);
	pthread_rwlock_destroy(&(list->lock));
	return 0;
}

int
PTRLIST_IDENT(rdlock)(PTRLIST *list)
{
	return pthread_rwlock_rdlock(&(list->lock));
}

int
PTRLIST_IDENT(tryrdlock)(PTRLIST *list)
{
	return pthread_rwlock_tryrdlock(&(list->lock));
}

int
PTRLIST_IDENT(wrlock)(PTRLIST *list)
{
	return pthread_rwlock_wrlock(&(list->lock));
}

int
PTRLIST_IDENT(trywrlock)(PTRLIST *list)
{
	return pthread_rwlock_trywrlock(&(list->lock));
}

int
PTRLIST_IDENT(unlock)(PTRLIST *list)
{
	return pthread_rwlock_unlock(&(list->lock));
}

int
PTRLIST_IDENT(add)(PTRLIST *list, PTRLIST_TYPE *item)
{
	int r;
	
	PTRLIST_IDENT(wrlock)(list);
	r = PTRLIST_IDENT(add_unlocked)(list, item);
	PTRLIST_IDENT(unlock)(list);
	return r;
}

int
PTRLIST_IDENT(add_unlocked)(PTRLIST *list, PTRLIST_TYPE *item)
{
	PTRLIST_TYPE **p;
	size_t c;
	
	if(!list->blocksize)
	{
		log_printf("ptrlist", CWLOG_CRIT, "object list has not been properly initialised");
		errno = EPERM;
		return -1;		
	}
	if(list->count + 1 > list->allocated)
	{	
		p = (PTRLIST_TYPE **) realloc(list->list, sizeof(PTRLIST_TYPE *) * (list->allocated + list->blocksize));
		if(!p)
		{
			return -1;
		}
		list->list = p;
		c = list->allocated;
		list->allocated += list->blocksize;
		for(; c < list->allocated; c++)
		{
			list->list[c] = NULL;
		}
	}
	for(c = 0; c < list->allocated; c++)
	{
		if(!list->list[c])
		{
			list->list[c] = item;
			list->count++;
			return 0;
		}
	}	
	/* This should never happen */
	errno = EPERM;
	return -1;
}

int
PTRLIST_IDENT(remove)(PTRLIST *list, PTRLIST_TYPE *item)
{
	int r;
	
	PTRLIST_IDENT(wrlock)(list);
	r = PTRLIST_IDENT(remove_unlocked)(list, item);
	PTRLIST_IDENT(unlock)(list);
	return r;
}

int
PTRLIST_IDENT(remove_unlocked)(PTRLIST *list, PTRLIST_TYPE *item)
{
	size_t c;
	
	for(c = 0; c < list->allocated; c++)
	{
		if(list->list[c] == item)
		{
			list->list[c] = NULL;
			list->count--;
			return 0;
		}
	}
	errno = ENOENT;
	return -1;
}

int
PTRLIST_IDENT(iterate)(PTRLIST *list, int (*callback)(PTRLIST_TYPE *item, void *data), void *data)
{
	size_t c;
	int r;
	
	pthread_rwlock_rdlock(&(list->lock));
	for(c = 0; c < list->allocated; c++)
	{
		if(list->list[c])
		{
			r = callback(list->list[c], data);
			if(r)
			{
				break;
			}
		}
	}
	pthread_rwlock_unlock(&(list->lock));
	return 0;
}
