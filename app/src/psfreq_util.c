/**
 * @file psfreq_util.c
 * @author pyamsoft <pyam(dot)soft(at)gmail(dot)com>
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @section DESCRIPTION
 * Utility functions file to help with things like reading from command
 * pipes.
 */

#define _GNU_SOURCE
#include <stdio.h>

#include "psfreq_log.h"
#include "psfreq_strings.h"
#include "psfreq_util.h"

char **psfreq_util_read_pipe(const char *const cmd, const unsigned char *size)
{
        char **lines;
        FILE *pipe;
        size_t n;
        unsigned char i;
        psfreq_log_debug("psfreq_read_pipe", "Check for non-NULL cmd");
        if (cmd == NULL) {
                psfreq_log_error("psfreq_read_pipe",
                                "cmd is NULL");
                return NULL;
        }
        psfreq_log_debug("psfreq_read_pipe", "Check for non-zero size");
        if (*size == 0) {
                psfreq_log_error("psfreq_read_pipe",
                                "Size is 0, which would"
                                " result in empty array");
                return NULL;
        }

        psfreq_log_debug("psfreq_read_pipe", "malloc for lines");
        lines = malloc(*size * sizeof(char *));
        if (lines == NULL) {
                psfreq_log_error("psfreq_read_pipe",
                                "Failed to malloc for lines");
                return NULL;
        }

        psfreq_log_debug("psfreq_read_pipe",
                        "Attempt to open pipe '%s'", cmd);
        pipe = popen(cmd, "r");
        if (pipe == NULL) {
                psfreq_log_error("psfreq_read_pipe",
                                "Failed to open pipe '%s'", cmd);
                free(lines);
                return NULL;
        }
        n = 0;
        for (i = 0; i < *size; ++i) {
                char *line = NULL;
                psfreq_log_debug("psfreq_read_pipe",
                                "Attempt to getline from pipe");
                if (getline(&line, &n, pipe) < 0) {
                        psfreq_log_error("psfreq_read_pipe",
                                        "Failed to read from pipe");
                        free(lines);
                        free(line);
                        pclose(pipe);
                        return NULL;
                } else {
                        psfreq_log_debug("psfreq_read_pipe",
                                        "Assign line '%s' to array %d",
                                        line, i);
                        line = psfreq_strings_strip_end(line);
                        lines[i] = line;
                }
        }
        psfreq_log_debug("psfreq_read_pipe", "Close pipe");
        pclose(pipe);
        return lines;
}

char *psfreq_util_read2(const char *base, const char *file)
{
        char *r;
        char *abs_path;
        psfreq_log_debug("psfreq_sysfs_read",
                        "Concat strings: '%s' and '%s'",
                        base, file);
        abs_path = psfreq_strings_concat(base, file);
        if (abs_path == NULL) {
                psfreq_log_error("psfreq_sysfs_read",
                                "Concat strings: '%s' and '%s' has failed.\n"
                                "Function will return false.",
                                base, file);
                return NULL;
        }
        r = psfreq_util_read(abs_path);
        free(abs_path);
        return r;
}

char *psfreq_util_read(const char *abs_path)
{
        FILE *f;
        char *line;
        size_t n;

        psfreq_log_debug("psfreq_sysfs_read",
                        "Attempt to open file: '%s'",
                        abs_path);
        f = fopen(abs_path, "r");
        if (f == NULL) {
                psfreq_log_error("psfreq_sysfs_read",
                                "File '%s' failed to open for reading.",
                                abs_path);
                return NULL;
        }

        psfreq_log_debug("psfreq_sysfs_read",
                        "Attempt to read buffer from file: '%s'",
                        abs_path);
        line = NULL;
        n = 0;
        psfreq_log_debug("psfreq_sysfs_read",
                        "Getting a line from file %s\n", abs_path);
        if (getline(&line, &n, f) < 0) {
                psfreq_log_error("psfreq_sysfs_read",
                                "Failed to read buffer from file '%s'.",
                                abs_path);
                free(line);
                fclose(f);
                return NULL;
        }

        line = psfreq_strings_strip_end(line);

        psfreq_log_debug("psfreq_sysfs_read",
                        "Close file: '%s'",
                        abs_path);
        fclose(f);
        return line;
}

unsigned char psfreq_util_write(const char *abs_path, const char *buf)
{
        FILE *f;
        psfreq_log_debug("psfreq_sysfs_write",
                        "Check that buf is not NULL");
        if (buf == NULL) {
                psfreq_log_error("psfreq_sysfs_write",
                                "buf is NULL, exit.");
                return 0;
        }
        psfreq_log_debug("psfreq_sysfs_write",
                        "Attempt to open file: '%s'",
                        abs_path);
        f = fopen(abs_path, "w");
        if (f == NULL) {
                psfreq_log_error("psfreq_sysfs_write",
                                "File '%s' failed to open for writing.",
                                abs_path);
                return 0;
        }

        psfreq_log_debug("psfreq_sysfs_write",
                        "Attempt to write buffer '%s' to file: '%s'",
                        buf, abs_path);
        if (fprintf(f, "%s\n", buf) < 0) {
                psfreq_log_error("psfreq_sysfs_write",
                                "Failed to write buffer: %s to file '%s'.",
                                buf, abs_path);
                fclose(f);
                return 0;
        }

        psfreq_log_debug("psfreq_sysfs_write",
                        "Close file: '%s'",
                        abs_path);
        fclose(f);
        return 1;
}

unsigned char psfreq_util_write2(const char *base, const char *file,
                                                   const char *buf)
{
	char *abs_path;
	unsigned char r;
        psfreq_log_debug("psfreq_sysfs_write",
                        "Concat strings: '%s' and '%s'",
                        base, file);
        abs_path = psfreq_strings_concat(base, file);
        if (abs_path == NULL) {
                psfreq_log_error("psfreq_sysfs_write",
                                "Concat strings: '%s' and '%s' has failed.\n"
                                "Function will return false.",
                                base, file);
                return 0;
        }

	r = psfreq_util_write(abs_path, buf);
	free(abs_path);
	return r;
}

unsigned char psfreq_util_write_num(const char *abs_path, const int *num)
{
        char *s = psfreq_strings_from_int(num);
        const unsigned char r = psfreq_util_write(abs_path, s);
        free(s);
        return r;
}
unsigned char psfreq_util_write_num2(const char *base, const char *file,
                                                   const int *num)
{
        char *s = psfreq_strings_from_int(num);
        const unsigned char r = psfreq_util_write2(base, file, s);
        free(s);
        return r;
}
