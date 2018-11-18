
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* shell.h */

#ifndef SHELL_H
#define SHELL_H

#include <signal.h>
#include <stdbool.h>

#include "parser.h"

#define PIPE_WRITE  1
#define PIPE_READ   0

/*
 * シェルの設定を保持する構造体
 */
struct shell_config {
    bool is_debug_mode;     /* デバッグモードかどうか */
};

/*
 * シェルの設定を保持するグローバル変数
 */
extern struct shell_config app_config;

/*
 * コマンドライン引数の解析
 */
void parse_cmdline(int argc, char** argv);

/*
 * シグナルSIGCHLDの処理
 */
void sigchld_handler(int sig, siginfo_t* info, void* q);

/*
 * シグナルハンドラの設定
 */
void set_signal_handlers();

/*
 * コマンドの絶対パスを取得
 */
char* search_path(const char* cmd);

/*
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit);

#endif /* SHELL_H */

