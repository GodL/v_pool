//
//  v_pool.h
//  v_pool
//
//  Created by lihao10 on 2020/4/17.
//  Copyright © 2020 GodL. All rights reserved.
//

#ifndef v_pool_h
#define v_pool_h

#include <stdio.h>

typedef const void *(*v_pool_create_value_callback)(const char *);
typedef void (*v_pool_destory_value_callback)(const void *);

typedef struct v_pool_callback {
    v_pool_create_value_callback create_callback;
    v_pool_destory_value_callback destory_callback;
} v_pool_callback;

typedef struct _v_pool * v_pool;

v_pool __attribute__((overloadable)) pool_create(v_pool_callback *callback,const char *path);

v_pool __attribute__((overloadable)) pool_create(v_pool_callback *callback,const char *path,unsigned int limit);

const void *pool_pop(v_pool pool);

void pool_return(v_pool pool,const void *value);

void pool_destory(v_pool pool);

#endif /* v_pool_h */
