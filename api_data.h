#ifndef _API_DATA_H
#define _API_DATA_H

#define OBSERVER_QUEUE "rpm_worker_observer"
#define OBSERVER_CLASS "AbfWorker::RpmWorkerObserver"
#define API_JOBS_SHIFT "/jobs/shift"
#define API_JOBS_FEEDBACK "/jobs/feedback"
#define API_JOBS_STATUS "/jobs/status"
#define API_JOBS_LOGS "/jobs/logs"
#define API_JOBS_STATISTICS "/jobs/statistics"
#define API_LOGS_PAYLOAD "{\"name\":\"%s\",\"logs\":\"%s\"}"
#define API_STATUS_QUERYSTRING "?key=abfworker::rpm-worker-%s::live-inspector"
#define API_STATISTICS_PAYLOAD "{\"uid\":\"%s\",\"worker_count\":1,\"busy_workers\":%d,\"host\":\"%s\",\"query_string\":\"%s\"}"

#endif
