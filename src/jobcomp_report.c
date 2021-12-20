#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "slurm/slurm_errno.h"
#include "src/common/gres.h"
#include "src/common/slurm_jobcomp.h"
#include "src/slurmctld/slurmctld.h"

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  Slurm uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "jobcomp" for Slurm job completion logging) and <method>
 * is a description of how this plugin satisfies that application.  Slurm will
 * only load job completion logging plugins if the plugin_type string has a
 * prefix of "jobcomp/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */
const char plugin_name[] = "Job completion reporting plugin";
const char plugin_type[] = "jobcomp/report";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

typedef struct report {
  /** @brief A Slurm Job ID. */
  uint32_t job_id;
  /** @brief A UNIX user ID. */
  uint32_t user_id;
  /** @brief A Slurm Cluster name. */
  char *cluster;
  /** @brief A Slurm Partition name. */
  char *partition;
  /** @brief A Slurm Job state. */
  uint32_t job_state;
  /** @brief The allocated CPUs. */
  uint64_t cpu;
  /** @brief The alloctaed memory in MB. */
  uint64_t mem;
  /** @brief The allocated GPUs. */
  uint64_t gpu;
  /** @brief The billing tres factor. */
  uint64_t billing;
  /** @brief The job start timestamp. */
  time_t start_time;
  /** @brief The job end timestamp. */
  time_t end_time;
  /** @brief The job duration. */
  time_t elapsed;
  /** @brief The name of the Qos. */
  char *qos_name;
  /** @brief The usage factor of the Qos. */
  double usage_factor;
  /**
   * @brief The total cost.
   *
   * total_cost = round((usage_factor * elapsed * billing)/60.0)
   */
  uint64_t total_cost;
} report_t;

#define REPORT_FORMAT          \
  "job_id: %d\n"               \
  "user_id: %d\n"              \
  "cluster: %s\n"              \
  "partition: %s\n"            \
  "state: %s\n"                \
  "allocated_ressources:\n"    \
  "  cpu: %ld\n"               \
  "  mem: %ld\n"               \
  "  gpu: %ld\n"               \
  "billable_ressources: %ld\n" \
  "time_start: %ld\n"          \
  "time_end: %ld\n"            \
  "job_duration: %ld\n"        \
  "cost_tier:\n"               \
  "  name: %s\n"               \
  "  factor: %lf\n"            \
  "total_cost: %ld\n"

/* File descriptor used for logging */
static char *log_directory = NULL;

/**
 * @brief Called when the plugin is loaded, before any other functions are
 * called. Put global initialization here.
 *
 * @return int SLURM_SUCCESS on success, or SLURM_ERROR on failure.
 */
extern int init(void) {
  slurm_info("%s: Initializing %s", plugin_type, plugin_name);
  return SLURM_SUCCESS;
}

/**
 * @brief Called when the plugin is removed. Clear any allocated storage here.
 *
 * @return int SLURM_SUCCESS on success, or SLURM_ERROR on failure.
 */
extern int fini(void) {
  slurm_info("%s: Finishing %s", plugin_type, plugin_name);
  xfree(log_directory);
  return SLURM_SUCCESS;
}

/**
 * @brief Specify the location to be used for job logging.
 *
 * @param location (input) specification of where logging should be done. The
 * interpretation of this string is at the discretion of the plugin
 * implementation.
 * @return int SLURM_SUCCESS if successful. On failure, the plugin should return
 * SLURM_ERROR and set the errno to an appropriate value to indicate the reason
 * for failure.
 */
extern int jobcomp_p_set_location(char *location) {
  slurm_info("%s: Set location %s", plugin_type, location);
  int rc = SLURM_SUCCESS;

  if (location == NULL) {
    return SLURM_ERROR;
  }
  xfree(log_directory);
  log_directory = xstrdup(location);

  return rc;
}

/**
 * @brief Note that a job is about to terminate or change size. The job's state
 * will include the JOB_RESIZING flag if and only if it is about to change size.
 * Otherwise the job is terminating. Note the existence of resize_time in the
 * job record if one wishes to record information about a job at each size (i.e.
 * a history of the job as its size changes through time).
 *
 * @param job_ptr (input) Pointer to job record as defined in
 * src/slurmctld/slurmctld.h
 * @return int SLURM_SUCCESS if successful. On failure, the plugin should return
 * SLURM_ERROR and set the errno to an appropriate value to indicate the reason
 * for failure.
 */
