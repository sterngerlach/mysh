
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
 * シグナルSIGCHLDの処理
 */
void sigchld_handler(int sig, siginfo_t* info, void* q);

/*
 * シグナルハンドラの設定
 */
void set_signal_handlers();

/*
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit);

#endif /* SHELL_H */

