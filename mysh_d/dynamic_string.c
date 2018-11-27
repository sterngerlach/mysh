
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* dynamic_string.c */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dynamic_string.h"
#include "util.h"

#define INITIAL_STRING_CAPACITY     8
#define INCREMENT_STRING_CAPACITY   8

/*
 * dynamic_string構造体の操作関数
 */

/*
 * 文字列を初期化
 */
bool initialize_dynamic_string(struct dynamic_string* dyn_str)
{
    assert(dyn_str != NULL);

    dyn_str->buffer = NULL;
    dyn_str->length = 0;
    dyn_str->capacity_buffer = 0;

    return true;
}

/*
 * 文字列を破棄
 */
void free_dynamic_string(struct dynamic_string* dyn_str)
{
    assert(dyn_str != NULL);

    if (dyn_str->buffer != NULL) {
        free(dyn_str->buffer);
        dyn_str->buffer = NULL;
    }

    dyn_str->length = 0;
    dyn_str->capacity_buffer = 0;
}

/*
 * 文字列の所有権を移動(ムーブ)
 */
char* move_dynamic_string(struct dynamic_string* dyn_str)
{
    char* str;

    assert(dyn_str != NULL);

    str = dyn_str->buffer;
    
    dyn_str->buffer = NULL;
    dyn_str->length = 0;
    dyn_str->capacity_buffer = 0;

    return str;
}

/*
 * 文字列を末尾に追加
 */
bool dynamic_string_append(
    struct dynamic_string* dyn_str, const char* str)
{
    return dynamic_string_append_substring(dyn_str, str, 0, strlen(str));
}

/*
 * 部分文字列を末尾に追加
 */
bool dynamic_string_append_substring(
    struct dynamic_string* dyn_str, const char* str,
    size_t index, size_t len)
{
    size_t new_capacity_buffer;
    char* new_buffer;

    assert(dyn_str != NULL);
    assert(str != NULL);
    assert(index <= strlen(str));
    assert(index + len <= strlen(str));
    
    /* 追加する文字列が空である場合は何もしない */
    if (*str == '\0')
        return true;

    /* バッファが空である場合 */
    if (dyn_str->buffer == NULL) {
        new_capacity_buffer = len + 1;
        new_buffer = (char*)calloc(new_capacity_buffer, sizeof(char));

        if (new_buffer == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        dyn_str->capacity_buffer = new_capacity_buffer;
        dyn_str->buffer = new_buffer;
    }
    
    /* バッファが溢れそうな場合は適宜サイズを拡張 */
    if (dyn_str->length + len + 1 >= dyn_str->capacity_buffer) {
        new_capacity_buffer = dyn_str->length + len + 1;
        new_buffer = (char*)realloc(dyn_str->buffer, sizeof(char) * new_capacity_buffer);

        if (new_buffer == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        dyn_str->capacity_buffer = new_capacity_buffer;
        dyn_str->buffer = new_buffer;
    }
    
    /* 文字列をバッファに追加 */
    dyn_str->length += len;
    strncat(dyn_str->buffer, str + index, len);
    dyn_str->buffer[dyn_str->length] = '\0';

    return true;
}

/*
 * 単一の文字を末尾に追加
 */
bool dynamic_string_append_char(struct dynamic_string* dyn_str, char c)
{
    size_t new_capacity_buffer;
    char* new_buffer;

    assert(dyn_str != NULL);
    
    /* バッファが空である場合 */
    if (dyn_str->buffer == NULL) {
        new_capacity_buffer = INITIAL_STRING_CAPACITY;
        new_buffer = (char*)calloc(new_capacity_buffer, sizeof(char));

        if (new_buffer == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        dyn_str->capacity_buffer = new_capacity_buffer;
        dyn_str->buffer = new_buffer;
    }

    /* バッファが溢れそうな場合は適宜サイズを拡張 */
    if (dyn_str->length + 2 >= dyn_str->capacity_buffer) {
        new_capacity_buffer = dyn_str->capacity_buffer + INCREMENT_STRING_CAPACITY;
        new_buffer = (char*)realloc(dyn_str->buffer, sizeof(char) * new_capacity_buffer);

        if (new_buffer == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        dyn_str->capacity_buffer = new_capacity_buffer;
        dyn_str->buffer = new_buffer;
    }

    /* 文字をバッファに追加 */
    dyn_str->buffer[dyn_str->length++] = c;
    dyn_str->buffer[dyn_str->length] = '\0';

    return true;
}

