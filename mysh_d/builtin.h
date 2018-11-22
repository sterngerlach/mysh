
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* builtin.h */

#ifndef BUILTIN_H
#define BUILTIN_H

#include <stdbool.h>

/*
 * ビルトインコマンドの関数の型宣言
 */
typedef void (*builtin_command)(int argc, char** args, bool* is_exit);

/*
 * ビルトインコマンドを格納する構造体
 */
struct builtin_command_table {
    char*           name;   /* コマンド名 */
    builtin_command func;   /* コマンドの関数ポインタ */
};

/*
 * ビルトインコマンドの一覧
 */
extern struct builtin_command_table builtin_commands[];

/*
 * ビルトインコマンドの検索
 */
builtin_command search_builtin_command(const char* command_name);

/*
 * cdコマンドの処理
 */
void builtin_cd(int argc, char** args, bool* is_exit);

/*
 * pwdコマンドの処理
 */
void builtin_pwd(int argc, char** args, bool* is_exit);

/*
 * exitコマンドの処理
 */
void builtin_exit(int argc, char** args, bool* is_exit);

#endif /* BUILTIN_H */

