
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* util.c */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/*
 * 空白文字かどうかを判定
 * シェルにおける空白文字である場合は0を, そうでない場合は0を返す
 */
int is_blank_char(char c)
{
    return (strchr(" \t", c) != NULL);
}

/*
 * メタ文字かどうかを判定
 * シェルにおけるメタ文字である場合は1を, そうでない場合は0を返す
 */
int is_meta_char(char c)
{
    return (strchr(" \t\n|&;()<>", c) != NULL);
}

/*
 * 文字列の末尾の改行文字を除去
 */
void chomp(char* str)
{
    char* p;

    if ((p = strrchr(str, '\n')) != NULL && *(p + 1) == '\0')
        *p = '\0';
    if ((p = strrchr(str, '\r')) != NULL && *(p + 1) == '\0')
        *p = '\0';
}

/*
 * 文字列(10進数の整数表現)を整数に変換
 */
bool strict_strtol(const char* nptr, long* valptr)
{
    long val;
    char* endptr;

    /* strtol()関数のエラーを判定するためにerrnoを0に設定
     * この方法によるエラーの判定は, strtol()関数が成功時にはerrnoの値を
     * 変更しないということが, Unixの仕様によって定められているため使用できる */
    errno = 0;

    val = strtol(nptr, &endptr, 10);

    /* オーバーフロー, アンダーフロー, 変更が行われなかった場合はエラー */
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
        (errno != 0 && val == 0)) {
        print_error(__func__, "strtol() failed: %s\n", strerror(errno));
        return false;
    }

    /* 数字が1つも含まれていない場合はエラー */
    if (nptr == endptr) {
        print_error(__func__, "no digits were found\n");
        return false;
    }

    /* 不正な文字が含まれていた場合はエラー */
    if (endptr[0] != '\0') {
        print_error(__func__, "invalid character: %c\n", endptr[0]);
        return false;
    }

    *valptr = val;
    return true;
}

/*
 * メッセージを標準エラー出力に表示
 */
void print_message(const char* function, const char* format, ...)
{
    va_list vp;

    fprintf(stderr,
            ANSI_ESCAPE_COLOR_BLUE "%s(): " ANSI_ESCAPE_COLOR_RESET,
            function);

    va_start(vp, format);
    vfprintf(stderr, format, vp);
    va_end(vp);
}

/*
 * エラーメッセージを標準エラー出力に表示
 */
void print_error(const char* function, const char* format, ...)
{
    va_list vp;

    fprintf(stderr,
            ANSI_ESCAPE_COLOR_RED "%s() failed: " ANSI_ESCAPE_COLOR_RESET,
            function);

    va_start(vp, format);
    vfprintf(stderr, format, vp);
    va_end(vp);
}

