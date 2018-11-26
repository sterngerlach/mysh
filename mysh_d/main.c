
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* main.c */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "lexer.h"
#include "parser.h"
#include "shell.h"
#include "util.h"

int main(int argc, char** argv)
{
    char* input;
    struct token_stream tok_stream;
    struct command cmd;
    bool is_exit;

    /* コマンドライン引数の解析 */
    parse_cmdline(argc, argv);

    /* シグナルハンドラの設定 */
    set_signal_handlers();

    fprintf(stderr, "welcome to mysh!\n");

    while (true) {
        /* プロンプトを表示 */
        prompt();

        /* ユーザ入力の取得 */
        if ((input = get_line()) == NULL)
            break;

        /* 末尾の改行を除去 */
        chomp(input);

        /* 入力文字列を表示 */
        if (app_config.is_debug_mode)
            print_message(__func__, "input command: %s\n", input);

        /* 入力文字列が空である場合 */
        if (!strlen(input)) {
            free(input);
            continue;
        }

        /* 入力文字列をトークン列に変換 */
        if (!get_token_stream(input, &tok_stream)) {
            print_error(__func__, "get_token_stream() failed\n");
            free_token_stream(&tok_stream);
            free(input);
            continue;
        } else {
            if (app_config.is_debug_mode) {
                print_message(__func__, "get_token_stream() succeeded\n");
                dump_token_stream(stderr, &tok_stream);
            }
        }
        
        /* コマンドの構文解析 */
        if (!parse_command(&tok_stream, &cmd)) {
            print_error(__func__, "parse_command() failed\n");
            free_command(&cmd);
            free_token_stream(&tok_stream);
            free(input);
            continue;
        } else {
            if (app_config.is_debug_mode) {
                print_message(__func__, "parse_command() succeeded\n");
                dump_command(stderr, &cmd);
            }
        }

        /* コマンドを展開 */
        if (!expand_command(&cmd)) {
            print_error(__func__, "expand_command() failed\n");
            free_command(&cmd);
            free_token_stream(&tok_stream);
            free(input);
            continue;
        } else {
            if (app_config.is_debug_mode) {
                print_message(__func__, "expand_command() succeeded\n");
                dump_command(stderr, &cmd);
            }
        }
        
        /* コマンドを実行 */
        execute_command(&cmd, &is_exit);

        if (is_exit)
            break;

        free_command(&cmd);
        free_token_stream(&tok_stream);
        free(input);
    }

    fputs("Bye\n", stderr);

    return EXIT_SUCCESS;
}
