
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* input.c */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "dynamic_string.h"
#include "history.h"
#include "input.h"
#include "linked_list.h"
#include "util.h"

/*
 * 入力されたコマンドの履歴
 */
struct command_history_entry command_history_head;

/*
 * 起動時の端末ラインディシプリンの設定
 */
struct termios termios_stdin_old;

/*
 * 指定されたファイルディスクリプタの端末ラインディシプリンを復元
 */
void tty_reset_fd(int fd, struct termios* termios_old)
{
    if (tcsetattr(fd, TCSAFLUSH, termios_old) < 0)
        print_error(__func__, "tcsetattr() failed: %s\n", strerror(errno));
}

/*
 * 端末の設定を復元
 */
void tty_reset()
{
    tty_reset_fd(STDIN_FILENO, &termios_stdin_old);
}

/*
 * 端末をcbreakモードに設定
 * シェルプログラムではこちらのモードを使用
 */
bool tty_set_cbreak()
{
    struct termios termios_cbreak;
    
    /* 現在の端末ラインディシプリンの設定を保存 */
    if (tcgetattr(STDIN_FILENO, &termios_stdin_old) < 0) {
        print_error(__func__, "tcgetattr() failed: %s\n", strerror(errno));
        return false;
    }

    /* シェルの終了時に端末の設定を自動的に復元 */
    if (atexit(tty_reset) != 0) {
        print_error(__func__, "atexit() failed, cannot set exit handler\n");
        return false;
    }

    termios_cbreak = termios_stdin_old;

    /* 入力文字のエコーをオフ, 端末を非カノニカルモードに設定 */
    termios_cbreak.c_lflag &= ~(ECHO | ICANON);

    /* 最低1文字読むまで待機 */
    termios_cbreak.c_cc[VMIN] = 1;
    termios_cbreak.c_cc[VTIME] = 0;

    /* 端末をcbreakモードに設定 */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_cbreak) < 0) {
        print_error(__func__, "tcsetattr() failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/*
 * 端末をrawモードに設定
 * シェルプログラムでは使用していない
 */
bool tty_set_raw()
{
    struct termios termios_raw;

    /* 現在の端末ラインディシプリンの設定を保存 */
    if (tcgetattr(STDIN_FILENO, &termios_stdin_old) < 0) {
        print_error(__func__, "tcgetattr() failed: %s\n", strerror(errno));
        return false;
    }

    /* シェルの終了時に端末の設定を自動的に復元 */
    if (atexit(tty_reset) != 0) {
        print_error(__func__, "atexit() failed, cannot set exit handler\n");
        return false;
    }

    termios_raw = termios_stdin_old;
    
    /* 入力文字のエコーをオフ, 端末を非カノニカルモード, 
     * 入力処理の拡張機能を無効, キーボードからのシグナル送信を無効に設定 */
    termios_raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* ブレーク状態におけるSIGINTシグナルの生成を無効, 
     * 復帰文字(Ctrl-M)の入力の改行文字(Ctrl-J)への変換を無効,
     * 入力の8ビット目のクリアを無効, XON/XOFFフロー制御を無効に設定 */
    termios_raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* 1バイト当たりの転送ビット数を変更するためのマスクを設定 */
    /* 出力でのパリティ符号の生成と入力でのパリティ検査を無効に設定 */
    termios_raw.c_lflag &= ~(CSIZE | PARENB);

    /* 1バイト当たりの転送ビット数を8ビットに設定 */
    termios_raw.c_lflag |= CS8;

    /* 出力モードのフラグの機能を無効化 */
    termios_raw.c_lflag &= ~(OPOST);

    /* 最低1文字読むまで待機 */
    termios_raw.c_cc[VMIN] = 1;
    termios_raw.c_cc[VTIME] = 0;

    /* 端末をrawモードに設定 */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_raw) < 0) {
        print_error(__func__, "tcsetattr() failed: %s\n", strerror(errno));
        return false;
    }
    
    return true;
}

/*
 * ユーザの入力文字列を取得
 */
