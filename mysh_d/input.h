
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* input.h */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>

#include "dynamic_string.h"
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
 * 印字可能な文字(制御文字でない文字)の処理
 */
bool handle_printable_character(
    struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Ctrl-Aの処理(カーソルを左端に戻す処理)
 */
bool handle_ctrl_a(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Ctrl-Eの処理(カーソルを右端に移す処理)
 */
bool handle_ctrl_e(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Ctrl-Bの処理(カーソルを1つ左に移動させる処理)
 */
bool handle_ctrl_b(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Ctrl-Fの処理(カーソルを1つ右に移動させる処理)
 */
bool handle_ctrl_f(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Ctrl-Dの処理(カーソル位置の文字を消去)
 */
bool handle_ctrl_d(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Tabキーの処理(ファイル名の自動補完)
 */
bool handle_tab(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * Enterキーの処理(改行文字をバッファに追加)
 */
bool handle_enter(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history);

/*
 * Backspaceキーの処理(カーソルの直前の文字を消去)
 */
bool handle_backspace(struct dynamic_string* input_buffer, size_t* pos, int ch);

/*
 * 上方向キーの処理(以前に入力したコマンドの履歴を表示
 */
bool handle_arrow_up(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history);

/*
 * 下方向キーの処理(以前に入力したコマンドの履歴を表示)
 */
bool handle_arrow_down(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history);

/*
 * ユーザの入力文字列を取得(cbreakモード)
 */
char* get_line_cbreak();

/*
 * プロンプトを表示
 */
void prompt(size_t* len);

#endif /* INPUT_H */

