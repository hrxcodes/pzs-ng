#include <errno.h>
#include <fnmatch.h>
#include "zsfunctions.h"

#ifdef _WITH_SS5
#include "constants.ss5.h"
#else
#include "constants.h"
#endif

#include "convert.h"

#include <strl/strl.h>

struct dirent **dirlist;
struct dirent **dirlistp;
unsigned int	direntries = 0;
unsigned int	direntriesp = 0;
int		num_groups = 0, num_users = 0;
struct USER   **user;
struct GROUP  **group;

/*
 * d_log - create/put comments in a .debug file
 * Last revised by: js
 *        Revision: r1217
 */
void 
d_log(char *fmt,...)
{
#if ( debug_mode == TRUE )
	time_t		timenow;
	FILE           *file;
	va_list		ap;
#if ( debug_altlog == TRUE )
	static char	debugpath[PATH_MAX];
	static char	debugname[PATH_MAX];
#else
	static char	debugname[] = ".debug";
#endif
#endif

	if (fmt == NULL)
		return;
#if ( debug_mode == TRUE )
	va_start(ap, fmt);
	timenow = time(NULL);

#if ( debug_altlog == TRUE )
	getcwd(debugpath, PATH_MAX);
	sprintf(debugname, "%s/%s/.debug", storage, debugpath);
#endif

	if ((file = fopen(debugname, "a+"))) {
		fprintf(file, "%.24s - ", ctime(&timenow));
		vfprintf(file, fmt, ap);
		fclose(file);
	}
	chmod(debugname, 0666);
	va_end(ap);
#endif
	return;
}

/*
 * create_missing - create a <filename>-missing 0byte file.
 * Last revised by: js
 *        Revision: 1218
 */
void 
create_missing(char *f)
{
	char		fname[PATH_MAX];

	snprintf(fname, PATH_MAX, "%s-missing", f);
	createzerofile(fname);
}

/*
 * findfilext - find a filename with a matching extension in current dir.
 * Last Modified by: d1
 *         Revision: r1 (2002.01.16)
 */
char           *
findfileext(char *fileext)
{
	int		n, k;

	n = direntries;
	while (n--) {
		if ((k = NAMLEN(dirlist[n])) < 4)
			continue;
		if (strcasecmp(dirlist[n]->d_name + k - 4, fileext) == 0)
			return dirlist[n]->d_name;
	}
	return NULL;
}

/*
 * findfilextparent - find a filename with a matching extension in parent dir.
 * Last Modified by: psxc
 *         Revision: ?? (2004.10.06)
 */
char           *
findfileextparent(char *fileext)
{
	int		n         , k;

	n = direntriesp;
	while (n--) {
		if ((k = NAMLEN(dirlistp[n])) < 4)
			continue;
		if (strcasecmp(dirlistp[n]->d_name + k - 4, fileext) == 0) {
			return dirlistp[n]->d_name;
		}
	}
	return NULL;
}

/*
 * findfilextcount - count files with given extension
 * Last Modified by: daxxar
 *         Revision: ?? (2003.12.11)
 */
int 
findfileextcount(char *fileext)
{
	int		n         , fnamelen, c = 0;

	n = direntries;
	while (n--) {
		if ((fnamelen = NAMLEN(dirlist[n])) < 4)
			continue;
		if (!strcasecmp((dirlist[n]->d_name + fnamelen - 4), fileext))
			c++;
	}
	return c;
}

/*
 * hexstrtodec - make sure a valid hex number is present in sfv
 * Last modified by: psxc
 *         Revision: r1219
 */
unsigned int 
hexstrtodec(unsigned char *s)
{
	unsigned int	n = 0;
	unsigned char	r;

	if ((strlen(s) > 8 ) || (!strlen(s)))
		return 0;

	while (1) {
		if ((unsigned char)*s >= 48 && (unsigned char)*s <= 57) {
			r = 48;
		} else if ((unsigned char)*s >= 65 && (unsigned char)*s <= 70) {
			r = 55;
		} else if ((unsigned char)*s >= 97 && (unsigned char)*s <= 102) {
			r = 87;
		} else if ((unsigned char)*s == 0) {
			return n;
		} else {
			return 0;
		}
		n <<= 4;
		n += *s++ - r;
	}
}