char* get_line()
{
    char* input = NULL;
    size_t length = 0;
    ssize_t cc = 0;

    /* プロンプトの表示 */
    prompt(NULL);

    if ((cc = getline(&input, &length, stdin)) == -1) {
        free(input);
        return NULL;
    }

    /* 改行を除去 */
    chomp(input);

    /* 履歴に追加 */
    update_command_history(&command_history_head, input);

    return input;
}

/*
 * 入力を1文字取得
 */
int read_key_input()
{
    int ch;

    /* 入力を1文字分取得 */
    while ((ch = fgetc(stdin)) == EOF) {
        if (errno == EAGAIN)
            continue;
        else
            return -1;
    }

    return ch;
}

/*
 * 同じ文字を指定された回数だけ出力
 */
void repeat_putc(int ch, size_t times)
{
    size_t i;
    for (i = 0; i < times; ++i)
        fputc(ch, stderr);
}

/*
 * 同じ文字列を指定された回数だけ出力
 */
void repeat_puts(char* str, size_t times)
{
    size_t i;
    for (i = 0; i < times; ++i)
        fputs(str, stderr);
}

/*
 * 印字可能な文字(制御文字でない文字)の処理
 */
bool handle_printable_character(
    struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    size_t old_buffer_len;

    /* 末尾ではない位置で入力された場合 */
    if (*pos < input_buffer->length) {
        /* 以前の入力バッファの長さを保存 */
        old_buffer_len = input_buffer->length;

        /* バッファに入力された文字を追加 */
        if (!dynamic_string_insert_char(input_buffer, ch, *pos)) {
            print_error(__func__, "dynamic_string_insert_char() failed\n");
            free_dynamic_string(input_buffer);
            return false;
        }

        /* スペースを印字してカーソルの右側の文字を消去 */
        repeat_putc(' ', old_buffer_len - *pos);

        /* カーソルの位置に移動 */
        repeat_puts("\x1b[1D", old_buffer_len - *pos);

        /* 入力の内容を表示 */
        fputs(input_buffer->buffer + *pos, stderr);
        
        /* カーソル位置を更新 */
        (*pos)++;

        /* カーソルの位置に移動 */
        repeat_puts("\x1b[1D", input_buffer->length - *pos);
    } else {
        /* 末端にカーソルがある状態で入力された場合 */
        /* バッファに入力された文字を追加 */
        if (!dynamic_string_append_char(input_buffer, ch)) {
            print_error(__func__, "dynamic_string_append_char() failed\n");
            free_dynamic_string(input_buffer);
            return false;
        }

        /* 入力された文字を画面に表示 */
        fputc(ch, stderr);

        /* カーソル位置を更新 */
        (*pos)++;
    }

    return true;
}

/*
 * Ctrl-Aの処理(カーソルを左端に戻す処理)
 */
bool handle_ctrl_a(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    (void)input_buffer;
    (void)ch;

    /* バックスペースを印字して左端へ移動 */
    repeat_puts("\x1b[1D", *pos);

    /* カーソル位置を更新 */
    *pos = 0;

    return true;
}

/*
 * Ctrl-Eの処理(カーソルを右端に移す処理)
 */
bool handle_ctrl_e(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    (void)ch;

    /* カーソルを右端へ移動 */
    repeat_puts("\x1b[1C", input_buffer->length - *pos);

    /* カーソル位置を更新 */
    *pos = input_buffer->length;

    return true;
}

/*
 * Ctrl-Bの処理(カーソルを1つ左に移動させる処理)
 */
bool handle_ctrl_b(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    (void)ch;

    /* バッファが空である場合は無視 */
    if (input_buffer->length == 0) {
        fputc('\a', stderr);
        return true;
    }

    /* カーソルが左端にある場合は無視 */
    if (*pos < 1) {
        fputc('\a', stderr);
        return true;
    }

    /* カーソルを1つ左へ進める */
    fputc('\b', stderr);
    (*pos)--;

    return true;
}

/*
 * Ctrl-Fの処理(カーソルを1つ右に移動させる処理)
 */
