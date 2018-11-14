
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* input.c */

#include <stdio.h>
#include <stdlib.h>

#include "input.h"

/*
 * ユーザの入力文字列を取得
 */
char* get_line()
{
    char* input = NULL;
    size_t length = 0;
    ssize_t cc = 0;

    if ((cc = getline(&input, &length, stdin)) == -1) {
        free(input);
        return NULL;
    }

    return input;
}

