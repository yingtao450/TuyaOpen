/**
 * @file websocket_utils.h
 * @brief Implements utility functions for WebSocket string operations
 *
 * This source file provides the implementation of utility functions for
 * string manipulation within the WebSocket framework. It includes functions
 * for string duplication, safe memory allocation, and cleanup operations.
 * The implementation supports safe string handling with proper memory
 * management, null checks, and buffer overflow prevention. This file is
 * essential for developers working on IoT applications that require reliable
 * string manipulation with proper memory management in WebSocket
 * communications.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __WEBSOCKET_UTILS_H__
#define __WEBSOCKET_UTILS_H__

#include "tal_api.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define ENABLE_WEBSOCKET_CLIENT_DEBUG

#ifdef ENABLE_WEBSOCKET_CLIENT_DEBUG
#define WS_DEBUG(...) PR_DEBUG(__VA_ARGS__)
#else
#define WS_DEBUG(...) PR_TRACE(__VA_ARGS__)
#endif

#ifndef WS_SAFE_GET_STR
#define WS_SAFE_GET_STR(x) ((x!=NULL) ? (x) : "")
#endif

#ifndef WS_ASSERT
#define WS_ASSERT(EXPR)                                                         \
    if (!(EXPR)) {                                                              \
        PR_ERR("WS_ASSERT(%s) has assert failed at %s:%d.",                     \
                #EXPR, __FUNCTION__, __LINE__);                                 \
    }
#endif /** !WS_ASSERT */