bool handle_ctrl_f(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    (void)ch;

    /* バッファが空である場合は無視 */
    if (input_buffer->length == 0) {
        fputc('\a', stderr);
        return true;
    }

    /* カーソルが右端にある場合は無視 */
    if (*pos >= input_buffer->length) {
        fputc('\a', stderr);
        return true;
    }

    /* カーソルを1つ右へ進める */
    fputs("\x1b[1C", stderr);
    (*pos)++;

    return true;
}

/*
 * Ctrl-Dの処理(カーソル位置の文字を消去)
 */
bool handle_ctrl_d(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    (void)ch;

    size_t old_buffer_len;

    /* バッファが空である場合は無視 */
    if (input_buffer->length == 0) {
        fputc('\a', stderr);
        return true;
    }
    
    /* カーソルが右端にある場合は無視 */
    if (*pos >= input_buffer->length) {
        fputc('\a', stderr);
        return true;
    }

    /* 以前の入力バッファの長さを保存 */
    old_buffer_len = input_buffer->length;

    /* バッファからカーソル位置の文字を消去 */
    if (!dynamic_string_remove_at(input_buffer, *pos)) {
        print_error(__func__, "dynamic_string_remove_at() failed\n");
        free_dynamic_string(input_buffer);
        return false;
    }

    /* スペースを印字してカーソルの右側の文字を消去 */
    repeat_putc(' ', old_buffer_len - *pos);

    /* カーソルの位置に移動 */
    repeat_puts("\x1b[1D", old_buffer_len - *pos);

    /* 入力の内容を表示 */
    fputs(input_buffer->buffer + *pos, stderr);

    /* カーソルの位置に移動 */
    repeat_puts("\x1b[1D", input_buffer->length - *pos);

    return true;
}

/*
 * Tabキーの処理(ファイル名の自動補完)
 */
bool handle_tab(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    DIR* dir;
    struct dirent* ent;
    char* match_name = NULL;
    char* begin = NULL;
    int i = 0;
    size_t match_num = 0;

    (void)ch;

    /* バッファが空である場合は無視 */
    if (input_buffer->length == 0) {
        fputc('\a', stderr);
        return true;
    }

    /* 入力の末尾の部分のみ取得 */
    begin = input_buffer->buffer;

    for (i = input_buffer->length - 1; i >= 0; --i) {
        if (strchr(" \t", input_buffer->buffer[i]) != NULL) {
            begin = input_buffer->buffer + i + 1;
            break;
        }
    }

    /* 入力の末尾がスペースまたはタブである場合は何もしない */
    if (!(*begin)) {
        fputc('\a', stderr);
        return true;
    }
    
    /* カレントディレクトリが開けない場合は何もしない */
    if ((dir = opendir(".")) == NULL)
        return true;

    /* ディレクトリ内のファイルを読み込み(エラーは無視) */
    while ((ent = readdir(dir)) != NULL) {
        /* 入力がファイル名の最初の部分に一致する場合 */
        if (!strncmp(ent->d_name, begin, strlen(begin))) {
            match_num++;
            match_name = ent->d_name;
        }
    }

    /* 候補が1つでない場合は何もしない */
    if (match_num != 1)
        return true;

    /* ファイル名に完全に一致する場合は何もしない */
    if (strlen(begin) == strlen(match_name))
        return true;

    /* カーソルを右端に移動 */
    repeat_puts("\x1b[1C", input_buffer->length - *pos);
    
    /* カーソルをファイル名の始まりに移動 */
    repeat_puts("\x1b[1D", strlen(begin));

    /* 入力されたファイル名を全て消去 */
    repeat_putc(' ', strlen(begin));

    /* カーソルをファイル名の始まりに移動 */
    repeat_puts("\x1b[1D", strlen(begin));

    /* 合致したファイル名で入力バッファを置き換え */
    if (!dynamic_string_remove_range(
        input_buffer, begin - input_buffer->buffer, strlen(begin))) {
        print_error(__func__, "dynamic_string_remove_all() failed\n");
        return false;
    }

    if (!dynamic_string_append(input_buffer, match_name)) {
        print_error(__func__, "dynamic_string_append() failed\n");
        return false;
    }
    
    /* ファイル名を表示 */
    fprintf(stderr, "%s", match_name);

    /* カーソル位置を更新 */
    *pos = input_buffer->length;

   return true;
}