/*
 * selector - dangling links
 * Last modified by: js(?)
 *         Revision: ??
 */
/*
 * dangling links
 */
#if defined(__linux__)
int 
selector(const struct dirent *d)
#elif defined(__NetBSD__)
	int		selector   (const struct dirent *d)
#else
int 
selector(struct dirent *d)
#endif
{
	struct stat	st;
	if ((stat(d->d_name, &st) < 0))
		return 0;
	return 1;
}

/*
 * rescandir - extract/store the dirlist for current dir
 * Last modified by: psxc
 *         Revision: r1220
 */

void 
rescandir(int usefree)
{
	if (direntries > 0 && usefree) {
		while (direntries--) {
			free(dirlist[direntries]);
		}
		free(dirlist);
	}
	if (usefree != 1) {
		direntries = scandir(".", &dirlist, selector, 0);
	}
}

/*
 * rescanparent - extract/store the dirlist for parent dir
 * Last modified by: psxc
 *         Revision: r1220
 */
void 
rescanparent(int usefree)
{
	if (direntriesp > 0 && usefree) {
		while (direntriesp--) {
			free(dirlistp[direntriesp]);
		}
		free(dirlistp);
	}
	if (usefree != 1) {
		direntriesp = scandir("..", &dirlistp, 0, 0);
	}
}


/*
 * del_releasedir - remove all files in current dir.
 * Last modified by: psxc
 *         Revision: ??
 */
void 
del_releasedir(char *relname)
{
	int	fnum = direntries;

	if (fnum > 0) {
		while (fnum--) {
			unlink(dirlist[fnum]->d_name);
		}
		rmdir(relname);
	}
}


/*
 * strtolower - make a string all lowercase
 * Last modified by: d1
 *         Revision: ?? (2002.01.16)
 */
void 
strtolower(char *s)
{
	while ((*s = tolower(*s)))
		s++;
}

/*
 * unlink_missing - remove <filename>-missing and <filename>.bad
 * Last modified by: psxc
 *         Revision: r1221
 */
void 
unlink_missing(char *s)
{
	char		t[PATH_MAX];
	int		n = 0;

	snprintf(t, PATH_MAX, "%s-missing", s);
	unlink(t);
	if ((n = findfile(t)))
		unlink(dirlist[n]->d_name);

	snprintf(t, PATH_MAX, "%s.bad", s);
	unlink(t);
	if ((n = findfile(t)))
		unlink(dirlist[n]->d_name);

}

/*
 * israr - define a file as rar.
 * Last modified by: d1
 *         Revision: ?? (2002.01.16)
 */
char 
israr(char *fileext)
{
	if ((*fileext == 'r' || *fileext == 's' || isdigit(*fileext)) &&
	    ((isdigit(*(fileext + 1)) && isdigit(*(fileext + 2))) ||
	     (*(fileext + 1) == 'a' && *(fileext + 2) == 'r')) &&
	    *(fileext + 3) == 0)
		return 1;
	return 0;
}

/*
 * Created    : 02.20.2002 Author     : dark0n3
 * 
 * Description: Checks if file is known mpeg/avi file
 */
char 
isvideo(char *fileext)
{
	switch (*fileext++) {
		case 'm':
			if (!memcmp(fileext, "pg", 3) ||
			    !memcmp(fileext, "peg", 4) ||
			    !memcmp(fileext, "2v", 3) ||
			    !memcmp(fileext, "2p", 3))
				return 1;
			break;
		case 'a':
			if (!memcmp(fileext, "vi", 3))
				return 1;
			break;
	}

	return 0;
}

/*
 * Modified: 2004-11-17 (psxc) - added support to modify the chars used in the progressbar
 */
void 
buffer_progress_bar(struct VARS *raceI)
{
	int		n;

	raceI->misc.progress_bar[14] = 0;
	if (raceI->total.files > 0) {
		for (n = 0; n < (raceI->total.files - raceI->total.files_missing) * 14 / raceI->total.files; n++)
			raceI->misc.progress_bar[n] = *charbar_filled;
		for (; n < 14; n++)
			raceI->misc.progress_bar[n] = *charbar_missing;
	}
}

/*
 * Modified: 01.16.2002
 */
