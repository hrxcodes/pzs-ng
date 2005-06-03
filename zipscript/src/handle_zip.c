#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "handle_zip.h"
#include "zsfunctions.h"
#include "convert.h"
#include "race-file.h"
#include "errors.h"
#include "dizreader.h"

int
handle_zip(HANDLER_ARGS *ha) {

	unsigned char	exit_value = EXIT_SUCCESS;
	char		target[strlen(unzip_bin) + 10 + NAME_MAX];
	long		loc;

	d_log(1, "handle_zip: File type is: ZIP\n");
	d_log(1, "handle_zip: Testing file integrity with %s\n", unzip_bin);
	if (!fileexists(unzip_bin)) {
		d_log(1, "handle_zip: ERROR! Not able to check zip-files - %s does not exist!\n", unzip_bin);
		sprintf(ha->g->v.misc.error_msg, BAD_ZIP);
		mark_as_bad(ha->g->v.file.name);
		ha->msg->error = convert(&ha->g->v, ha->g->ui, ha->g->gi, bad_file_msg);
		if (exit_value < 2)
			writelog(ha->g, ha->msg->error, bad_file_zip_type);
		exit_value = 2;
		return exit_value;
	} else {
#if (test_for_password || extract_nfo)
		if ((!findfileextcount(".", ".nfo") || findfileextcount(".", ".zip") == 1) && !mkdir(".unzipped", 0777)) {
			sprintf(target, "%s -qqjo \"%s\" -d .unzipped", unzip_bin, ha->g->v.file.name);
		} else
			sprintf(target, "%s -qqt \"%s\"", unzip_bin, ha->g->v.file.name);
#else
		sprintf(target, "%s -qqt \"%s\"", unzip_bin, ha->g->v.file.name);
#endif
		if (execute(target) != 0) {
			d_log(1, "handle_zip: Integrity check failed (#%d): %s\n", errno, strerror(errno));
			sprintf(ha->g->v.misc.error_msg, BAD_ZIP);
			mark_as_bad(ha->g->v.file.name);
			ha->msg->error = convert(&ha->g->v, ha->g->ui, ha->g->gi, bad_file_msg);
			if (exit_value < 2)
				writelog(ha->g, ha->msg->error, bad_file_zip_type);
			exit_value = 2;
			return exit_value;
		}
#if (test_for_password || extract_nfo || zip_clean)
			if (!findfileextcount(".", ".nfo") || findfileextcount(".", ".zip") == 1) {
				if (check_zipfile(".unzipped", ha->g->v.file.name)) {
					d_log(1, "handle_zip: File is password protected.\n");
					sprintf(ha->g->v.misc.error_msg, PASSWORD_PROTECTED);
					ha->msg->error = convert(&ha->g->v, ha->g->ui, ha->g->gi, bad_file_msg);
					if (exit_value < 2)
						writelog(ha->g, ha->msg->error, bad_file_password_type);
					exit_value = 2;
					return exit_value;
				}
			}
#endif
	}
	d_log(1, "handle_zip: Integrity ok\n");
	printf(zipscript_zip_ok);

	if ((matchpath(zip_dirs, ha->g->l.path)) || (matchpath(group_dirs, ha->g->l.path))  ) {
		d_log(1, "handle_zip: Directory matched with zip_dirs/group_dirs\n");
	} else {
		d_log(1, "handle_zip: WARNING! Directory did not match with zip_dirs/group_dirs\n");
		if (strict_path_match == TRUE) {
			d_log(1, "handle_zip: Strict mode on - exiting\n");
			sprintf(ha->g->v.misc.error_msg, UNKNOWN_FILE, ha->fileext);
			mark_as_bad(ha->g->v.file.name);
			ha->msg->error = convert(&ha->g->v, ha->g->ui, ha->g->gi, bad_file_msg);
			if (exit_value < 2)
				writelog(ha->g, ha->msg->error, bad_file_wrongdir_type);
			exit_value = 2;
			return exit_value;
		}
	}
	if (!fileexists("file_id.diz")) {
		d_log(1, "handle_zip: file_id.diz does not exist, trying to extract it from %s\n", ha->g->v.file.name);
		sprintf(target, "%s -qqjnCLL \"%s\" file_id.diz 2>.delme", unzip_bin, ha->g->v.file.name);
		if (execute(target) != 0)
			d_log(1, "handle_zip: No file_id.diz found (#%d): %s\n", errno, strerror(errno));
		else {
			if ((loc = findfile(".", "file_id.diz.bad")))
				remove_at_loc(".", loc);
			chmod("file_id.diz", 0666);
		}
	}
	d_log(1, "handle_zip: Reading diskcount from diz:\n");
	ha->g->v.total.files = read_diz("file_id.diz");
	d_log(1, "handle_zip:    Expecting %d files.\n", ha->g->v.total.files);

	if (ha->g->v.total.files == 0) {
		d_log(1, "handle_zip:    Could not get diskcount from diz.\n");
		ha->g->v.total.files = 1;
		unlink("file_id.diz");
	}
	ha->g->v.total.files_missing = ha->g->v.total.files;

	d_log(1, "handle_zip: Storing new race data\n");
	writerace(ha->g->l.race, &ha->g->v, 0, F_CHECKED);
	d_log(1, "handle_zip: Reading race data from file to memory\n");
	readrace(ha->g->l.race, &ha->g->v, ha->g->ui, ha->g->gi);
	if (ha->g->v.total.files_missing < 0) {
		d_log(1, "handle_zip: There seems to be more files in zip than we expected\n");
		ha->g->v.total.files -= ha->g->v.total.files_missing;
		ha->g->v.total.files_missing = 0;
		ha->g->v.misc.write_log = FALSE;
	}
	d_log(1, "handle_zip: Setting message pointers\n");
	ha->msg->race = zip_race;
	ha->msg->update = zip_update;
	ha->msg->halfway = CHOOSE(ha->g->v.total.users, zip_halfway, zip_norace_halfway);
	ha->msg->newleader = zip_newleader;

	return exit_value;
}