/*
 * Enterキーの処理(改行文字をバッファに追加)
 */
bool handle_enter(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history)
{
    (void)pos;

    /* バッファが空である場合はヌル文字を追加 */
    /* バッファに文字や文字列が追加されないとき, バッファはNULLのままであるため,
     * 空文字列が入力されると(何も入力しない状態でEnterキーが押されると), 
     * get_line_cbreak()関数がバッファへのポインタとしてNULLを返すことになり,
     * 呼び出し側がエラーと判断してしまうのを防ぐ */

    /* 適当な文字を追加することでバッファが自動的に拡張されて, 
     * get_line_cbreak()関数を呼び出したときにNULLではない値が返る */
    if (input_buffer->length == 0) {
        if (!dynamic_string_append_char(input_buffer, '\0')) {
            print_error(__func__, "dynamic_string_append_char() failed\n");
            free_dynamic_string(input_buffer);
            return false;
        }
    }

    /* 入力された文字を画面に表示 */
    fputc(ch, stderr);

    /* 履歴を更新 */
    if (!update_command_history(&command_history_head, input_buffer->buffer)) {
        print_error(__func__, "update_command_history() failed\n");
        free_dynamic_string(input_buffer);
        return false;
    }
    
    /* 履歴の表示で使用する最初の履歴を更新 */
    *current_history = get_last_list_entry(
        &command_history_head, struct command_history_entry, entry);

    return true;
}

/*
 * Backspaceキーの処理(カーソルの直前の文字を消去)
 */
bool handle_backspace(struct dynamic_string* input_buffer, size_t* pos, int ch)
{
    size_t old_buffer_len;

    (void)ch;

    /* バッファが空である場合は無視 */
    if (input_buffer->length == 0) {
        fputc('\a', stderr);
        return true;
    }

    /* カーソルが左端にある場合は無視 */
    if (*pos < 1) {
        fputc('\a', stderr);
        return true;
    }

    /* 以前の入力バッファの長さを保存 */
    old_buffer_len = input_buffer->length;

    /* バッファからカーソル位置の直前の文字を消去 */
    if (!dynamic_string_remove_at(input_buffer, *pos - 1)) {
        print_error(__func__, "dynamic_string_remove_at() failed\n");
        free_dynamic_string(input_buffer);
        return false;
    }
    
    /* 1文字分左に移動 */
    fputc('\b', stderr);

    /* カーソル位置を更新 */
    (*pos)--;

    /* スペースを印字してカーソルの右側の文字を消去 */
    repeat_putc(' ', old_buffer_len - *pos);

    /* カーソルの位置に移動 */
    repeat_puts("\x1b[1D", old_buffer_len - *pos);

    /* 入力の内容を表示 */
    fputs(input_buffer->buffer + *pos, stderr);

    /* カーソルの位置に移動 */
    repeat_puts("\x1b[1D", input_buffer->length - *pos);

    return true;
}

/*
 * 上方向キーの処理(以前に入力したコマンドの履歴を表示
 */
bool handle_arrow_up(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history)
{
    (void)ch;

    /* 表示する履歴がなければ無視 */
    if (is_list_empty(&command_history_head.entry))
        return true;

    /* 既に履歴を一巡した場合は末尾の履歴から再度表示 */
    if (&((*current_history)->entry) == &command_history_head.entry)
        *current_history = get_prev_list_entry(
            *current_history, struct command_history_entry, entry);

    /* カーソルを右端に移動 */
    repeat_puts("\x1b[1C", input_buffer->length - *pos);

    /* カーソルを左端に移動 */
    repeat_puts("\x1b[1D", input_buffer->length);

    /* 入力されたコマンドを全て消去 */
    repeat_putc(' ', input_buffer->length);

    /* カーソルを左端に移動 */
    repeat_puts("\x1b[1D", input_buffer->length);

    /* バッファの内容を履歴で置き換え */
    if (input_buffer->length > 0) {
        if (!dynamic_string_remove_all(input_buffer)) {
            print_error(__func__, "dynamic_string_remove_all() failed\n");
            return false;
        }
    }

    if (!dynamic_string_append(input_buffer, (*current_history)->command)) {
        print_error(__func__, "dynamic_string_append() failed\n");
        return false;
    }
    
    /* コマンドの履歴を表示 */
    fprintf(stderr, "%s", input_buffer->buffer);

    /* カーソル位置を更新 */
    *pos = input_buffer->length;

    /* 表示する履歴を前に進める */
    *current_history = get_prev_list_entry(
        *current_history, struct command_history_entry, entry);
   
    return true;
}

