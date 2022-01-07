#if !defined(API_H)
#define API_H

#include "jobcomp_report.h"

/**
 * @brief POST the report in the url.
 *
 * @param report Report structure to be published.
 * @param url URL
 * @return int Error code.
 */
int publish(report_t* report, char* url);

#endif  // API_H
