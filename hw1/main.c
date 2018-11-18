#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const size_t kStrideSize = 1024 * 1024 * 32;
static const size_t kMaxAffinity = 32;
static const size_t kIterationsPerAffinity = 1024 * 1024;


unsigned long long rdtscl(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int main() {
    for (unsigned affinity = 4; affinity <= kMaxAffinity; affinity++) {
        size_t buf_size = affinity * kStrideSize;
        size_t *memory_buffer = calloc(buf_size, sizeof(size_t));
        assert(memory_buffer);

        // Initialize the buffer
        for (size_t i = 0; i < buf_size; ++i) {
            size_t target_idx = i + kStrideSize;
            if (target_idx >= buf_size) {
                memory_buffer[i] = (target_idx + 1) % buf_size;
            } else {
                memory_buffer[i] = target_idx;
            }
        }

        unsigned long long delta = 0;
        unsigned long long max = 0;
        unsigned long long min = 10000000;

        size_t i = 0;
        for (size_t iteration = 0; iteration < kIterationsPerAffinity; ++iteration) {
            unsigned  long long start = rdtscl();
            size_t new_idx = memory_buffer[i];
            unsigned long long diff = rdtscl() - start;
            assert(new_idx - i >= kStrideSize);
            i = new_idx;
            delta += diff;
            if (diff > max) {
                max = diff;
            }
            if (diff < min) {
                min = diff;
            }
        }

        printf("Affinity: %u avg: %f max: %llu min: %llu\n", affinity, ((float) delta) / kIterationsPerAffinity, max, min);

        free(memory_buffer);
    }
}