/*
 * 下方向キーの処理(以前に入力したコマンドの履歴を表示)
 */
bool handle_arrow_down(
    struct dynamic_string* input_buffer, size_t* pos, int ch,
    struct command_history_entry** current_history)
{
    (void)ch;

    /* 表示する履歴がなければ無視 */
    if (is_list_empty(&command_history_head.entry))
        return true;

    /* 既に履歴を一巡した場合は先頭の履歴から再度表示 */
    if (&((*current_history)->entry) == &command_history_head.entry)
        *current_history = get_next_list_entry(
            *current_history, struct command_history_entry, entry);

    /* カーソルを右端に移動 */
    repeat_puts("\x1b[1C", input_buffer->length - *pos);

    /* カーソルを左端に移動 */
    repeat_puts("\x1b[1D", input_buffer->length);

    /* 入力されたコマンドを全て消去 */
    repeat_putc(' ', input_buffer->length);

    /* カーソルを左端に移動 */
    repeat_puts("\x1b[1D", input_buffer->length);

    /* バッファの内容を履歴で置き換え */
    if (input_buffer->length > 0) {
        if (!dynamic_string_remove_all(input_buffer)) {
            print_error(__func__, "dynamic_string_remove_all() failed\n");
            return false;
        }
    }

    if (!dynamic_string_append(input_buffer, (*current_history)->command)) {
        print_error(__func__, "dynamic_string_append() failed\n");
        return false;
    }
    
    /* コマンドの履歴を表示 */
    fprintf(stderr, "%s", input_buffer->buffer);

    /* カーソル位置を更新 */
    *pos = input_buffer->length;

    /* 表示する履歴を次に進める */
    *current_history = get_next_list_entry(
        *current_history, struct command_history_entry, entry);
    
    return true;
}

/*
 * ユーザの入力文字列を取得(cbreakモード)
 */
