#ifndef __AUX_H__
#define __AUX_H__



#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <glib.h>
#include <pcre.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/resource.h>
#include <stdio.h>




#define OFFSET_OF(t,e)		((unsigned int) (unsigned long) &(((t *) 0)->e))
#define ZERO(x)			memset(&(x), 0, sizeof(x))

#define IPF			"%u.%u.%u.%u"
#define IPP(x)			((unsigned char *) (&(x)))[0], ((unsigned char *) (&(x)))[1], ((unsigned char *) (&(x)))[2], ((unsigned char *) (&(x)))[3]
#define IP6F			"%x:%x:%x:%x:%x:%x:%x:%x"
#define IP6P(x)			ntohs(((u_int16_t *) (x))[0]), \
				ntohs(((u_int16_t *) (x))[1]), \
				ntohs(((u_int16_t *) (x))[2]), \
				ntohs(((u_int16_t *) (x))[3]), \
				ntohs(((u_int16_t *) (x))[4]), \
				ntohs(((u_int16_t *) (x))[5]), \
				ntohs(((u_int16_t *) (x))[6]), \
				ntohs(((u_int16_t *) (x))[7])
#define D6F			"["IP6F"]:%u"
#define D6P(x)			IP6P((x).sin6_addr.s6_addr), ntohs((x).sin6_port)
#define DF			IPF ":%u"
#define DP(x)			IPP((x).sin_addr.s_addr), ntohs((x).sin_port)

#define BIT_ARRAY_DECLARE(name, size)	int name[((size) + sizeof(int) * 8 - 1) / (sizeof(int) * 8)]

#define UINT64F			"%" G_GUINT64_FORMAT




typedef int (*parse_func)(char **, void **, void *);

GList *g_list_link(GList *, GList *);
int pcre_multi_match(pcre *, pcre_extra *, const char *, unsigned int, parse_func, void *, GQueue *);
static inline void strmove(char **, char **);
static inline void strdupfree(char **, const char *);


#if !GLIB_CHECK_VERSION(2,14,0)
#define G_QUEUE_INIT { NULL, NULL, 0 }
void g_string_vprintf(GString *string, const gchar *format, va_list args);
void g_queue_clear(GQueue *);
#endif

#if !GLIB_CHECK_VERSION(2,32,0)
static inline int g_hash_table_contains(GHashTable *h, const void *k) {
	return g_hash_table_lookup(h, k) ? 1 : 0;
}
#endif


static inline void strmove(char **d, char **s) {
	if (*d)
		free(*d);
	*d = *s;
	*s = strdup("");
}

static inline void strdupfree(char **d, const char *s) {
	if (*d)
		free(*d);
	*d = strdup(s);
}


static inline void nonblock(int fd) {
	fcntl(fd, F_SETFL, O_NONBLOCK);
}
static inline void reuseaddr(int fd) {
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}
static inline void ipv6only(int fd, int yn) {
	setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yn, sizeof(yn));
}

static inline int bit_array_isset(int *name, unsigned int bit) {
	return name[(bit) / (sizeof(int) * 8)] & (1 << ((bit) % (sizeof(int) * 8)));
}

static inline void bit_array_set(int *name, unsigned int bit) {
	name[(bit) / (sizeof(int) * 8)] |= 1 << ((bit) % (sizeof(int) * 8));
}

static inline void bit_array_clear(int *name, unsigned int bit) {
	name[(bit) / (sizeof(int) * 8)] &= ~(1 << ((bit) % (sizeof(int) * 8)));
}

static inline char chrtoupper(char x) {
	return x & 0xdf;
}

static inline void swap_ptrs(void *a, void *b) {
	void *t, **aa, **bb;
	aa = a;
	bb = b;
	t = *aa;
	*aa = *bb;
	*bb = t;
}

static inline void in4_to_6(struct in6_addr *o, u_int32_t ip) {
	o->s6_addr32[0] = 0;
	o->s6_addr32[1] = 0;
	o->s6_addr32[2] = htonl(0xffff);
	o->s6_addr32[3] = ip;
}

