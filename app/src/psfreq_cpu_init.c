/**
 * @file psfreq_cpu_init.c
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
 * Part of the implementation for the psfreq_cpu_type. This module
 * handles the essential setup when a new psfreq_cpu_type is initialized.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psfreq_cpu.h"
#include "psfreq_log.h"
#include "psfreq_strings.h"
#include "psfreq_util.h"

static uint8_t psfreq_cpu_init_find_number_cpus(void);
static bool psfreq_cpu_init_system_has_pstate(
                const struct psfreq_sysfs_type *sysfs);
static uint32_t psfreq_cpu_init_system_max_freq(
                const struct psfreq_sysfs_type *sysfs);
static uint32_t psfreq_cpu_init_system_min_freq(
                const struct psfreq_sysfs_type *sysfs);
static char **psfreq_cpu_init_vector(const struct psfreq_cpu_type *cpu,
                const char *const what);

void psfreq_cpu_init(struct psfreq_cpu_type *cpu,
                const struct psfreq_sysfs_type *sysfs)
{
        cpu->cpu_num = psfreq_cpu_init_find_number_cpus();
        cpu->has_pstate = psfreq_cpu_init_system_has_pstate(sysfs);
        cpu->cpuinfo_max_freq = psfreq_cpu_init_system_max_freq(sysfs);
        cpu->cpuinfo_min_freq = psfreq_cpu_init_system_min_freq(sysfs);
        cpu->vector_scaling_min_freq = psfreq_cpu_init_vector(cpu, "min_freq");
        cpu->vector_scaling_max_freq = psfreq_cpu_init_vector(cpu, "max_freq");
        cpu->vector_scaling_governor = psfreq_cpu_init_vector(cpu, "governor");
}

void psfreq_cpu_destroy(struct psfreq_cpu_type *cpu)
{
                psfreq_log_debug("psfreq_cpu_destroy",
                                "Free all allocated memory");
        for (uint8_t i = 0; i < cpu->cpu_num; ++i) {
                psfreq_log_debug("psfreq_cpu_destroy",
                                "free vector_scaling_min_freq[%u]", i);
                free(cpu->vector_scaling_min_freq[i]);

                psfreq_log_debug("psfreq_cpu_destroy",
                                "free vector_scaling_max_freq[%u]", i);
                free(cpu->vector_scaling_max_freq[i]);

                psfreq_log_debug("psfreq_cpu_destroy",
                                "free vector_scaling_governor[%u]", i);
                free(cpu->vector_scaling_governor[i]);
        }

        psfreq_log_debug("psfreq_cpu_destroy",
                        "free vector_scaling_min_freq");
        free(cpu->vector_scaling_min_freq);

        psfreq_log_debug("psfreq_cpu_destroy",
                        "free vector_scaling_max_freq");
        free(cpu->vector_scaling_max_freq);

        psfreq_log_debug("psfreq_cpu_destroy",
                        "free vector_scaling_governor");
        free(cpu->vector_scaling_governor);
}

/*
 * Find the total number of CPUS (logical and physical) that exist on the
 * system.
 */
static uint8_t psfreq_cpu_init_find_number_cpus(void)
{
        const char *const cmd = "grep processor /proc/cpuinfo | wc -l";

        uint8_t size = 1;
        char **res = psfreq_util_read_pipe(cmd, &size);
        if (res == NULL) {
                psfreq_log_error("psfreq_cpu_init_find_number_cpus",
                                "Failed to find number of cpus");
                return 0;
        }

        const uint32_t n = psfreq_strings_to_uint(res[0]);
        psfreq_log_debug("psfreq_cpu_init_find_number_cpus",
                        "Free memory held by res");
        free(res[0]);
        free(res);
        psfreq_log_debug("psfreq_cpu_init_find_number_cpus",
                        "Number of cpus: %u", n);
        return n;
}

static bool psfreq_cpu_init_system_has_pstate(
                const struct psfreq_sysfs_type *sysfs)
{
        char *driver = psfreq_sysfs_read(sysfs,
                        "cpu0/cpufreq/scaling_driver");
        if (driver == NULL) {
                psfreq_log_error("psfreq_cpu_init_system_has_pstate",
                                "Unable to check for intel_pstate driver");
                return false;
        }

        const char *cmp = "intel_pstate\n";
        psfreq_log_debug("psfreq_cpu_init_system_has_pstate",
                        "Compare driver '%s' with '%s'", driver, cmp);
        const bool r = (strcmp(driver, cmp) == 0);
        free(driver);
        return r;
}

static uint32_t psfreq_cpu_init_system_max_freq(
                const struct psfreq_sysfs_type *sysfs)
{
        char *line = psfreq_sysfs_read(sysfs, "cpu0/cpufreq/cpuinfo_max_freq");
        if (line == NULL) {
                psfreq_log_error("psfreq_cpu_init_system_max_freq",
                                "Unable to read for cpuinfo_max_freq");
                return 0;
        }
        const uint32_t result = psfreq_strings_to_uint(line);
        if (result == 0) {
                psfreq_log_error("psfreq_cpu_init_system_max_freq",
                                "Unable to convert string '%s' to unit32_t",
                                line);
        }

        free(line);
        return result;
}

static uint32_t psfreq_cpu_init_system_min_freq(
                const struct psfreq_sysfs_type *sysfs)
{
        char *line = psfreq_sysfs_read(sysfs, "cpu0/cpufreq/cpuinfo_min_freq");
        if (line == NULL) {
                psfreq_log_error("psfreq_cpu_init_system_min_freq",
                                "Unable to read for cpuinfo_min_freq");
                return 0;
        }
        const uint32_t result = psfreq_strings_to_uint(line);
        if (result == 0) {
                psfreq_log_error("psfreq_cpu_init_system_min_freq",
                                "Unable to convert string '%s' to unit32_t",
                                line);
        }

        free(line);
        return result;
}

static char **psfreq_cpu_init_vector(const struct psfreq_cpu_type *cpu,
                const char *const what)
{
        psfreq_log_debug("psfreq_cpu_init_vector", "Check for non-zero size");
        const uint8_t num = cpu->cpu_num;
        if (num == 0) {
                psfreq_log_error("psfreq_cpu_init_vector",
                                "Size is 0, failed to find cpu number");
                return NULL;
        }

        psfreq_log_debug("psfreq_cpu_init_vector",
                        "malloc for vector");
        char **vector = malloc(num * sizeof(char *));
        if (vector == NULL) {
                psfreq_log_error("psfreq_cpu_init_vector",
                                "Failed to malloc for vector");
                return NULL;
        }

        for (uint8_t i = 0; i < num; ++i) {
                char *buf = NULL;
                if (asprintf(&buf, "cpu%u/cpufreq_scaling_%s", i, what) < 0) {
                        psfreq_log_error("psfreq_cpu_init_vector",
                                "asprintf returned a -1, indicating a failure "
                                "during\n either memory allocation or some "
                                "other error.");
                        return NULL;
                }

                psfreq_log_debug("psfreq_cpu_init_vector",
                                "assign '%s' to vector index %d", buf, i);
                vector[i] = buf;
        }
        return vector;
}
