#include <pthread.h>
#include <openssl/err.h>
#include "log_levels.h"

extern void log_printf(unsigned int level, const char *message, ...);
extern void* xmalloc(size_t);

static pthread_mutex_t *mutex_buf = NULL;

static void locking_function(int mode, int n, const char *file, int line) {
  if (mode & CRYPTO_LOCK) {
    log_printf(LOG_DEBUG, "%s:%d Locking %d mutex\n", file, line, n + 1);
    pthread_mutex_lock(&mutex_buf[n]);
  } else if (mode & CRYPTO_UNLOCK) {
    log_printf(LOG_DEBUG, "%s:%d Unlocking %d mutex\n", file, line, n + 1);
    pthread_mutex_unlock(&mutex_buf[n]);
  }
}

int thread_setup() {
  mutex_buf = xmalloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));

  if (!mutex_buf) {
    return 0;
  }

  for(int i = 0; i < CRYPTO_num_locks(); i++) {
    pthread_mutex_init(&mutex_buf[i], NULL);
  }

  (void)(locking_function);
  CRYPTO_set_locking_callback(locking_function);
  return 1;
}