
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* dynamic_string.h */

#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <stdbool.h>

/*
 * 文字列を表す構造体
 * 文字列の追加時に, バッファのサイズが不足する場合は自動的に拡張される
 */
struct dynamic_string {
    char*   buffer;          /* 文字列を格納するバッファ */
    size_t  length;          /* ヌル文字を除いた文字列の長さ */
    size_t  capacity_buffer; /* 文字列を格納するバッファのサイズ */
};

/*
 * 文字列を初期化
 */
bool initialize_dynamic_string(struct dynamic_string* dyn_str);

/*
 * 文字列を破棄
 */
void free_dynamic_string(struct dynamic_string* dyn_str);

/*
 * 文字列の所有権を移動(ムーブ)
 */
char* move_dynamic_string(struct dynamic_string* dyn_str);

/*
 * 文字列を末尾に追加
 */
bool dynamic_string_append(
    struct dynamic_string* dyn_str, const char* str);

/*
 * 部分文字列を末尾に追加
 */
bool dynamic_string_append_substring(
    struct dynamic_string* dyn_str, const char* str,
    size_t index, size_t len);

/*
 * 単一の文字を末尾に追加
 */
bool dynamic_string_append_char(struct dynamic_string* dyn_str, char c);

#endif /* DYNAMIC_STRING_H */

