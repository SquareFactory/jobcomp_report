#include "api.h"

#include <curl/curl.h>

#include "cJSON.h"
#include "src/common/gres.h"
#include "src/common/log.h"

size_t write_data(void* buffer, size_t size, size_t nmemb, void* userp);

size_t write_data(__attribute__((unused)) void* buffer, size_t size,
                  size_t nmemb, __attribute__((unused)) void* userp) {
  return size * nmemb;
}

/**
 * @brief Add the fields from the report. output must be initialized.
 *
 * @param report The report object.
 * @param output A JSON object.
 * @return int Error code.
 */
int build_json_object(const report_t* report, cJSON* output);

int publish(report_t* report, char* url) {
  int rc = 0;
  CURLcode res;
  cJSON* body = NULL;
  char* body_str = NULL;
  CURL* curl = curl_easy_init();
  if (!curl) {
    error("curl_easy_init failed");
    goto cleanup;
  }
  body = cJSON_CreateObject();
  if (!body) {
    error("cJSON_CreateObject: failed");
    rc = 1;
    goto cleanup;
  }

  struct curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charset: utf-8");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

  if (build_json_object(report, body) != 0) {
    rc = error("build_json_object: failed");
    goto cleanup;
  }
  body_str = cJSON_PrintUnformatted(body);
  if (!body_str) {
    rc = error("cJSON_PrintUnformatted: failed");
    goto cleanup;
  }
  debug("%s\n", body_str);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    rc = error(
        "curl_easy_perform() failed: %s\n"
        "url was %s\n"
        "body was %s\n",
        curl_easy_strerror(res), url, body_str);
  }

cleanup:
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  cJSON_Delete(body);
  free(body_str);
  return rc;
}

int build_json_object(const report_t* report, cJSON* output) {
  cJSON* allocated_ressources = NULL;
  cJSON* cost_tier = NULL;
  if (!cJSON_AddNumberToObject(output, "job_id", report->job_id)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "user_id", report->user_id)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddStringToObject(output, "account", report->account)) {
    return error("cJSON_AddStringToObject: failed");
  }
  if (!cJSON_AddStringToObject(output, "cluster", report->cluster)) {
    return error("cJSON_AddStringToObject: failed");
  }
  if (!cJSON_AddStringToObject(output, "partition", report->partition)) {
    return error("cJSON_AddStringToObject: failed");
  }
  if (!cJSON_AddStringToObject(output, "state",
                               job_state_string(report->job_state))) {
    return error("cJSON_AddStringToObject: failed");
  }

  if (!cJSON_AddItemToObject(output, "allocated_ressources",
                             allocated_ressources = cJSON_CreateObject())) {
    return error("cJSON_AddItemToObject: failed");
  }
  if (!cJSON_AddNumberToObject(allocated_ressources, "cpu", report->cpu)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(allocated_ressources, "mem", report->mem)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(allocated_ressources, "gpu", report->gpu)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "allocated_billing_factor",
                               report->billing)) {
    return error("cJSON_AddNumberToObject: failed");
  }

  if (!cJSON_AddNumberToObject(output, "start_time", report->start_time)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "end_time", report->end_time)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "billed_time", report->elapsed)) {
    return error("cJSON_AddNumberToObject: failed");
  }

  if (!cJSON_AddItemToObject(output, "cost_tier",
                             cost_tier = cJSON_CreateObject())) {
    return error("cJSON_AddItemToObject: failed");
  }
  if (!cJSON_AddStringToObject(cost_tier, "name", report->qos_name)) {
    return error("cJSON_AddStringToObject: failed");
  }
  if (!cJSON_AddNumberToObject(cost_tier, "factor", report->usage_factor)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "total_cost", report->total_cost)) {
    return error("cJSON_AddNumberToObject: failed");
  }
  if (!cJSON_AddNumberToObject(output, "priority", report->priority)) {
    return error("cJSON_AddNumberToObject: failed");
  }

  return 0;
}
