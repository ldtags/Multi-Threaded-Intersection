#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define FACTOR 1000

/*
 * Lanes I, II, III, and IV.
 * Resource 1, 2, 3, and 4.
 *
 *      |   |   |
 *      |       |
 *      | I |   |
 * -----+       +-----
 *        1   4  IV
 * - - -         - - -
 *    II  2   3
 * -----+       +-----
 *      |   |III|
 *      |       |
 *      |   |   |
 *
 * E.g., Lane III requires resource 3 and 4 to safely proceed through the
 * intersection.
 */
uint8_t intersection[4];
pthread_mutex_t mutex[4];
pthread_mutex_t fin;
pthread_cond_t cond[4];

bool *done;

void* lane_fn(void *arg)
{
        int id, req1, req2;

        id   = ((int *) arg)[0];
        req1 = ((int *) arg)[0];
        req2 = ((int *) arg)[1];

        do {
                uint32_t p;

                /* Sleep random interval. */
                getentropy(&p, sizeof(p));
                usleep(p % FACTOR);

                printf("Car %d entering intersection (%d)\n", id, req1);

                /* Lowest Priority First Lock Ordering */
                if(req1 < req2) {
                        pthread_mutex_lock(&mutex[req1]);
                        pthread_mutex_lock(&mutex[req2]);
                } else {
                        pthread_mutex_lock(&mutex[req2]);
                        pthread_mutex_lock(&mutex[req1]);
                }

                printf("Car %d in intersection (%d and %d)\n", id, req1, req2);

                /* Collision detection; non-zero implies already claimed. */
                if (intersection[req1] != 0) {
                        printf("%d crashed into %d!\n", id, intersection[req1]);
                }

                if (intersection[req2] != 0) {
                        printf("%d crashed into %d!\n", id, intersection[req2]);
                }

                /* Actually enter intersection. */
                intersection[req1] = id;
                intersection[req2] = id;

                /* Spend a bit of time in intersection. */
                usleep(FACTOR);

                printf("Car %d out of intersection (%d and %d)\n", id, req1, req2);

                /* Leave intersection. */
                intersection[req1] = 0;
                intersection[req2] = 0;

                /* ensures unlock is consistent with lock ordering */
                if(req1 < req2) {
                        pthread_mutex_unlock(&mutex[req2]);
                        pthread_mutex_unlock(&mutex[req1]);
                } else {
                        pthread_mutex_unlock(&mutex[req1]);
                        pthread_mutex_unlock(&mutex[req2]);
                }

        } while (pthread_mutex_trylock(&fin)); // if the lock can be acquired, three seconds have passed
        
        pthread_mutex_unlock(&fin); // immediately unlock the lock for other threads to acquire
        return NULL;
}

int main(void)
{
        bool local = false;
        done = &local;

        /* Each lane gets a thread, which simulates cars passing through. */
        pthread_t lane[4];

        /* The requirements for each lane to pass through intersection. */
        int req[4][2] = {
                { 0, 1 },
                { 1, 2 },
                { 2, 3 },
                { 3, 0 }
        };

        /* Initializing conditions and mutexes */
        pthread_mutex_init(&fin, NULL);
        for (int i = 0; i < 4; i++) {
                pthread_mutex_init(&mutex[i], NULL);
        }

        /* Start simulators (threads). */
        pthread_mutex_lock(&fin);
        for (int i = 0; i < 4; i++) {
                pthread_create(&lane[i], NULL, lane_fn, &req[i]);
        }

        /* Terminate simulation after three seconds. */
        sleep(3);
        /* unlocks fin mutex that tells lane_fn three seconds has passed */
        pthread_mutex_unlock(&fin);

        for (int i = 0; i < 4; i++) {
                pthread_join(lane[i], NULL);
        }

        /* Destroy mutexes */
        for (int i = 0; i < 4; i++) {
                pthread_mutex_destroy(&mutex[i]);
        }
        pthread_mutex_destroy(&fin);

        return 0;
}