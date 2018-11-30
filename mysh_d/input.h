
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* input.h */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>

#include "history.h"

/*
 * 入力されたコマンドの履歴
 */
extern struct command_history_entry command_history_head;

/*
 * 起動時の端末ラインディシプリンの設定
 */
extern struct termios termios_stdin_old;

/*
 * 指定されたファイルディスクリプタの端末ラインディシプリンを復元
 */
void tty_reset_fd(int fd, struct termios* termios_old);

/*
 * 端末の設定を復元
 */
void tty_reset();

/*
 * 端末をcbreakモードに設定
 * シェルプログラムではこちらのモードを使用
 */
bool tty_set_cbreak();

/*
 * 端末をrawモードに設定
 * シェルプログラムでは使用していない
 */
bool tty_set_raw();

/*
 * ユーザの入力文字列を取得
 */
char* get_line();

/*
 * 入力を1文字取得
 */
int read_key_input();

/*
 * 同じ文字を指定された回数だけ出力
 */
void repeat_putc(int ch, size_t times);

/*
 * 同じ文字列を指定された回数だけ出力
 */
void repeat_puts(char* str, size_t times);

/*
 * ユーザの入力文字列を取得(cbreakモード)
 */
char* get_line_cbreak();

/*
 * プロンプトを表示
 */
void prompt(size_t* len);

#endif /* INPUT_H */