static inline void smart_ntop(char *o, struct in6_addr *a, size_t len) {
	const char *r;

	if (IN6_IS_ADDR_V4MAPPED(a))
		r = inet_ntop(AF_INET, &(a->s6_addr32[3]), o, len);
	else
		r = inet_ntop(AF_INET6, a, o, len);

	if (!r)
		*o = '\0';
}

static inline char *smart_ntop_p(char *o, struct in6_addr *a, size_t len) {
	int l;

	if (IN6_IS_ADDR_V4MAPPED(a)) {
		if (inet_ntop(AF_INET, &(a->s6_addr32[3]), o, len))
			return o + strlen(o);
		*o = '\0';
		return NULL;
	}
	else {
		*o = '[';
		if (!inet_ntop(AF_INET6, a, o+1, len-2)) {
			*o = '\0';
			return NULL;
		}
		l = strlen(o);
		o[l] = ']';
		o[l+1] = '\0';
		return o + (l + 1);
	}
}

static inline void smart_ntop_port(char *o, struct sockaddr_in6 *a, size_t len) {
	char *e;

	e = smart_ntop_p(o, &a->sin6_addr, len);
	if (!e)
		return;
	if (len - (e - o) < 7)
		return;
	sprintf(e, ":%hu", ntohs(a->sin6_port));
}

static inline int smart_pton(int af, char *src, void *dst) {
	char *p;
	int ret;

	if (af == AF_INET6) {
		if (src[0] == '[' && (p = strchr(src, ']'))) {
			*p = '\0';
			ret = inet_pton(af, src+1, dst);
			*p = ']';
			return ret;
		}
	}
	return inet_pton(af, src, dst);
}

static inline int strmemcmp(const void *mem, int len, const char *str) {
	int l = strlen(str);
	if (l < len)
		return -1;
	if (l > len)
		return 1;
	return memcmp(mem, str, len);
}



typedef pthread_mutex_t mutex_t;
typedef pthread_rwlock_t rwlock_t;
typedef pthread_cond_t cond_t;

#define mutex_init(m) pthread_mutex_init(m, NULL)
#define mutex_destroy(m) pthread_mutex_destroy(m)
#define mutex_lock(m) pthread_mutex_lock(m)
#define mutex_trylock(m) pthread_mutex_trylock(m)
#define mutex_unlock(m) pthread_mutex_unlock(m)
#define MUTEX_STATIC_INIT PTHREAD_MUTEX_INITIALIZER

#define rwlock_init(l) pthread_rwlock_init(l, NULL)
#define rwlock_lock_r(l) pthread_rwlock_rdlock(l)
#define rwlock_unlock_r(l) pthread_rwlock_unlock(l)
#define rwlock_lock_w(l) pthread_rwlock_wrlock(l)
#define rwlock_unlock_w(l) pthread_rwlock_unlock(l)

#define cond_init(c) pthread_cond_init(c, NULL)
#define cond_wait(c,m) pthread_cond_wait(c,m)
#define cond_signal(c) pthread_cond_signal(c)
#define cond_broadcast(c) pthread_cond_broadcast(c)
#define COND_STATIC_INIT PTHREAD_COND_INITIALIZER


void threads_join_all(int);
void thread_create_detach(void (*)(void *), void *);



static inline int rlim(int res, rlim_t val) {
	struct rlimit rlim;

	ZERO(rlim);
	rlim.rlim_cur = rlim.rlim_max = val;
	return setrlimit(res, &rlim);
}

static inline int is_addr_unspecified(const struct in6_addr *a) {
	if (a->s6_addr32[0])
		return 0;
	if (a->s6_addr32[1])
		return 0;
	if (a->s6_addr32[3])
		return 0;
	if (a->s6_addr32[2] == 0 || a->s6_addr32[2] == htonl(0xffff))
		return 1;
	return 0;
}

#endif