char* get_line_cbreak()
{
    int ch;
    int seq[3];
    size_t pos;
    size_t prompt_len;

    struct dynamic_string input_buffer;
    
    static bool is_current_history_initialized = false;
    static struct command_history_entry* current_history;

    /* 履歴の表示で使用する最初の履歴を設定 */
    if (!is_current_history_initialized) {
        current_history = &command_history_head;
        is_current_history_initialized = true;
    }

    /* 動的文字列を初期化 */
    if (!initialize_dynamic_string(&input_buffer)) {
        print_error(__func__, "initialize_dynamic_string() failed\n");
        return NULL;
    }

    /* プロンプトを表示 */
    prompt(&prompt_len);
    
    /* 現在のカーソル位置を初期化 */
    pos = 0;

    /* Enterキーが押されるまで入力を取得 */
    while (1) {
        /* 入力を1文字取得 */
        if ((ch = read_key_input()) < 0)
            return NULL;
        
        /* 入力が制御文字でない(印字可能な文字である)場合 */
        if (!iscntrl(ch)) {
            handle_printable_character(&input_buffer, &pos, ch);
        } else if (ch == '\x01') {
            /* Ctrl-Aの処理 */
            if (!handle_ctrl_a(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_ctrl_a() failed\n");
                return NULL;
            }
        } else if (ch == '\x05') {
            /* Ctrl-Eの処理 */
            if (!handle_ctrl_e(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_ctrl_e() failed\n");
                return NULL;
            }
        } else if (ch == '\x02') {
            /* Ctrl-Bの処理 */
            if (!handle_ctrl_b(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_ctrl_b() failed\n");
                return NULL;
            }
        } else if (ch == '\x06') {
            /* Ctrl-Fの処理 */
            if (!handle_ctrl_f(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_ctrl_f() failed\n");
                return NULL;
            }
        } else if (ch == '\x04') {
            /* Ctrl-Dの処理 */
            if (!handle_ctrl_d(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_ctrl_d() failed\n");
                return NULL;
            }
        } else if (ch == '\t') {
            /* Tabキーの処理 */
            if (!handle_tab(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_tab() failed\n");
                return NULL;
            }
        } else if (ch == '\n') {
            /* Enterキーの処理 */
            if (!handle_enter(&input_buffer, &pos, ch, &current_history)) {
                print_error(__func__, "handle_enter() failed\n");
                return NULL;
            }

            /* 動的文字列が保持していたバッファを返す */
            return move_dynamic_string(&input_buffer);
        } else if (ch == '\x08' || ch == 0x7F) {
            /* Backspaceキーの処理 */
            if (!handle_backspace(&input_buffer, &pos, ch)) {
                print_error(__func__, "handle_backspace() failed\n");
                return NULL;
            }
        } else if (ch == 14) {
            /* Ctrl-Nの処理 */
            if (!handle_arrow_down(&input_buffer, &pos, ch, &current_history)) {
                print_error(__func__, "handle_arrow_down() failed\n");
                return NULL;
            }
        } else if (ch == 16) {
            /* Ctrl-Pの処理 */
            if (!handle_arrow_up(&input_buffer, &pos, ch, &current_history)) {
                print_error(__func__, "handle_arrow_up() failed\n");
                return NULL;
            }
        } else if (ch == '\x1b') {
            /* Escキーの処理 */

            /* 入力を2文字取得 */
            if ((seq[0] = read_key_input()) < 0)
                return move_dynamic_string(&input_buffer);

            if ((seq[1] = read_key_input()) < 0)
                return move_dynamic_string(&input_buffer);

            if (seq[0] == '[') {
                if (isdigit(seq[1])) {
                    /* 入力を1文字取得 */
                    if ((seq[2] = read_key_input()) < 0)
                        return move_dynamic_string(&input_buffer);

                    if (seq[2] == '~') {
                        switch (seq[1]) {
                            case '1':
                            case '7':
                                /* Homeキーの処理 */
                                if (!handle_ctrl_a(&input_buffer, &pos, ch)) {
                                    print_error(__func__, "handle_ctrl_a() failed\n");
                                    return NULL;
                                }
                                break;
                            case '2':
                            case '8':
                                /* Endキーの処理 */
                                if (!handle_ctrl_e(&input_buffer, &pos, ch)) {
                                    print_error(__func__, "handle_ctrl_e() failed\n");
                                    return NULL;
                                }
                                break;
                        }
                    }
                } else {
                    switch (seq[1]) {
                        case 'A':
                            /* 上矢印キーの処理 */
                            if (!handle_arrow_up(&input_buffer, &pos, ch, &current_history)) {
                                print_error(__func__, "handle_arrow_up() failed\n");
                                return NULL;
                            }
                            break;
                        case 'B':
                            /* 下矢印キーの処理 */
                            if (!handle_arrow_down(&input_buffer, &pos, ch, &current_history)) {
                                print_error(__func__, "handle_arrow_down() failed\n");
                                return NULL;
                            }
                            break;
                        case 'C':
                            /* 右矢印キーの処理 */
                            if (!handle_ctrl_f(&input_buffer, &pos, ch)) {
                                print_error(__func__, "handle_ctrl_f() failed\n");
                                return NULL;
                            }
                            break;
                        case 'D':
                            /* 左矢印キーの処理 */
                            if (!handle_ctrl_b(&input_buffer, &pos, ch)) {
                                print_error(__func__, "handle_ctrl_b() failed\n");
                                return NULL;
                            }
                            break;
                        case 'H':
                            /* Homeキーの処理 */
                            if (!handle_ctrl_a(&input_buffer, &pos, ch)) {
                                print_error(__func__, "handle_ctrl_a() failed\n");
                                return NULL;
                            }
                            break;
                        case 'F':
                            /* Endキーの処理 */
                            if (!handle_ctrl_e(&input_buffer, &pos, ch)) {
                                print_error(__func__, "handle_ctrl_e() failed\n");
                                return NULL;
                            }
                            break;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                    case 'H':
                        /* Homeキーの処理 */
                        if (!handle_ctrl_a(&input_buffer, &pos, ch)) {
                            print_error(__func__, "handle_ctrl_a() failed\n");
                            return NULL;
                        }
                        break;
                    case 'F':
                        /* Endキーの処理 */
                        if (!handle_ctrl_e(&input_buffer, &pos, ch)) {
                            print_error(__func__, "handle_ctrl_e() failed\n");
                            return NULL;
                        }
                        break;
                }
            }
        }
    }
    
    return NULL;
}

/*
 * プロンプトを表示
 */
void prompt(size_t* len)
{
    static char cwd[PATH_MAX + 1];
    static char hostname[HOST_NAME_MAX + 1];
    static char username[LOGIN_NAME_MAX + 1];
    
    int ret;
    char* env_hostname;
    char* env_user;
    char* env_home;
    char* p;
    struct passwd* pw;

    memset(hostname, 0, sizeof(hostname));
    memset(username, 0, sizeof(username));
    
    /* 環境変数HOSTNAMEの値を取得 */
    if ((env_hostname = getenv("HOSTNAME")) != NULL) {
        strcpy(hostname, env_hostname);
    } else {
        /* ホスト名をgetenv関数で取得できない場合は別の方法を試す */
        if (gethostname(hostname, HOST_NAME_MAX) < 0) {
            print_error(__func__, "gethostname() failed, could not get hostname: %s\n", strerror(errno));
            *hostname = '\0';
        }
    }
    
    /* 環境変数USERの値を取得 */
    if ((env_user = getenv("USER")) != NULL) {
        strcpy(username, env_user);
    } else {
        /* ユーザ名をgetenv関数で取得できない場合は別の方法を試す */
        if (getlogin_r(username, LOGIN_NAME_MAX) != 0) {
            print_error(__func__, "getlogin_r() failed: %s\n", strerror(errno));
            *username = '\0';
        }
    }

    /* 環境変数HOMEの値を取得 */
    if ((env_home = getenv("HOME")) == NULL) {
        /* ホームディレクトリをgetenv関数で取得できない場合は別の方法を試す */
        if ((pw = getpwuid(getuid())) == NULL) {
            print_error(__func__, "getpwuid() failed, could not get HOME: %s\n", strerror(errno));
            env_home = NULL;
        } else {
            env_home = pw->pw_dir;
        }
    }
    
    /* カレントディレクトリを取得 */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error(__func__, "getcwd() failed: %s\n", strerror(errno));
        *cwd = '\0';
    }

    /* カレントディレクトリがホームディレクトリ内にある場合は文字列を置き換え */
    if (starts_with(cwd, env_home)) {
        cwd[strlen(env_home) - 1] = '~';
        p = cwd + strlen(env_home) - 1;
    } else {
        p = cwd;
    }
    
    /* 出力の長さ(末尾のヌル文字は除く)を取得して引数に設定 */
    if (len != NULL) {
        ret = snprintf(NULL, 0, "%s@%s:%s> ",
                       strlen(hostname) > 0 ? hostname : "-",
                       strlen(username) > 0 ? username : "-",
                       strlen(p) > 0 ? p : "-");
        
        if (ret < 0)
            print_error(__func__, "snprintf() failed: %s\n", strerror(errno));
        else
            *len = (size_t)ret;
    }
    
    /* プロンプトを表示 */
    fprintf(stderr,
        ANSI_ESCAPE_COLOR_CYAN "%s@%s" ANSI_ESCAPE_COLOR_RESET ":"
        ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET "> ",
        strlen(hostname) > 0 ? hostname : "-",
        strlen(username) > 0 ? username : "-",
        strlen(p) > 0 ? p : "-");
}

