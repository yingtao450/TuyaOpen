/**
 * @file websocket_utils.c
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

#include "websocket_utils.h"

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
char *websocket_string_dupcpy(char *src)
{
    size_t len = 0;
    char *dst = NULL;
    if (src != NULL && ((len = strlen(src)) > 0)) {
        WS_MALLOC_ERR_RET_VAL(dst, len + 1, NULL);
        strncpy(dst, src, len);
        dst[len] = '\0';
    }
    return dst;
}

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
void websocket_string_delete(char **src)
{
    WS_SAFE_FREE(*src);
}
