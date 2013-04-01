/* Utilities for endpoint module implementors */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_cw.h"

static int cw_request_headers_vset_status_(jd_var *headers, jd_var *value);

int
cw_request_puts(CONTAINER_REQUEST *me, const char *str)
{
	return me->api->write(me, str, strlen(str));
}

const char *
cw_request_protocol(CONTAINER_REQUEST *me)
{
	return me->api->getenv(me, "SERVER_PROTOCOL");
}

const char *
cw_request_method(CONTAINER_REQUEST *me)
{
	return me->api->getenv(me, "REQUEST_METHOD");
}

int
cw_request_environment(jd_var *env, jd_var *out)
{
	jd_assign(out, env);
	return 0;
}

const char *
cw_request_getenv(jd_var *env, const char *name)
{
	const char *str;
	
	str = NULL;
	JD_SCOPE
	{
		jd_var *v;
		
		v = jd_get_ks(env, name, 0);
		if(v)
		{
			str = jd_bytes(v, NULL);
		}
	}
	return str;
}

int
cw_request_env_init(char *const *env, jd_var *out)
{
	size_t n;
	const char *t;
	
	memset(out, 0, sizeof(jd_var));
	jd_set_hash(out, 32);
	for(n = 0; env[n]; n++)
	{
		t = strchr(env[n], '=');
		if(!t)
		{
			continue;
		}
		JD_SCOPE
		{
			jd_var *kv, *key, *value;
			
			kv = jd_nsv(env[n]);
			key = jd_substr(jd_nv(), kv, 0, t - env[n]);
			value = jd_substr(jd_nv(), kv, t - env[n] + 1, strlen(env[n]));
			jd_assign(jd_get_key(out, key, 1), value);
		}
	}
	return (int) n;
}

int
cw_request_headers_init(jd_var *out)
{
	memset(out, 0, sizeof(jd_var));
	jd_set_hash(out, 32);
	cw_request_headers_set(out, "Status", "200 OK", 1);
	cw_request_headers_set(out, "Content-Type", "text/html", 1);
}

int
cw_request_headers_set(jd_var *headers, const char *name, const char *value, int replace)
{
	int r;
	
	JD_SCOPE
	{
		jd_var *n, *v;
		
		n = jd_nsv(name);
		if(value)
		{			
			v = jd_nsv(value);
		}
		else
		{
			v = NULL;
		}
		r = cw_request_headers_vset(headers, n, v, replace);
	}
	return r;
}

int
cw_request_headers_vset(jd_var *headers, jd_var *name, jd_var *value, int replace)
{
	char *s, *p;
	jd_var *hkey, *akey, *v;
	int r;
	
	/* XXX Stringify name */
	if((!value || value->type == VOID) && !replace)
	{
		/* Can't append nothing */
		errno = EINVAL;
		return -1;
	}
	if(!strcasecmp(jd_bytes(name, NULL), "status"))
	{
		if(!value || value->type == VOID || !replace)
		{
			/* You must send exactly one 'Status' header */
			errno = EPERM;
			return -1;
		}		
		/* Special-case handling for the 'Status' header */
		r = cw_request_headers_vset_status_(headers, value);
		if(r)
		{
			return r;
		}
	}
	s = strdup(jd_bytes(name, NULL));
	if(!s)
	{
		return -1;
	}
	/* Use a lowercased form of the name as the key */
	for(p = s; *p; p++)
	{
		*p = tolower(*p);
	}
	hkey = jd_get_ks(headers, s, 1);
	free(s);
	/* If it's not already an array, create one */
	if(hkey->type == VOID)
	{
		jd_set_array(hkey, 2);
		jd_push(hkey, 2);
	}
	/* The first element of the array is the name of the header, preserving
	 * case.
	*/
	akey = jd_get_idx(hkey, 0);		
	jd_assign(akey, name);
	/* The second element of the array is a list of headers */
	akey = jd_get_idx(hkey, 1);
	if(replace)
	{
		/* Empty the existing value */
		jd_set_array(akey, 1);
		if(!value || value->type == VOID)
		{
			return 0;
		}
	}
	else if(akey->type == VOID)
	{
		jd_set_array(akey, 1);
	}
	/* Append the value */
	v = jd_push(akey, 1);
	jd_assign(v, value);
	return 0;
}

int
cw_request_headers_status(jd_var *headers)
{
	jd_var *skey;
	
	skey = jd_get_ks(headers, "STATUS", 1);
	if(skey)
	{
		return jd_get_int(skey);
	}
	return 200;
}

