
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* builtin.c */

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "builtin.h"
#include "util.h"

/*
 * ビルトインコマンドの一覧
 */
struct builtin_command_table builtin_commands[] = {
    { "cd",     builtin_cd },
    { "exit",   builtin_exit },
    { NULL,     NULL },
};

/*
 * ビルトインコマンドの検索
 */
builtin_command search_builtin_command(const char* command_name)
{
    struct builtin_command_table* cmd;

    for (cmd = builtin_commands; cmd->name != NULL; ++cmd)
        if (!strcmp(command_name, cmd->name))
            return cmd->func;

    return NULL;
}

/*
 * cdコマンドの処理
 */
void builtin_cd(int argc, char** args, bool* is_exit)
{
    char* env_home;
    struct passwd* pw;

    /* 引数が多すぎる場合はエラー */
    if (argc > 2) {
        print_error(__func__, "cd command takes 1 argument but %d were given\n", argc - 1);
        return;
    }

    /* 引数がない場合はホームディレクトリに移動 */
    if (argc < 2) {
        /* 環境変数HOMEの値を取得 */
        if ((env_home = getenv("HOME")) == NULL) {
            print_error(__func__, "getenv() failed: %s\n", strerror(errno));
            
            /* ホームディレクトリをgetenv関数で取得できない場合は別の方法を試す */
            if ((pw = getpwuid(getuid())) == NULL) {
                print_error(__func__, "getpwuid() failed, could not get HOME: %s\n", strerror(errno));
                return;
            } else {
                env_home = pw->pw_dir;
            }
        }

        /* カレントディレクトリを変更 */
        if (chdir(env_home) < 0)
            print_error(__func__, "chdir() failed: %s\n", strerror(errno));
    } else {
        /* 指定された引数のディレクトリに移動 */
        if (chdir(args[1]) < 0)
            print_error(__func__, "chdir() failed: %s\n", strerror(errno));
    }
}

/*
 * exitコマンドの処理
 */
void builtin_exit(int argc, char** args, bool* is_exit)
{
    *is_exit = true;
}

