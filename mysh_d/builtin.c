
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* builtin.c */

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "builtin.h"
#include "history.h"
#include "input.h"
#include "util.h"

/*
 * ビルトインコマンドの一覧
 */
struct builtin_command_table builtin_commands[] = {
    { "cd",                 builtin_cd },
    { "pwd",                builtin_pwd },
    { "history",            builtin_history },
    { "help",               builtin_help },
    { "exit",               builtin_exit },
    { "tera",               builtin_tera },
    { "self-introduction",  builtin_self_introduction },
    { NULL,                 NULL },
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
    static char pwd[PATH_MAX + 1];

    (void)is_exit;

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
        if (chdir(env_home) < 0) {
            print_error(__func__, "chdir() failed: %s\n", strerror(errno));
            return;
        }
    } else {
        /* 指定された引数のディレクトリに移動 */
        if (chdir(args[1]) < 0) {
            print_error(__func__, "chdir() failed: %s\n", strerror(errno));
            return;
        }
    }

    /* カレントディレクトリを取得 */
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        print_error(__func__, "getcwd() failed: %s\n", strerror(errno));
        return;
    }

    /* 環境変数PWDを更新 */
    if (setenv("PWD", pwd, 1) < 0) {
        print_error(__func__, "setenv() failed: %s\n", strerror(errno));
        return;
    }
}

/*
 * pwdコマンドの処理
 */
void builtin_pwd(int argc, char** args, bool* is_exit)
{
    static char pwd[PATH_MAX + 1];

    (void)args;
    (void)is_exit;

    /* 引数が多すぎる場合はエラー */
    if (argc > 1) {
        print_error(__func__, "pwd command takes no arguments but %d were given\n", argc - 1);
        return;
    }

    /* カレントディレクトリを取得 */
    if (getcwd(pwd, sizeof(pwd)) == NULL) {
        print_error(__func__, "getcwd() failed: %s\n", strerror(errno));
        return;
    }

    fprintf(stderr, "%s\n", pwd);
}

/*
 * historyコマンドの処理
 */
void builtin_history(int argc, char** args, bool* is_exit)
{
    struct command_history_entry* iter;
    int index = 0;

    (void)argc;
    (void)args;
    (void)is_exit;
        
    /* 入力されたコマンドの履歴を表示 */
    list_for_each_entry(iter, &command_history_head.entry, struct command_history_entry, entry)
        fprintf(stderr, "%d %s\n", index++, iter->command);
}

/*
 * helpコマンドの処理
 */
void builtin_help(int argc, char** args, bool* is_exit)
{
    (void)argc;
    (void)args;
    (void)is_exit;
    
    fprintf(stderr,
        "mysh: my own shell in 5,000 lines of C code\n"
        "build time: %s - %s\n"
        "usage: mysh [option]...\n\n"
        "available options: \n"
        "    -d, --debug        "
        "launch shell in debug mode (enable verbose outputs)\n"
        "    -r, --raw          "
        "set terminal to cbreak mode (see input.c) and then retrieve the user input\n"
        "                       "
        "Ctrl-A, Ctrl-E, Ctrl-B, Ctrl-F, Ctrl-P, Ctrl-N, Up, Down, Right, Left, "
        "Backspace, Ctrl-H, Del, Ctrl-D are enabled\n\n"
        "built-in commands: \n"
        "    help               "
        "show help\n"
        "    cd <path>          "
        "set current working directory to <path>\n"
        "    pwd                "
        "show current working directory\n"
        "    history            "
        "show shell command history\n"
        "    exit               "
        "exit mysh\n"
        "    tera               "
        "???\n"
        "    self-introduction  "
        "???\n",
        __DATE__, __TIME__);
}

/*
 * exitコマンドの処理
 */
void builtin_exit(int argc, char** args, bool* is_exit)
{
    (void)argc;
    (void)args;

    *is_exit = true;
}

/*
 * teraコマンドの処理
 */
void builtin_tera(int argc, char** args, bool* is_exit)
{
    (void)argc;
    (void)args;
    (void)is_exit;

    fprintf(stderr, "We love Prof. Teraoka!\n");
}

/*
 * self-introductionコマンドの処理
 */
void builtin_self_introduction(int argc, char** args, bool* is_exit)
{
    (void)argc;
    (void)args;
    (void)is_exit;

    fprintf(stderr,
        "I like collecting CDs and vinyl LPs.\n"
        "So, here are my top 20 favorite albums of all time :-)\n\n"
        " 1. Taeko Ohnuki / Purissima (1988)\n"
        " 2. Akiko Yano / Granola (1987)\n"
        " 3. Rajie / Espresso (1985)\n"
        " 4. Tatsuro Yamashita / Boku-no Naka-no Shonen (1988)\n"
        " 5. Taeko Ohnuki / Cliche (1982)\n"
        " 6. Seri Ishikawa / Rakuen (1985)\n"
        " 7. Taeko Ohnuki / A Slice of Life (1987)\n"
        " 8. Akiko Yano / Touge No Wagaya (1986)\n"
        " 9. Pizzicato Five / Sweet Pizzicato Five (1992)\n"
        "10. Yukihiro Takahashi / Once A Fool... (1985)\n"
        "11. Taeko Ohnuki / Signifie (1983)\n"
        "12. Yellow Magic Orchestra / BGM (1981)\n"
        "13. P-Model / Karkador (1985)\n"
        "14. Ryuichi Sakamoto / Ongaku Zukan (1984)\n"
        "15. Akiko Yano / Welcome Back (1989)\n"
        "16. Mariya Takeuchi / Request (1987)\n"
        "17. EPO / Pump! Pump! (1986)\n"
        "18. Minako Yoshida / Dark Crystal (1989)\n"
        "19. Haruomi Hosono / SFX (1984)\n"
        "20. Happy End / Kazemachi Roman (1971)\n\n"
        "I guess you know some of them. Thank you!\n");
}

