
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* main.c */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

int main(int argc, char** argv)
{
    char* input;
    struct token_stream tok_stream;
    struct command cmd;

    while (true) {
        fputs("> ", stderr);

        /* ユーザ入力の取得 */
        if ((input = get_line()) == NULL)
            break;

        /* 末尾の改行を除去 */
        chomp(input);

        /* 入力文字列をトークン列に変換 */
        if (!get_token_stream(input, &tok_stream))
            break;

        fprintf(stderr, "input command: %s\n", input);
        dump_token_stream(stderr, &tok_stream);
        
        /* コマンドの構文解析 */
        if (!parse_command(&tok_stream, &cmd)) {
            free_command(&cmd);
            free_token_stream(&tok_stream);
            free(input);
            continue;
        }
        
        dump_command(stderr, &cmd);

        free_command(&cmd);
        free_token_stream(&tok_stream);
        free(input);
    }

    fputc('\n', stderr);

    return EXIT_SUCCESS;
}
