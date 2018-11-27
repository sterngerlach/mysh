
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
 * 指定されたファイル記述子を新たなファイル記述子にコピーして閉じる
 */
int dup2_close(int old_fd, int new_fd);

/*
 * 子プロセスが終了するまで待機
 */
int wait_child_process(pid_t cpid, int* status);

/*
 * コマンドを展開
 */
bool expand_command(struct command* cmd);

/*
 * チルダで開始する文字列を展開
 */
bool expand_tilde(const char* str, char** result);

/*
 * 環境変数を展開
 */
bool expand_variable(const char* str, char** result);

/*
 * ワイルドカードを展開
 * ワイルドカードの展開を一から実装するのは大変であるため,
 * 代わりにGNU C ライブラリ(glibc)のglob()関数を使用している
 */
bool expand_wildcard(
    const struct simple_command* simple_cmd,
    struct simple_command* result);

/*
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit);

#endif /* SHELL_H */

