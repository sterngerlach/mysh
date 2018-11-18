
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* input.c */

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "util.h"

/*
 * ユーザの入力文字列を取得
 */
char* get_line()
{
    char* input = NULL;
    size_t length = 0;
    ssize_t cc = 0;

    if ((cc = getline(&input, &length, stdin)) == -1) {
        free(input);
        return NULL;
    }

    return input;
}

/*
 * プロンプトを表示
 */
void prompt()
{
    char cwd[PATH_MAX + 1];
    char hostname[HOST_NAME_MAX + 1];
    char username[LOGIN_NAME_MAX + 1];
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

    fprintf(stderr,
        ANSI_ESCAPE_COLOR_CYAN "%s@%s" ANSI_ESCAPE_COLOR_RESET ":"
        ANSI_ESCAPE_COLOR_RED "%s" ANSI_ESCAPE_COLOR_RESET "> ",
        strlen(hostname) > 0 ? hostname : "-",
        strlen(username) > 0 ? username : "-",
        strlen(p) > 0 ? p : "-");
}