void 
move_progress_bar(unsigned char delete, struct VARS *raceI, struct USERINFO **userI, struct GROUPINFO **groupI)
{
	char           *bar;
	char	       *delbar = 0;
	int		n, m = 0;
	regex_t		preg;
	regmatch_t	pmatch[1];

	delbar = convert5(del_progressmeter);
	d_log("del_progressmeter: %s\n", delbar);
	d_log("raceI->total.files: %i\n", raceI->total.files);
	d_log("raceI->total.files_missing: %i\n", raceI->total.files_missing);
	regcomp(&preg, delbar, REG_NEWLINE | REG_EXTENDED);
	/* workaround if progressbar was changed while zs-c is running */
	rescandir(2);

	n = direntries;

	if (delete) {
		while (n--) {
			if (regexec(&preg, dirlist[n]->d_name, 1, pmatch, 0) == 0) {
				if (!(int)pmatch[0].rm_so && (int)pmatch[0].rm_eo == (int)NAMLEN(dirlist[n])) {
					d_log("Found progress bar (%s), removing\n", dirlist[n]->d_name);
					remove(dirlist[n]->d_name);
					*dirlist[n]->d_name = 0;
					m = 1;
				}
			}
		}
		if (m) {
			regfree(&preg);
			return;
		} else {
			d_log("Progress bar could not be deleted, not found!\n");
		}
	} else {
		if (!raceI->total.files)
			return;
		bar = convert(raceI, userI, groupI, progressmeter);
		while (n--) {
			if (regexec(&preg, dirlist[n]->d_name, 1, pmatch, 0) == 0) {
				if (!(int)pmatch[0].rm_so && (int)pmatch[0].rm_eo == (int)NAMLEN(dirlist[n])) {
					if (!m) {
						d_log("Found progress bar (%s), renaming (to %s)\n", dirlist[n]->d_name, bar);
						rename(dirlist[n]->d_name, bar);
						m = 1;
					} else {
						d_log("Found (extra) progress bar (%s), removing\n", dirlist[n]->d_name);
						remove(dirlist[n]->d_name);
						*dirlist[n]->d_name = 0;
						m = 2;
					}
				}
			}
		}
		if (!m) {
			d_log("Progress bar could not be moved, creating a new one now!\n");
			createstatusbar(bar);
		}
	}
	regfree(&preg);
}

/*
 * Modified: Unknown
 */
