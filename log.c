#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_containerware.h"

static int log_level = CWLOG_DEBUG;
static pthread_mutex_t log_lock;
static const char *short_program_name = "cw";

static int log_prefix_(FILE *fout, CONTAINER_WORKER_HOST *host, int severity);
static int log_vxprintf_(const char *facility, CONTAINER_WORKER_HOST *host, int severity, const char *fmt, va_list ap);
static int log_xprintf_(const char *facility, CONTAINER_WORKER_HOST *host, int severity, const char *fmt, ...);

static const char *log_levels[] = {
	"emerg",
	"alert",
	"critical",
	"error",
	"warn",
	"notice",
	"info",
	"access",
	"debug"
};

/* Initialise logging
 * Threading: must be called on the main thread before any other logging
 * calls.
 */
int
log_init(int argc, char **argv)
{
	const char *t;
	
	pthread_mutex_init(&log_lock, NULL);
	if(argc && argv[0] && argv[0][0])
	{
		t = strrchr(argv[0], '/');
		if(t)
		{
			short_program_name = t + 1;	
		}
		else
		{
			short_program_name = argv[0];
		}
	}
	return 0;
}

/* Write a log prefix to fout
 * Threading: must only be called by the thread which holds log_lock
 */
static int
log_prefix_(FILE *fout, CONTAINER_WORKER_HOST *host, int severity)
{
	time_t t;
	struct tm tm;
	static char tbuf[48], sbuf[48];
	const char *s, *h;

	t = time(NULL);
	gmtime_r(&t, &tm);
	strftime(tbuf, sizeof(tbuf), "%b %e %H:%M:%S", &tm);
	if(severity < 0 || severity > CWLOG_DEBUG)
	{
		snprintf(sbuf, sizeof(sbuf), "%d", severity);
		s = sbuf;
	}
	else
	{
		s = log_levels[severity];
	}
	h = config_get("global:instance", "localhost");
	if(host)
	{
		fprintf(fout, "%s %s %s[%lu/%lu] [%s] ", tbuf, host->info.instance, host->info.app, (unsigned long) host->info.pid, (unsigned long) host->info.workerid, s);
	}
	else
	{
		fprintf(fout, "%s %s %s[%lu] [%s] ", tbuf, h, short_program_name, (unsigned long) getpid(), s);
	}
	return 0;
}

static int
log_vxprintf_(const char *facility, CONTAINER_WORKER_HOST *host, int severity, const char *fmt, va_list ap)
{
	if(severity <= log_level)
	{
		pthread_mutex_lock(&log_lock);	
		log_prefix_(stderr, host, severity);
		if(facility)
		{
			fprintf(stderr, "%s: ", facility);
		}
		JD_SCOPE
		{
			fputs(jd_bytes(jd_vprintf(jd_nv(), fmt, ap), NULL), stderr);
		}
		fputc('\n', stderr);
		fflush(stderr);
		pthread_mutex_unlock(&log_lock);	
	}
	return 0;
}

static int
log_xprintf_(const char *facility, CONTAINER_WORKER_HOST *host, int severity, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	return log_vxprintf_(facility, host, severity, fmt, ap);
}

int
log_puts(const char *facility, int severity, const char *str)
{
	return log_xprintf_(facility, NULL, severity, "%s", str);
}

int
log_vprintf(const char *facility, int severity, const char *fmt, va_list ap)
{
	return log_vxprintf_(facility, NULL, severity, fmt, ap);
}

int
log_printf(const char *facility, int severity, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	return log_vxprintf_(facility, NULL, severity, fmt, ap);
}

int
log_hputs(CONTAINER_WORKER_HOST *host, int severity, const char *str)
{
	return log_xprintf_(NULL, host, severity, str);
}

int
log_hvprintf(CONTAINER_WORKER_HOST *host, int severity, const char *fmt, va_list ap)
{
	return log_vxprintf_(NULL, host, severity, fmt, ap);
}

int
log_hprintf(CONTAINER_WORKER_HOST *host, int severity, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	return log_vxprintf_(NULL, host, severity, fmt, ap);
}
