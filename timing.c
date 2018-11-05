#include "timing.h"

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

enum {
  k_timing_num_timers = 8,
};

struct timing_struct {
  size_t max_timer;
  void (*p_callbacks[k_timing_num_timers])(void*);
  void* p_objects[k_timing_num_timers];
  int64_t timings[k_timing_num_timers];
  uint8_t running[k_timing_num_timers];
  void* p_legacy_callback;
  void* p_legacy_object;
};

struct timing_struct*
timing_create() {
  struct timing_struct* p_timing = malloc(sizeof(struct timing_struct));
  if (p_timing == NULL) {
    errx(1, "couldn't allocate timing_struct");
  }

  (void) memset(p_timing, '\0', sizeof(struct timing_struct));

  p_timing->max_timer = 0;

  return p_timing;
}

void
timing_destroy(struct timing_struct* p_timing) {
  free(p_timing);
}

static int64_t
timing_recalculate(struct timing_struct* p_timing) {
  size_t i;
  /* Default is never. */
  int64_t next_event = INT64_MAX;
  size_t max_timer = p_timing->max_timer;
  for (i = 0; i < max_timer; ++i) {
    if (!p_timing->running[i]) {
      continue;
    }
    if (p_timing->timings[i] < next_event) {
      next_event = p_timing->timings[i];
    }
  }

  return next_event;
}

size_t
timing_register_timer(struct timing_struct* p_timing,
                      void* p_callback,
                      void* p_object) {
  size_t i;
  for (i = 0; i < k_timing_num_timers; ++i) {
    if (p_timing->p_callbacks[i] == NULL) {
      break;
    }
  }
  assert(i < k_timing_num_timers);

  p_timing->max_timer = (i + 1);

  p_timing->p_callbacks[i] = p_callback;
  p_timing->p_objects[i] = p_object;
  p_timing->timings[i] = 0;
  p_timing->running[i] = 0;

  return i;
}

void
timing_start_timer(struct timing_struct* p_timing, size_t id, int64_t time) {
  assert(id < k_timing_num_timers);
  assert(id < p_timing->max_timer);
  assert(p_timing->p_callbacks[id] != NULL);

  p_timing->timings[id] = time;
  p_timing->running[id] = 1;

  timing_recalculate(p_timing);
}

void
timing_stop_timer(struct timing_struct* p_timing, size_t id) {
  assert(id < k_timing_num_timers);
  assert(id < p_timing->max_timer);
  assert(p_timing->p_callbacks[id] != NULL);

  p_timing->running[id] = 0;

  timing_recalculate(p_timing);
}

int64_t
timing_increase_timer(struct timing_struct* p_timing, size_t id, int64_t time) {
  assert(id < k_timing_num_timers);
  assert(id < p_timing->max_timer);
  assert(p_timing->p_callbacks[id] != NULL);

  p_timing->timings[id] += time;

  timing_recalculate(p_timing);

  return p_timing->timings[id];
}

int64_t
timing_advance(struct timing_struct* p_timing, int64_t time) {
  size_t i;
  int64_t next_event;

  size_t max_timer = p_timing->max_timer;

  for (i = 0; i < max_timer; ++i) {
    int64_t value;
    uint8_t running = p_timing->running[i];
    void (*p_callback)(void*) = p_timing->p_callbacks[i];

    if (!running) {
      continue;
    }
    value = p_timing->timings[i];
    value -= time;
    p_timing->timings[i] = value;
    if (value <= 0) {
      p_callback(p_timing->p_objects[i]);
    }
  }

  next_event = timing_recalculate(p_timing);

  return next_event;
}

void
timing_set_sync_tick_callback(struct timing_struct* p_timing,
                              void* p_callback,
                              void* p_object) {
  p_timing->p_legacy_callback = p_callback;
  p_timing->p_legacy_object = p_object;
}

void
timing_do_sync_tick_callback(struct timing_struct* p_timing) {
  void (*p_callback)(void*) = p_timing->p_legacy_callback;
  p_callback(p_timing->p_legacy_object);
}