short int 
findfile(char *filename)
{
	int		n = direntries;

	while (n--) {
#if (sfv_cleanup == TRUE)
#if (sfv_cleanup_lowercase == TRUE)
		if (!strcasecmp(dirlist[n]->d_name, filename)) {
#else
		if (!strcmp(dirlist[n]->d_name, filename)) {
#endif
#else
		if (!strcmp(dirlist[n]->d_name, filename)) {
#endif
			return n;
		}
	}
	return 0;
}

void
removedotfiles()
{
	int		n = direntries;

	while (n--) {
		if ((!strncasecmp(dirlist[n]->d_name, ".", 1)) && (strlen(dirlist[n]->d_name) > 2)) {
			unlink(dirlist[n]->d_name);
		}
	}
}

char           *
findfilename(char *filename)
{
	int	n;

	n = direntries;
	while (n--) {
		if (!strcasecmp(dirlist[n]->d_name, filename))
			return dirlist[n]->d_name;
	}
	return NULL;
}

/*
 * Modified: 01.16.2002
 */
void 
removecomplete()
{
	char	       *mydelbar = 0;
	int		n;
	regex_t		preg;
	regmatch_t	pmatch[1];

	if (message_file_name)
		unlink(message_file_name);
	mydelbar = convert5(del_completebar);
	d_log("del_completebar: %s\n", mydelbar);
	regcomp(&preg, mydelbar, REG_NEWLINE | REG_EXTENDED);
	n = direntries;
	while (n--) {
		if (regexec(&preg, dirlist[n]->d_name, 1, pmatch, 0) == 0) {
			if ((int)pmatch[0].rm_so == 0 && (int)pmatch[0].rm_eo == (int)NAMLEN(dirlist[n])) {
				remove(dirlist[n]->d_name);
				*dirlist[n]->d_name = 0;
			}
		}
	}
	regfree(&preg);
}

/*
 * Modified: 01.16.2002
 */
short int 
matchpath(char *instr, char *path)
{
	int		pos = 0;

	if ( strlen(instr) < 2 || strlen(path) < 2 )
		return 0;
	do {
		switch (*instr) {
		case 0:
		case ' ':
			if (!strncmp(instr - pos, path, pos - 1)) {
				return 1;
			}
			pos = 0;
			break;
		default:
			pos++;
			break;
		}
	} while (*instr++);
	return 0;
}

/*
 * Modified: 01.16.2002
 */
short int 
strcomp(char *instr, char *searchstr)
{
	int		pos = 0,	k;

	k = strlen(searchstr);

	if ( strlen(instr) == 0 || k == 0 )
		return 0;

	do {
		switch (*instr) {
		case 0:
		case ',':
			if (k == pos && !strncasecmp(instr - pos, searchstr, pos)) {
				return 1;
			}
			pos = 0;
			break;
		default:
			pos++;
			break;
		}
	} while (*instr++);
	return 0;
}

short int 
matchpartialpath(char *instr, char *path)
{
	int	pos = 0;
	char	partstring[PATH_MAX + 2];

	if ( strlen(instr) < 2 || strlen(path) < 2 )
		return 0;

	sprintf(partstring, "%s/", path);
	do {
		switch (*instr) {
		case 0:
		case ' ':
			if (!strncasecmp(instr - pos, partstring + strlen(partstring) - pos, pos)) {
				return 1;
			}
			pos = 0;
			break;
		default:
			pos++;
			break;
		}
	} while (*instr++);
	return 0;
}


/* check for matching subpath
   psxc - 2004-12-18
 */
short int 
subcomp(char *directory)
{
	int 	k = strlen(directory);
	int	m = strlen(subdir_list);
	int	pos = 0, l = 0, n = 0, j = 0;
	char	tstring[m + 1];

	if ( k < 2 )
		return 0;

	do {
		switch (subdir_list[l]) {
		case 0:
			break;
		case ',':
			tstring[j] = '\0';
			if (k <= j && !strncasecmp(tstring, directory, j - n)) {
				return 1;
			}
			pos = l;
			n = 0;
			j=0;
			break;
		case '?':
			tstring[j] = subdir_list[l];
			tstring[j+1] = '\0';
			n++;
			j++;
			break;
		default:
			tstring[j] = subdir_list[l];
			tstring[j+1] = '\0';
			pos++;
			j++;
			break;
		}
	m--;
	l++;
	} while (m);
	if (k <= j && !strncasecmp(tstring, directory, j - n)) {
		return 1;
	}
	return 0;
}

/* Checks if file exists */
short int 
fileexists(char *f)
{
	if (access(f, R_OK) == -1)
		return 0;
	return 1;

}

/* Create symbolic link (related to mp3 genre/year/group etc)
 * Last midified by: psxc
 *         Revision: r1228
 */
void 
createlink(char *factor1, char *factor2, char *source, char *ltarget)
{

#if ( userellink == 1 )
	char		result	[MAXPATHLEN];
#endif
	char		org	[PATH_MAX];
	char	       *target = org;
	int		l1 = strlen(factor1) + 1,
			l2 = strlen(factor2) + 1,
			l3 = strlen(ltarget) + 1;

	memcpy(target, factor1, l1);
	target += l1 - 1;
	if (*(target - 1) != '/') {
		*(target) = '/';
		target += 1;
	}
	memcpy(target, factor2, l2);
	target += l2;
	memcpy(target - 1, "/", 2);

	mkdir(org, 0777);

	//printf("\n%s\n%s\n\n", source, target);
#if ( userellink == 1 )
	abs2rel(source, org, result, MAXPATHLEN);
#endif

	memcpy(target, ltarget, l3);

#if ( userellink == 1 )
	symlink(result, org);
#else
	symlink(source, org);
#endif
}


void 
readsfv_ffile(struct VARS *raceI)
{
	int		fd, line_start = 0, index_start, ext_start, n;
	char           *buf, *fname;

	fd = open(raceI->file.name, O_RDONLY);
	buf = m_alloc(raceI->file.size + 2);
	read(fd, buf, raceI->file.size);
	close(fd);

	for (n = 0; n <= raceI->file.size; n++) {
		if (buf[n] == '\n' || n == raceI->file.size) {
			index_start = n - line_start;
			if (buf[line_start] != ';') {
				while (buf[index_start + line_start] != ' ' && index_start--);
				if (index_start > 0) {
					buf[index_start + line_start] = 0;
					fname = buf + line_start;
					ext_start = index_start;
#if (sfv_cleanup == TRUE)
#if (sfv_cleanup_lowercase == TRUE)
					while ((fname[ext_start] = tolower(fname[ext_start])) != '.' && ext_start > 0)
#else
					while (fname[ext_start] != '.' && ext_start > 0)
#endif
#else
					while (fname[ext_start] != '.' && ext_start > 0)
#endif
						ext_start--;
					if (fname[ext_start] != '.') {
						ext_start = index_start;
					} else {
						ext_start++;
					}
					index_start++;
					raceI->total.files++;
					if (!strcomp(ignored_types, fname + ext_start)) {
						if (findfile(fname)) {
							raceI->total.files_missing--;
						}
					}
				}
			}
			line_start = n + 1;
		}
	}
	raceI->total.files_missing = raceI->total.files + raceI->total.files_missing;
	m_free(buf);
}

void 
get_rar_info(struct VARS *raceI)
{
	FILE           *file;

	if ((file = fopen(raceI->file.name, "r"))) {
		fseek(file, 45, SEEK_CUR);
		fread(&raceI->file.compression_method, 1, 1, file);
//		d_log("DEBUG: raceI.file.compression_method : %d\n", raceI.file.compression_method);
		if ( ! (( 47 < raceI->file.compression_method ) && ( raceI->file.compression_method < 54 )) )
			raceI->file.compression_method = 88;
		fclose(file);
	}
}

/*
 * Modified   : 02.07.2002 Author     : dark0n3
 * 
 * Description: Executes extern program and returns return value
 * 
 * check execute_old for the... old version
 */
int 
execute(char *s)
{
	int		n;

	if ((n = system(s)) == -1)
		d_log("%s\n", strerror(errno));

	return n;

}

int 
execute_old(char *s)
{
	int		n;
	int		args = 0;
	char           *argv[128];	/* Noone uses this many args anyways */

	argv[0] = s;
	while (1) {
		if (*s == ' ') {
			*s = 0;
			args++;
			argv[args] = s + 1;
		} else if (*s == 0) {
			args++;
			argv[args] = NULL;
			break;
		}
		s++;
	}

	switch (fork()) {
	case 0:
		close(1);
		close(2);
		n = execv(argv[0], argv);
		exit(0);
		break;
	default:
		wait(&n);
		break;
	}

	return n >> 8;
}



/*
 * Copyright (c) 1997 Shigio Yamaguchi. All rights reserved. Copyright (c)
 * 1999 Tama Communications Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * abs2rel: convert an absolute path name into relative.
 * 
 * i)	path	absolute path
 * i)	base	base directory (must be absolute path)
 * o)	result	result buffer
 * i)	size	size of result buffer
 * r)		!= NULL: relative path == NULL: error
 */
char           *
abs2rel(path, base, result, size)
	const char     *path;
	const char     *base;
	char           *result;
	const size_t	size;
{
	const char     *pp, *bp, *branch;
	/*
	 * endp points the last position which is safe in the result buffer.
	 */
	const char     *endp = result + size - 1;
	char           *rp;

	if (*path != '/') {
		if (strlen(path) >= size)
			goto erange;
		strcpy(result, path);
		goto finish;
	} else if (*base != '/' || !size) {
		errno = EINVAL;
		return (NULL);
	} else if (size == 1)
		goto erange;
	/*
	 * seek to branched point.
	 */
	branch = path;
	for (pp = path, bp = base; *pp && *bp && *pp == *bp; pp++, bp++)
		if (*pp == '/')
			branch = pp;
	if ((*pp == 0 || (*pp == '/' && *(pp + 1) == 0)) &&
	    (*bp == 0 || (*bp == '/' && *(bp + 1) == 0))) {
		rp = result;
		*rp++ = '.';
		if (*pp == '/' || *(pp - 1) == '/')
			*rp++ = '/';
		if (rp > endp)
			goto erange;
		*rp = 0;
		goto finish;
	}
	if ((*pp == 0 && *bp == '/') || (*pp == '/' && *bp == 0))
		branch = pp;
	/*
	 * up to root.
	 */
	rp = result;
	for (bp = base + (branch - path); *bp; bp++)
		/* fixed by iwdisb */
		if (*bp == '/' && *(bp + 1) != 0 && *(bp + 1) != '/') {
			if (rp + 3 > endp)
				goto erange;
			*rp++ = '.';
			*rp++ = '.';
			*rp++ = '/';
		}
	if (rp > endp)
		goto erange;
	*rp = 0;
	/*
	 * down to leaf.
	 */
	if (*branch) {
		if (rp + strlen(branch + 1) > endp)
			goto erange;
		strcpy(rp, branch + 1);
	} else
		*--rp = 0;
finish:
	return result;
erange:
	errno = ERANGE;
	return (NULL);
}

char           *
get_g_name(int gid)
{
	int		n;
	for (n = 0; n < num_groups; n++)
		if ((int)group[n]->id / 100 == (int)gid / 100)
			return group[n]->name;
	return "NoGroup";
}

char           *
get_u_name(int uid)
{
	int		n;
	for (n = 0; n < num_users; n++)
		if (user[n]->id == (unsigned int)uid)
			return user[n]->name;
	return "Unknown";
}

/* Buffer groups file */
int 
buffer_groups(char *groupfile, int setfree)
{
	char           *f_buf, *g_name;
	gid_t		g_id;
	off_t		f_size;
	int		f         , n, m, g_n_size, l_start = 0;
	int		GROUPS = 0;
	struct stat	fileinfo;

	if (setfree != 0) {
		for (n = 0; n < setfree; n++) {
			free(group[n]->name);
			free(group[n]);
		}
		free(group);
		return 0;
	}

	f = open(groupfile, O_NONBLOCK);

	fstat(f, &fileinfo);
	f_size = fileinfo.st_size;
	f_buf = malloc(f_size);
	read(f, f_buf, f_size);

	for (n = 0; n < f_size; n++)
		if (f_buf[n] == '\n')
			GROUPS++;
	group = malloc(GROUPS * sizeof(struct GROUP *));

	for (n = 0; n < f_size; n++) {
		if (f_buf[n] == '\n' || n == f_size) {
			f_buf[n] = 0;
			m = l_start;
			while (f_buf[m] != ':' && m < n)
				m++;
			if (m != l_start) {
				f_buf[m] = 0;
				g_name = f_buf + l_start;
				g_n_size = m - l_start;
				m = n;
				while (f_buf[m] != ':' && m > l_start)
					m--;
				f_buf[m] = 0;
				while (f_buf[m] != ':' && m > l_start)
					m--;
				if (m != n) {
					g_id = atoi(f_buf + m + 1);
					group[num_groups] = malloc(sizeof(struct GROUP));
					group[num_groups]->name = malloc(g_n_size + 1);
					strcpy(group[num_groups]->name, g_name);
					group[num_groups]->id = g_id;
					num_groups++;
				}
			}
			l_start = n + 1;
		}
	}

	close(f);
	free(f_buf);
	return num_groups;
}

/* Buffer users file */
int
buffer_users(char *passwdfile, int setfree)
{
	char           *f_buf, *u_name;
	uid_t		u_id;
	off_t		f_size;
	int		f, n, m, l, u_n_size, l_start = 0;
	int		USERS = 0;
	struct stat	fileinfo;

	if (setfree != 0) {
		for (n = 0; n < setfree; n++) {
			free(user[n]->name);
			free(user[n]);
		}
		free(user);
		return 0;
	}

	f = open(passwdfile, O_NONBLOCK);
	fstat(f, &fileinfo);
	f_size = fileinfo.st_size;
	f_buf = malloc(f_size);
	read(f, f_buf, f_size);

	for (n = 0; n < f_size; n++)
		if (f_buf[n] == '\n')
			USERS++;
	user = malloc(USERS * sizeof(struct USER *));

	for (n = 0; n < f_size; n++) {
		if (f_buf[n] == '\n' || n == f_size) {
			f_buf[n] = 0;
			m = l_start;
			while (f_buf[m] != ':' && m < n)
				m++;
			if (m != l_start) {
				f_buf[m] = 0;
				u_name = f_buf + l_start;
				u_n_size = m - l_start;
				m = n;
				for (l = 0; l < 4; l++) {
					while (f_buf[m] != ':' && m > l_start)
						m--;
					f_buf[m] = 0;
				}
				while (f_buf[m] != ':' && m > l_start)
					m--;
				if (m != n) {
					u_id = atoi(f_buf + m + 1);
					user[num_users] = malloc(sizeof(struct USER));
					user[num_users]->name = malloc(u_n_size + 1);
					strcpy(user[num_users]->name, u_name);
					user[num_users]->id = u_id;
					num_users++;
				}
			}
			l_start = n + 1;
		}
	}

	close(f);
	free(f_buf);
	return(num_users);
}

/*
 * get the sum of same filetype Done by psxc 2004, Oct 6th
 */

/*
unsigned long
sample_dir(char *dirname)
{
	int		n, k = 0;
	unsigned long	l = 0;
	struct stat	filestat;

	n = direntries;
	while (n--) {
		if (samplecmp(dirlist[n]->d_name, sampledirs)) {
			
			continue;
		} else
			continue;



		if ((k = NAMLEN(dirlist[n])) < 4)
			continue;
		if (strcasecmp(dirlist[n]->d_name + k - 4, fileext) == 0) {
			if (stat(dirlist[n]->d_name, &filestat) != 0)
				filestat.st_size = 1;
			l = l + filestat.st_size;
			continue;
		}
	}
	if (!(l = l - fsize) > 0)
		l = 0;
	return l;
}
	
 */

unsigned long 
sfv_compare_size(char *fileext, unsigned long fsize)
{
	int		n, k = 0;
	unsigned long	l = 0;
	struct stat	filestat;

	n = direntries;
	while (n--) {
		if ((k = NAMLEN(dirlist[n])) < 4)
			continue;
		if (strcasecmp(dirlist[n]->d_name + k - 4, fileext) == 0) {
			if (stat(dirlist[n]->d_name, &filestat) != 0)
				filestat.st_size = 1;
			l = l + filestat.st_size;
			continue;
		}
	}
	if (!(l = l - fsize) > 0)
		l = 0;
	return l;
}

void
mark_as_bad(char *filename)
{

#if (mark_file_as_bad == TRUE)
	char	newname[PATH_MAX];

	if (!fileexists(filename)) {
		d_log("Debug: Failed to open file \"%s\"\n", filename);
		return;
	}
	sprintf(newname, "%s.bad", filename);
	if (rename(filename, newname)) {
		d_log("Error - failed to rename %s to %s.\n", filename, newname);
	} else {
		createzerofile(filename);
		chmod(newname, 0644);
	}
#endif
	d_log("File (%s) marked as bad.\n", filename);
}

void 
writelog(GLOBAL *g, char *msg, char *status)
{
	FILE           *glfile;
	char           *date;
	char           *line, *newline;
	time_t		timenow;

	if (g->v.misc.write_log == TRUE && !matchpath(group_dirs, g->l.path)) {
		timenow = time(NULL);
		date = ctime(&timenow);
		if (!(glfile = fopen(log, "a+"))) {
			d_log("Unable to open %s for read/write (append) - NO RACEINFO WILL BE WRITTEN!\n", log);
			return;
		}
		line = newline = msg;
		while (1) {
			switch (*newline++) {
			case 0:
				fprintf(glfile, "%.24s %s: \"%s\" %s\n", date, status, g->l.path, line);
				fclose(glfile);
				return;
			case '\n':
				fprintf(glfile, "%.24s %s: \"%s\" %.*s\n", date, status, g->l.path, (int)(newline - line - 1), line);
				line = newline;
				break;
			}
		}
	}
}

void
buffer_paths(GLOBAL *g, char path[2][PATH_MAX], int *k, int len)
{
	int		cnt, n = 0;

	for (cnt = len; *k && cnt; cnt--) {
		if (g->l.path[cnt] == '/') {
			(*k)--;
			strlcpy(path[*k], g->l.path + cnt + 1, n+1);
			path[*k][n] = 0;
			n = 0;
		} else {
			n++;
		}
	}
}

void 
remove_nfo_indicator(GLOBAL *g)
{
	int		k = 2;
	char		path[2][PATH_MAX];

	buffer_paths(g, path, &k, (strlen(g->l.path)-1));

	g->l.nfo_incomplete = i_incomplete(incomplete_nfo_indicator, path, &g->v);
	if (fileexists(g->l.nfo_incomplete))
		unlink(g->l.nfo_incomplete);
	g->l.nfo_incomplete = i_incomplete(incomplete_base_nfo_indicator, path, &g->v);
	if (fileexists(g->l.nfo_incomplete))
		unlink(g->l.nfo_incomplete);
}

void 
getrelname(GLOBAL *g)
{
	int		k = 2, subc;
	char		path[2][PATH_MAX];

	buffer_paths(g, path, &k, (strlen(g->l.path)-1));

	subc = subcomp(path[1]);
	
	d_log("getrelname():\tsubc:\t\t%d\n", subc);
	d_log("\t\t\tpath[0]:\t%s\n", path[0]);
	d_log("\t\t\tpath[1]:\t%s\n", path[1]);
	d_log("\t\t\tg->l_path:\t%s\n", path[1]);

	if (subc) {
		snprintf(g->v.misc.release_name, PATH_MAX, "%s/%s", path[0], path[1]);
		strlcpy(g->l.link_source, g->l.path, PATH_MAX);
		strlcpy(g->l.link_target, path[1], PATH_MAX);
		g->l.incomplete = c_incomplete(incomplete_cd_indicator, path, &g->v);
		g->l.nfo_incomplete = i_incomplete(incomplete_base_nfo_indicator, path, &g->v);
		g->l.in_cd_dir = 1;
	} else {
		strlcpy(g->v.misc.release_name, path[1], PATH_MAX);
		strlcpy(g->l.link_source, g->l.path, PATH_MAX);
		strlcpy(g->l.link_target, path[1], PATH_MAX);
		g->l.incomplete = c_incomplete(incomplete_indicator, path, &g->v);
		g->l.nfo_incomplete = i_incomplete(incomplete_nfo_indicator, path, &g->v);
		g->l.in_cd_dir = 0;
	}
	
	d_log("\t\t\tlink_source:\t%s\n", g->l.link_source);
	d_log("\t\t\tlink_target:\t%s\n", g->l.link_target);
}

unsigned char 
get_filetype(GLOBAL *g, char *ext)
{
	if (!strcasecmp(ext, "zip"))
		return 0;
	if (!strcasecmp(ext, "sfv"))
		return 1;
	if (!strcasecmp(ext, "nfo"))
		return 2;
	if (strcomp(allowed_types, ext) && !matchpath(allowed_types_exemption_dirs, g->l.path))
		return 4;
	if (!strcomp(ignored_types, ext))
		return 3;

	return 255;
}

#if ( audio_group_sort == TRUE )
char    *
remove_pattern(param, pattern, op)
	char           *param, *pattern;
	int		op;
{
	register int	len;
	register char  *end;
	register char  *p, *ret, c;

	if (param == NULL || *param == '\0')
		return (param);
	if (pattern == NULL || *pattern == '\0')	/* minor optimization */
		return (param);

	len = strlen(param);
	end = param + len;

	switch (op) {
	case RP_LONG_LEFT:	/* remove longest match at start */
		for (p = end; p >= param; p--) {
			c = *p;
			*p = '\0';
			if ((fnmatch(pattern, param, 0)) != FNM_NOMATCH) {
				*p = c;
				return (p);
			}
			*p = c;
		}
		break;

	case RP_SHORT_LEFT:	/* remove shortest match at start */
		for (p = param; p <= end; p++) {
			c = *p;
			*p = '\0';
			if (fnmatch(pattern, param, 0) != FNM_NOMATCH) {
				*p = c;
				return (p);
			}
			*p = c;
		}
		break;


	case RP_LONG_RIGHT:	/* remove longest match at end */
		for (p = param; p <= end; p++) {
			if (fnmatch(pattern, param, 0) != FNM_NOMATCH) {
				c = *p;
				*p = '\0';
				ret = param;
				*p = c;
				return (ret);
			}
		}
		break;

	case RP_SHORT_RIGHT:	/* remove shortest match at end */
		for (p = end; p >= param; p--) {
			if (fnmatch(pattern, param, 0) != FNM_NOMATCH) {
				c = *p;
				*p = '\0';
				ret = param;
				*p = c;
				return (ret);
			}
		}
		break;
	}
	return (param);		/* no match, return original string */
}
#endif