#ifndef WS_CALL_ERR_RET
#define WS_CALL_ERR_RET(func)\
do{\
    rt = (func);\
    if (OPRT_OK != (rt)){\
       PR_ERR("call %s return %d", #func, rt);\
       return (rt);\
    }\
}while(0)
#endif

#ifndef WS_CALL_ERR_RET_VAL
#define WS_CALL_ERR_RET_VAL(func, y)\
do{\
    rt = (func);\
    if (OPRT_OK != (rt)){\
       PR_ERR("call %s return %d", #func, rt);\
       return (y);\
    }\
}while(0)
#endif


#ifndef WS_CALL_ERR_GOTO
#define WS_CALL_ERR_GOTO(func)\
do{\
    rt = (func);\
    if (OPRT_OK != (rt)){\
        PR_ERR("call %s return %d", #func, rt);\
        goto err_exit;\
    }\
}while(0)
#endif

#ifndef WS_CHECK_NULL_RET
#define WS_CHECK_NULL_RET(x)\
do{\
    if (NULL == (x)){\
        PR_ERR("%s is null!", #x);\
        return OPRT_INVALID_PARM;\
    }\
}while(0)
#endif

#ifndef WS_CHECK_NULL_UNRET
#define WS_CHECK_NULL_UNRET(x)\
do{\
    if (NULL == (x)){\
        PR_ERR("%s is null!", #x);\
        return ;\
    }\
}while(0)
#endif

#ifndef WS_CHECK_NULL_RET_VAL
#define WS_CHECK_NULL_RET_VAL(x, y)\
do{\
    if (NULL == (x)){\
        PR_ERR("%s is null!", #x);\
        return (y);\
    }\
}while(0)
#endif

#ifndef WS_CHECK_BOOL_RET_VAL
#define WS_CHECK_BOOL_RET_VAL(x, y)\
do{\
    if (TRUE == (x)){\
        PR_ERR("%s is true.", #x);\
        return (y);\
    }\
}while(0)
#endif

/**
 * Malloc memory, goto label(err_exit) when error occurred.
 */
#ifndef WS_MALLOC_ERR_GOTO
#define WS_MALLOC_ERR_GOTO(_ptr, _size)\
    do{\
        if (NULL == ((_ptr) = Malloc((_size)))){\
            PR_ERR("Malloc err.");\
            rt = OPRT_MALLOC_FAILED;\
            goto err_exit;\
        }\
    }while(0)
#endif

#ifndef WS_MALLOC_ZERO_ERR_GOTO
#define WS_MALLOC_ZERO_ERR_GOTO(_ptr, _size)\
    do{\
        if (NULL == ((_ptr) = Malloc((_size)))){\
            PR_ERR("Malloc err.");\
            rt = OPRT_MALLOC_FAILED;\
            goto err_exit;\
        }\
        memset((_ptr), 0, (_size));\
    }while(0)
#endif


/**
 * Malloc memory, return when error occured.
 */
#ifndef WS_MALLOC_ERR_RET
#define WS_MALLOC_ERR_RET(_ptr, _size)\
    do{\
        if (NULL == ((_ptr) = Malloc((_size)))){\
            PR_ERR("Malloc err.");\
            return OPRT_MALLOC_FAILED;\
        }\
    }while(0)
#endif

#ifndef WS_MALLOC_ERR_RET_VAL
#define WS_MALLOC_ERR_RET_VAL(_ptr, _size, y)\
    do{\
        if (NULL == ((_ptr) = Malloc((_size)))){\
            PR_ERR("Malloc err.");\
            return (y);\
        }\
    }while(0)
#endif

#ifndef WS_MALLOC_ZERO_ERR_RET
#define WS_MALLOC_ZERO_ERR_RET(_ptr, _size)\
    do{\
        if (NULL == ((_ptr) = Malloc((_size)))){\
            PR_ERR("Malloc err.");\
            return OPRT_MALLOC_FAILED;\
        }\
        memset((_ptr), 0, (_size));\
    }while(0)
#endif

/**
 * Free memory which have malloced
 */
#ifndef WS_SAFE_FREE
#define WS_SAFE_FREE(_ptr)\
    do{\
        if(NULL != (_ptr)){\
            Free((_ptr));\
            (_ptr) = NULL;\
        } else {\
            PR_NOTICE("%s is null", #_ptr);\
        }\
    } while(0)
#endif

#define WS_STOR_BE16(a, p) \
   ((uint8_t *)(p))[0] = ((uint16_t)(a) >> 8) & 0xFFU, \
   ((uint8_t *)(p))[1] = ((uint16_t)(a) >> 0) & 0xFFU

#define WS_STOR_BE64(a, p) \
    ((uint8_t *)(p))[0] = ((uint64_t)(a) >> 56) & 0xFFU, \
    ((uint8_t *)(p))[1] = ((uint64_t)(a) >> 48) & 0xFFU, \
    ((uint8_t *)(p))[2] = ((uint64_t)(a) >> 40) & 0xFFU, \
    ((uint8_t *)(p))[3] = ((uint64_t)(a) >> 32) & 0xFFU, \
    ((uint8_t *)(p))[4] = ((uint64_t)(a) >> 24) & 0xFFU, \
    ((uint8_t *)(p))[5] = ((uint64_t)(a) >> 16) & 0xFFU, \
    ((uint8_t *)(p))[6] = ((uint64_t)(a) >> 8) & 0xFFU, \
    ((uint8_t *)(p))[7] = ((uint64_t)(a) >> 0) & 0xFFU

#define WS_LOAD_BE16(p) ( \
    ((uint16_t)(((uint8_t *)(p))[0]) << 8) | \
    ((uint16_t)(((uint8_t *)(p))[1]) << 0))

#define WS_LOAD_BE64(p) ( \
    ((uint64_t)(((uint8_t *)(p))[0]) << 56) | \
    ((uint64_t)(((uint8_t *)(p))[1]) << 48) | \
    ((uint64_t)(((uint8_t *)(p))[2]) << 40) | \
    ((uint64_t)(((uint8_t *)(p))[3]) << 32) | \
    ((uint64_t)(((uint8_t *)(p))[4]) << 24) | \
    ((uint64_t)(((uint8_t *)(p))[5]) << 16) | \
    ((uint64_t)(((uint8_t *)(p))[6]) << 8) | \
    ((uint64_t)(((uint8_t *)(p))[7]) << 0))

    /**
 * @brief Create a duplicate copy of a string
 * 
 * This function creates a new string by duplicating the content of the source string.
 * It allocates new memory for the duplicate string and copies the content.
 * The caller is responsible for freeing the returned string using websocket_string_delete.
 * 
 * @param[in] src Source string to be duplicated (can be NULL)
 * 
 * @return char* 
 * @retval NULL If src is NULL, empty, or memory allocation fails
 * @retval pointer Pointer to the newly allocated duplicate string
 * 
 * @note The returned string must be freed using websocket_string_delete
 */
char *websocket_string_dupcpy(char *src);

/**
 * @brief Safely delete a dynamically allocated string
 * 
 * This function safely frees the memory of a dynamically allocated string
 * and sets the pointer to NULL to prevent dangling pointer issues.
 * 
 * @param[in,out] src Pointer to the string pointer to be freed
 *                    Will be set to NULL after freeing
 * 
 * @note This function is safe to call with NULL or already freed pointers
 */
void websocket_string_delete(char **str);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /** !__WEBSOCKET_UTILITIES_H__ */
