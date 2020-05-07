//
//  v_pool.c
//  v_pool
//
//  Created by lihao10 on 2020/4/17.
//  Copyright © 2020 GodL. All rights reserved.
//

#include "v_pool.h"
#include "atomque.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <libkern/OSAtomic.h>

struct _v_pool {
    volatile unsigned int length;
    unsigned int limit;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    const char *path;
    v_pool_callback callback;
    atomque_ref pool;
    atomque_ref wait_queue;
};

v_pool __attribute__((overloadable)) pool_create(v_pool_callback *callback,const char *path) {
    return pool_create(callback, path, 6);
}

v_pool __attribute__((overloadable)) pool_create(v_pool_callback *callback,const char *path,unsigned int limit) {
    if (callback == NULL || path == NULL || limit == 0) return NULL;
    v_pool pool = malloc(sizeof(struct _v_pool));
    if (pool == NULL) return NULL;
    pool->length = 0;
    pool->limit = limit;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pool->path = strdup(path);
    pool->callback = *callback;
    atomque_callback atomque_callback = {
        NULL,
        NULL,
        callback->destory_callback
    };
    pool->pool = atomque_create(&atomque_callback);
    if (pool->pool == NULL) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->cond);
        free((void *)pool->path);
        free(pool);
        pool = NULL;
    }
    pool->wait_queue = atomque_create(&atomque_callback);
    if (pool->wait_queue == NULL) {
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->cond);
        free((void *)pool->path);
        atomque_destory(pool->pool);
        free(pool);
        pool = NULL;
    }
    return pool;
}

const void *pool_pop(v_pool pool) {
    if (pool == NULL) return NULL;
    const void *value = atomque_dequeue(pool->pool);
    if (value == NULL) {
        if (pool->length >= pool->limit) {
            atomque_enqueue(pool->wait_queue, pthread_self());
            pthread_mutex_lock(&pool->lock);
            while ((value = atomque_dequeue(pool->pool)) == NULL) {
                pthread_cond_wait(&pool->cond, &pool->lock);
            }
            pthread_mutex_unlock(&pool->lock);
        }else {
            unsigned int length = __sync_fetch_and_add_4(&pool->length, 1);
            if (length > pool->limit) {
                atomque_enqueue(pool->wait_queue, pthread_self());
                pool->length = pool->limit;
                pthread_mutex_lock(&pool->lock);
                while ((value = atomque_dequeue(pool->pool)) == NULL) {
                    pthread_cond_wait(&pool->cond, &pool->lock);
                }
                pthread_mutex_unlock(&pool->lock);
            }else {
                value = pool->callback.create_callback(pool->path);
            }
        }
    }
    return value;
}

void pool_return(v_pool pool,const void *value) {
    if (pool == NULL || value == NULL) return;
    atomque_enqueue(pool->pool, value);
    pthread_t pthread = (pthread_t)atomque_dequeue(pool->wait_queue);
    if (pthread) {
        pthread_mutex_lock(&pool->lock);
        pthread_cond_signal_thread_np(&pool->cond, pthread);
        pthread_mutex_unlock(&pool->lock);
    }
}

void pool_destory(v_pool pool) {
    if (pool == NULL) return;
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    atomque_destory(pool->wait_queue);
    const void *value = NULL;
    while ((value = atomque_dequeue(pool->pool))) {
        pool->callback.destory_callback(value);
    }
    atomque_destory(pool->pool);
    free((void *)pool->path);
    free(pool);
    pool = NULL;
}