extern int jobcomp_p_log_record(job_record_t *job_ptr) {
  debug("%s: start %s %d", plugin_type, __func__, job_ptr->job_id);
  if (job_ptr == NULL) return error("%s: job_ptr is NULL", plugin_type);

  // Assert the job state
  if (!IS_JOB_COMPLETE(job_ptr) && !IS_JOB_TIMEOUT(job_ptr) &&
      !IS_JOB_FAILED(job_ptr) && !IS_JOB_COMPLETING(job_ptr)) {
    debug(
        "%s: job %d is not COMPLETED but was %s, "
        "ignoring...",
        plugin_type, job_ptr->job_id, job_state_string(job_ptr->job_state));
    return SLURM_SUCCESS;
  }

  // Assert the directory exists
  int rc = SLURM_SUCCESS;
  struct stat s;
  if (log_directory == NULL || stat(log_directory, &s) != 0 ||
      !S_ISDIR(s.st_mode)) {
    slurm_perror("failure: stat");
    return error("%s: JobCompLoc log directory %s is not accessible",
                 plugin_type, log_directory);
  }

  // Format the output file path
  char log_path[1024];
  snprintf(log_path, sizeof(log_path), "%s/%d.cost", log_directory,
           job_ptr->job_id);

  debug("%s: fetch report", plugin_type);

  // Parsing the job_ptr
  report_t report;
  report.job_id = job_ptr->job_id;
  debug("%s: report.job_id %d", plugin_type, report.job_id);
  if (job_ptr->assoc_ptr && job_ptr->assoc_ptr->cluster &&
      job_ptr->assoc_ptr->cluster[0])
    report.cluster = xstrdup(job_ptr->assoc_ptr->cluster);
  else
    report.cluster = NULL;
  debug("%s: report.cluster %s", plugin_type, report.cluster);
  if (job_ptr->part_ptr && job_ptr->part_ptr->name &&
      job_ptr->part_ptr->name[0])
    report.partition = xstrdup(job_ptr->part_ptr->name);
  else
    report.partition = NULL;
  debug("%s: report.partition %s", plugin_type, report.partition);
  report.job_state = job_ptr->job_state;
  debug("%s: report.job_state %s", plugin_type,
        job_state_string(job_ptr->job_state));
  report.user_id = job_ptr->user_id;
  debug("%s: report.user_id %d", plugin_type, report.user_id);
  report.start_time = job_ptr->start_time;
  debug("%s: report.start_time %ld", plugin_type, report.start_time);
  report.end_time = job_ptr->end_time;
  debug("%s: report.end_time %ld", plugin_type, report.end_time);
  report.elapsed = job_ptr->end_time - job_ptr->start_time;
  debug("%s: report.elapsed %ld", plugin_type, report.elapsed);
  if (job_ptr->qos_ptr) report.usage_factor = job_ptr->qos_ptr->usage_factor;
  debug("%s: report.usage_factor %lf", plugin_type, report.usage_factor);
  if (job_ptr->qos_ptr && job_ptr->qos_ptr->name && job_ptr->qos_ptr->name[0])
    report.qos_name = xstrdup(job_ptr->qos_ptr->name);
  else
    report.qos_name = NULL;
  debug("%s: report.qos_name %s", plugin_type, report.qos_name);
  if (job_ptr->tres_alloc_cnt) {
    report.billing = job_ptr->tres_alloc_cnt[TRES_ARRAY_BILLING];
    debug("%s: report.billing %ld", plugin_type, report.billing);
    report.mem = job_ptr->tres_alloc_cnt[TRES_ARRAY_MEM];
    debug("%s: report.mem %ld", plugin_type, report.mem);
    report.cpu = job_ptr->tres_alloc_cnt[TRES_ARRAY_CPU];
    debug("%s: report.cpu %ld", plugin_type, report.cpu);
  }
  report.total_cost = ((uint64_t)round(
      ((double)report.billing * (double)report.elapsed * report.usage_factor) /
      60.0l));
  debug("%s: report.total_cost %ld", plugin_type, report.total_cost);

  // Trying to find the gres gpu. Default to 0.
  // See: https://slurm.schedmd.com/gres_design.html
  debug("%s: gres_list_alloc", plugin_type);
  report.gpu = 0;
  if (job_ptr->gres_list_alloc && !list_is_empty(job_ptr->gres_list_alloc)) {
    gres_state_t *gres_state = NULL;
    ListIterator itr = list_iterator_create(job_ptr->gres_list_alloc);

    while ((gres_state = list_next(itr))) {
      debug("%s: found gres %s", plugin_type, gres_state->gres_name);
      if (xstrncmp(gres_state->gres_name, "gpu", 3) == 0) {
        gres_job_state_t *gres_job_state = gres_state->gres_data;
        report.gpu = gres_job_state->total_gres;
        debug("%s: report.gpu %ld", plugin_type, report.gpu);
        break;
      }
    }
    slurm_list_iterator_destroy(itr);
  }

  debug("%s: archiving %s", plugin_type, log_path);

  char buffer[1024];
  snprintf(buffer, sizeof(buffer), REPORT_FORMAT, report.job_id, report.user_id,
           report.cluster, report.partition,
           job_state_string(job_ptr->job_state), report.cpu, report.mem,
           report.gpu, report.billing, report.start_time, report.end_time,
           report.elapsed, report.qos_name, report.usage_factor,
           report.total_cost);

  debug("%s\n", buffer);

  FILE *output = fopen(log_path, "w");
  if (output != NULL) {
    fprintf(output, "%s\n", buffer);
    fclose(output);
  } else {
    rc = SLURM_ERROR;
  }

  xfree(report.cluster);
  xfree(report.qos_name);

  debug("%s: end %s %d", plugin_type, __func__, job_ptr->job_id);
  return rc;
}

/**
 * @brief Get completed job info from storage.
 *
 * @param job_cond (input) specification of filters to identify the jobs we
 * wish information about (start time, end time, cluster name, user id, etc).
 * acct_job_cond_t is defined in common/slurm_accounting_storage.h.
 * @return List A list of job records or NULL on error. Elements on the list
 * are of type jobcomp_job_rec_t, which is defined in common/slurm_jobcomp.h.
 * Any returned list must be destroyed to avoid memory leaks.
 */
extern List jobcomp_p_get_jobs(void *job_cond) {
  debug("%s: %s function is not implemented", plugin_type, __func__);
  return NULL;
}

/**
 * @brief Used to archive old data.
 *
 * @param arch_cond
 * @return int Error number for the last failure encountered by the job
 * completion plugin.
 */
extern int jobcomp_p_archive(void *arch_cond) {
  debug("%s: %s function is not implemented", plugin_type, __func__);
  return SLURM_SUCCESS;
}