int
cw_request_headers_send(jd_var *headers, int (*callback)(void *data, const char *name, const char *value, int more), void *data)
{
	jd_var keys = JD_INIT;
	jd_var *key, *hdata, *name, *values, *value;
	const char *p;
	size_t hcount, hi, vcount, vi;
	int r;
	
	jd_keys(&keys, headers);
	hcount = jd_count(&keys);
	for(hi = 0; hi < hcount; hi++)
	{
		key = jd_get_idx(&keys, hi);
		p = jd_bytes(key, NULL);
		if(isalpha(*p) && isupper(*p))
		{
			/* Headers stored with upper-case keys are used
			 * internally
			 */
			continue;
		}
		hdata = jd_get_key(headers, key, 0);
		name = jd_get_idx(hdata, 0);
		values = jd_get_idx(hdata, 1);
		vcount = jd_count(values);
		for(vi = 0; vi < vcount; vi++)
		{
			/* XXX stringify value */
			value = jd_get_idx(values, vi);
			r = callback(data, jd_bytes(name, NULL), jd_bytes(value, NULL), ((vi + 1) < vcount) ? 1 : 0); 
			/* Return -1 to stop processing this header, -2 to stop processing all headers */
			if(r < 0)
			{
				break;
			}
		}
		if(r == -2)
		{
			break;
		}
	}
	jd_release(&keys);
	return 0;
}

static int
cw_request_headers_vset_status_(jd_var *headers, jd_var *value)
{
	jd_var *skey;
	const char *s;
	char *p;
	long status;
	
	s = jd_bytes(value, NULL);
	p = NULL;
	status = strtol(s, &p, 10);
	if(status < 100 || status > 999 || (*p && !isspace(*p)))
	{
		errno = EINVAL;
		return -1;
	}
	skey = jd_get_ks(headers, "STATUS", 1);
	jd_set_int(skey, (jd_int) status);
	return 0;
}

int
cw_request_info_init(struct cw_request_info_struct *info, jd_var *env)
{
	jd_var *request_uri, *query_string, *v;
	const char *r, *s, *p;
	char *buf, *x;
	char hexbuf[3];
	long hexval;
	
	memset(&(info->request_vstr), 0, sizeof(jd_var));
	memset(&(info->request_array), 0, sizeof(jd_var));
	request_uri = jd_get_ks(env, "REQUEST_URI", 0);
	query_string = jd_get_ks(env, "QUERY_STRING", 0);
	s = NULL;
	if(request_uri)
	{
		r = jd_bytes(request_uri, NULL);
		/* Depending upon the source of the environment, the
		 * query-string may end up part of the REQUEST_URI
		 */
		s = strchr(r, '?');
		if(s)
		{
			jd_set_empty_string(&(info->request_vstr), s - r);
			jd_set_bytes(&(info->request_vstr), r, s - r);
		}
		else
		{
			jd_set_string(&(info->request_vstr), r);
		}
	}
	else
	{
		jd_set_empty_string(&(info->request_vstr), 0);
	}
	info->request_str = jd_bytes(&(info->request_vstr), NULL);
	info->request_uri = uri_create_str(info->request_str, NULL);
	jd_set_array(&(info->request_array), 8);
	if(strlen(info->request_str))
	{
		buf = (char *) malloc(strlen(info->request_str) + 1);
		hexbuf[2] = 0;
		for(r = info->request_str; *r; r++)
		{
			if(*r == '/')
			{
				continue;
			}
			for(p = r; *p && *p != '/'; p++);
			strncpy(buf, r, p - r);
			buf[p - r] = 0;
			
			for(x = buf; *x; x++)
			{
				if(x[0] == '%')
				{
					if(isxdigit(x[1]) && isxdigit(x[2]))
					{
						hexbuf[0] = x[1];
						hexbuf[1] = x[2];
						hexval = strtol(hexbuf, NULL, 16);
						*x = hexval;
						memmove(&(x[1]), &(x[3]), strlen(&(x[3])) + 1);
						continue;
					}
				}
			}
			v = jd_push(&(info->request_array), 1);
			jd_set_string(v, buf);
			if(!*p)
			{
				break;
			}
			r = p;
		}
		free(buf);
	}
	return 0;
}

int
cw_request_info_destroy(struct cw_request_info_struct *info)
{
	jd_release(&(info->request_vstr));
	jd_release(&(info->request_array));
	uri_destroy(info->request_uri);
	return 0;
}
