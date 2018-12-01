
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* shell.c */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <glob.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtin.h"
#include "dynamic_string.h"
#include "input.h"
#include "shell.h"
#include "util.h"

/*
 * 環境変数(外部変数)
 */
extern char** environ;

/*
 * シェルの設定を保持するグローバル変数
 */
struct shell_config app_config;

/*
 * コマンドライン引数の解析
 */
void parse_cmdline(int argc, char** argv)
{
    /* オプションのフォーマット文字列 */
    static const char* const option_str = "dr";

    /* 有効なオプションの配列 */
    static const struct option long_options[] = {
        { "debug",  no_argument,    NULL, 'd' },
        { "raw",    no_argument,    NULL, 'r' },
        { NULL,     0,              NULL, 0   }
    };
    
    int opt;
    int opt_index;

    /* シェルの設定を初期化 */
    app_config.is_debug_mode = false;
    app_config.is_raw_mode = false;
    
    /* オプションの取得 */
    while ((opt = getopt_long(argc, argv,
            option_str, long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'd':
                /* シェルをデバッグモードで起動 */
                app_config.is_debug_mode = true;
                break;
            case 'r':
                /* シェルの入力をcbreakモードに設定 */
                app_config.is_raw_mode = true;
                break;
            default:
                /* それ以外のオプションは無視 */
                break;
        }
    }
}

/*
 * シグナルSIGCHLDの処理
 */
void sigchld_handler(int sig, siginfo_t* info, void* q)
{
    pid_t pid;
    int status;

    /* 警告の無視 */
    (void)sig;
    (void)info;
    (void)q;

    while ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
        if (pid == -1) {
            if (errno == ECHILD) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                print_error(__func__, "waitpid() failed: %s\n", strerror(errno));
                break;
            }
        } else {
            if (app_config.is_debug_mode)
                print_message(__func__, "process (pid: %d) exited\n", pid);
        }
    }
}

/*
 * シグナルハンドラの設定
 */
void set_signal_handlers()
{
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(sigact));

    /* シグナルハンドラを起動したシグナル以外はブロックしない */
    sigemptyset(&sigact.sa_mask);

    /* シグナルSIGINTのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sigact, NULL);

    /* シグナルSIGQUITのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = SA_RESTART;

    sigaction(SIGQUIT, &sigact, NULL);

    /* シグナルSIGTSTPのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = SA_RESTART;

    sigaction(SIGTSTP, &sigact, NULL);

    /* シグナルSIGCHLDのハンドラを設定 */
    /* sigact.sa_handler = SIG_DFL;
    sigact.sa_flags = SA_NOCLDWAIT; */

    /* getline関数が途中で割り込まれないようにSA_RESTARTを設定 */
    /* SA_RESTARTを設定していなかったせいで数時間が潰れた */
    sigact.sa_sigaction = sigchld_handler;
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;

    sigaction(SIGCHLD, &sigact, NULL);

    /* シグナルSIGTTINのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = SA_RESTART;

    sigaction(SIGTTIN, &sigact, NULL);
    
    /* シグナルSIGTTOUのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = SA_RESTART;
    
    sigaction(SIGTTOU, &sigact, NULL);
}

/*
 * シグナルハンドラのリセット
 */
void reset_signal_handlers()
{
    struct sigaction sigact;

    memset(&sigact, 0, sizeof(sigact));

    /* シグナルハンドラを起動したシグナル以外はブロックしない */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_handler = SIG_DFL;
    sigact.sa_flags = 0;

    /* シグナルSIGINTのハンドラを設定 */
    sigaction(SIGINT, &sigact, NULL);

    /* シグナルSIGQUITのハンドラを設定 */
    sigaction(SIGQUIT, &sigact, NULL);

    /* シグナルSIGTSTPのハンドラを設定 */
    sigaction(SIGTSTP, &sigact, NULL);

    /* シグナルSIGCHLDのハンドラを設定 */
    sigaction(SIGCHLD, &sigact, NULL);

    /* シグナルSIGTTINのハンドラを設定 */
    sigaction(SIGTTIN, &sigact, NULL);
    
    /* シグナルSIGTTOUのハンドラを設定 */
    sigaction(SIGTTOU, &sigact, NULL);
}

/*
 * コマンドの絶対パスを取得
 */
char* search_path(const char* cmd)
{
    static char path_buf[PATH_MAX + 1];
    char* env_path;
    char* env_path_buf;
    char* path;
    char* str;
    char* saveptr;
    size_t len;
    struct stat buf;
    
    /* cmdが絶対パスである場合はそのまま返す */
    if (cmd[0] == '/') {
        strcpy(path_buf, cmd);
        return path_buf;
    }

    /* カレントディレクトリを取得 */
    if (getcwd(path_buf, sizeof(buf)) == NULL) {
        print_error(__func__, "getcwd() failed: %s\n", strerror(errno));
        return NULL;
    }

    /* カレントディレクトリとコマンド名を結合 */
    if (strlen(path_buf) > 1 && path_buf[strlen(path_buf) - 1] != '/')
        strcat(path_buf, "/");

    strcat(path_buf, cmd);

    /* カレントディレクトリ内にコマンドが存在したら返す */
    if (stat(path_buf, &buf) == 0)
        return path_buf;

    /* 環境変数PATHの値を取得 */
    if ((env_path = getenv("PATH")) != NULL) {
        /* 環境変数PATHをコピー */
        if ((env_path_buf = strdup(env_path)) == NULL) {
            print_error(__func__, "strdup() failed: %s\n", strerror(errno));
            return NULL;
        }
    } else {
        /* 環境変数PATHをgetenv関数で取得できない場合は別の方法を試す */
        len = confstr(_CS_PATH, NULL, 0);

        if (len == 0) {
            print_error(__func__, "confstr() failed, could not get PATH: %s\n", strerror(errno));
            return NULL;
        }

        if ((env_path_buf = (char*)calloc(len, sizeof(char))) == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return NULL;
        }

        /* 環境変数PATHを取得 */
        confstr(_CS_PATH, env_path_buf, len);
    }

    /* 環境変数PATHに含まれるパスを走査 */
    for (str = env_path_buf; ; str = NULL) {
        path = strtok_r(str, ":", &saveptr);

        if (path == NULL)
            break;

        /* パスの末尾のスラッシュを除去 */
        if (strlen(path) > 1 && path[strlen(path) - 1] == '/')
            path[strlen(path) - 1] = '\0';
        
        /* パスとコマンド名を結合 */
        strcpy(path_buf, path);
        strcat(path_buf, "/");
        strcat(path_buf, cmd);
        
        /* パス内にコマンドが存在するかどうかを確認 */
        if (stat(path_buf, &buf) == 0) {
            free(env_path_buf);
            return path_buf;
        }
    }

    free(env_path_buf);

    return NULL;   
}

/*
 * 指定されたファイル記述子を新たなファイル記述子にコピーして閉じる
 */
int dup2_close(int old_fd, int new_fd)
{
    if (old_fd == new_fd)
        return 0;

    if (dup2(old_fd, new_fd) == -1) {
        print_error(__func__, "dup2() failed: %s\n", strerror(errno));
        close(old_fd);
        return -1;
    }
    
    if (close(old_fd) == -1) {
        print_error(__func__, "close() failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/*
 * 子プロセスが終了するまで待機
 */
int wait_child_process(pid_t cpid, int* status)
{
    pid_t wpid;

    while ((wpid = waitpid(cpid, status, 0)) != 0) {
        if (wpid == -1) {
            if (errno == EINTR) {
                /* シグナルに割り込まれたのでやり直し */
                continue;
            } else if (errno == ECHILD) {
                /* 指定されたプロセスIDの子プロセスはいない */
                return 0;
            } else {
                print_error(__func__, "waitpid() failed: %s\n", strerror(errno));
                return -1;
            }
        } else {
            return 0;
        }
    }

    return -1;
}

/*
 * コマンドを展開
 */
bool expand_command(struct command* cmd)
{
    size_t i;
    size_t j;
    size_t k;

    char* expansion_result;

    struct shell_command* shell_cmd;
    struct redirect_info* redir_info;
    struct simple_command* simple_cmd;
    struct simple_command new_simple_cmd;

    /* 展開処理のためにメモリ領域の動的確保と解放が何度も行われているため
     * 大変非効率になっている */
    
    for (i = 0; i < cmd->num_shell_commands; ++i) {
        shell_cmd = &cmd->shell_commands[i];
        redir_info = &shell_cmd->redir_info;

        /* 入力元のファイル名の展開 */
        if (redir_info->input_file_name != NULL) {
            /* チルダで開始する文字列を展開 */
            if (!expand_tilde(redir_info->input_file_name, &expansion_result)) {
                print_error(__func__, "expand_tilde() failed\n");
                return false;
            }

            /* 文字列が置き換わった場合は, 置き換える前の文字列を破棄 */
            /* redir_info->input_file_nameにはstrdup()関数の戻り値が格納されているため,
             * free()関数によって解放することができる */
            if (expansion_result != NULL) {
                /* デバッグ情報の表示 */
                if (app_config.is_debug_mode)
                    print_message(__func__,
                        "tilde expansion of redirect input file name: %s --> %s\n",
                        redir_info->input_file_name, expansion_result);

                free(redir_info->input_file_name);
                redir_info->input_file_name = expansion_result;
            }

            /* 環境変数を展開 */
            if (!expand_variable(redir_info->input_file_name, &expansion_result)) {
                print_error(__func__, "expand_variable() failed\n");
                return false;
            }
            
            /* デバッグ情報の表示 */
            if (app_config.is_debug_mode)
                print_message(__func__,
                    "environment variable expansion of redirect input file name: %s --> %s\n",
                    redir_info->input_file_name, expansion_result);
            
            free(redir_info->input_file_name);
            redir_info->input_file_name = expansion_result;
        }

        /* 出力先のファイル名の展開 */
        if (redir_info->output_file_name != NULL) {
            /* チルダで開始する文字列を展開 */
            if (!expand_tilde(redir_info->output_file_name, &expansion_result)) {
                print_error(__func__, "expand_tilde() failed\n");
                return false;
            }
            
            /* redir_info->output_file_nameにはstrdup()関数の戻り値が格納されているため,
             * free()関数によって解放することができる */
            if (expansion_result != NULL) {
                /* デバッグ情報の表示 */
                if (app_config.is_debug_mode)
                    print_message(__func__,
                        "tilde expansion of redirect output file name: %s --> %s\n",
                        redir_info->output_file_name, expansion_result);

                free(redir_info->output_file_name);
                redir_info->output_file_name = expansion_result;
            }

            /* 環境変数を展開 */
            if (!expand_variable(redir_info->output_file_name, &expansion_result)) {
                print_error(__func__, "expand_variable() failed\n");
                return false;
            }

            /* デバッグ情報の表示 */
            if (app_config.is_debug_mode)
                print_message(__func__,
                    "environment variable expansion of redirect output file name: %s --> %s\n",
                    redir_info->output_file_name, expansion_result);
            
            free(redir_info->output_file_name);
            redir_info->output_file_name = expansion_result;
        }

        for (j = 0; j < shell_cmd->num_simple_commands; ++j) {
            simple_cmd = &shell_cmd->simple_commands[j];

            for (k = 0; k < simple_cmd->num_arguments; ++k) {
                /* コマンドライン引数の展開 */
                /* チルダで開始する文字列を展開 */
                if (!expand_tilde(simple_cmd->arguments[k], &expansion_result)) {
                    print_error(__func__, "expand_tilde() failed\n");
                    return false;
                }

                /* 文字列が置き換わった場合は, 置き換える前の文字列を破棄 */
                /* simple_cmd->argumentsにはstrndup()関数の戻り値が格納されているため,
                 * free()関数によって解放することができる */
                if (expansion_result != NULL) {
                    /* デバッグ情報の表示 */
                    if (app_config.is_debug_mode)
                        print_message(__func__,
                            "tilde expansion of argument: %s --> %s\n",
                            simple_cmd->arguments[k], expansion_result);

                    free(simple_cmd->arguments[k]);
                    simple_cmd->arguments[k] = expansion_result;
                }
                
                /* 環境変数を展開 */
                if (!expand_variable(simple_cmd->arguments[k], &expansion_result)) {
                    print_error(__func__, "expand_variable() failed\n");
                    return false;
                }

                /* デバッグ情報の表示 */
                if (app_config.is_debug_mode)
                    print_message(__func__,
                        "environment variable expansion of argument: %s --> %s\n",
                        simple_cmd->arguments[k], expansion_result);

                free(simple_cmd->arguments[k]);
                simple_cmd->arguments[k] = expansion_result;
            }

            /* ワイルドカードを展開 */
            if (!expand_wildcard(simple_cmd, &new_simple_cmd)) {
                print_error(__func__, "expand_wildcard() failed\n");
                return false;
            }

            /* 展開後のコマンドライン引数と入れ替え */
            free_simple_command(simple_cmd);
            shell_cmd->simple_commands[j] = new_simple_cmd;
        }
    }

    return true;
}

/*
 * チルダで開始する文字列を展開
 */
bool expand_tilde(const char* str, char** result)
{
    char* p = NULL;
    char* home_dir = NULL;
    char* username = NULL;
    char* pwd = NULL;
    size_t username_len;
    struct passwd* pw;

    struct dynamic_string dyn_result;
    
    /* 展開後の文字列へのポインタ */
    *result = NULL;
    
    /* 文字列が空である場合は何もしない */
    /* 文字列の先頭がチルダでない場合は何もしない */
    if (str[0] == '\0' || str[0] != '~')
        return true;

    /* スラッシュの直前までの部分文字列(ユーザ名)の長さを切り出す */
    /* ユーザ名の長さが0である(チルダのみである)場合は環境変数HOMEの値で書き換え */
    username_len = ((p = strchr(str, '/')) != NULL) ?
                   (size_t)(p - str - 1) : (strlen(str) - 1);

    /* 環境変数HOMEの値で置き換える場合 */
    if (username_len == 0) {
        /* 環境変数HOMEの値を取得 */
        if ((home_dir = getenv("HOME")) == NULL) {
            print_error(__func__, "getenv() failed: %s\n", strerror(errno));
            return false;
        }
        
        /* 動的文字列を作成 */
        if (!initialize_dynamic_string(&dyn_result)) {
            print_error(__func__, "initialize_dynamic_string() failed\n");
            return false;
        }

        /* ホームディレクトリと, チルダを除いた元の文字列を順に追加 */
        if (!dynamic_string_append(&dyn_result, home_dir)) {
            print_error(__func__, "dynamic_string_append() failed\n");
            free_dynamic_string(&dyn_result);
            return false;
        }

        if (!dynamic_string_append(&dyn_result, str + 1)) {
            print_error(__func__, "dynamic_string_append() failed\n");
            free_dynamic_string(&dyn_result);
            return false;
        }

        /* 動的文字列の所有権を移動(ムーブ演算) */
        /* 動的文字列が確保したメモリ領域を解放する必要はなし */
        *result = move_dynamic_string(&dyn_result);
        
        return true;
    }
    
    /* カレントディレクトリで置き換える場合 */
    if (!strncmp(str + 1, "+", username_len)) {
        /* 環境変数PWDの値を取得 */
        if ((pwd = getenv("PWD")) == NULL) {
            print_error(__func__, "getenv() failed: %s\n", strerror(errno));
            return false;
        }

        /* 動的文字列を作成 */
        if (!initialize_dynamic_string(&dyn_result)) {
            print_error(__func__, "initialize_dynamic_string() failed\n");
            return false;
        }

        /* カレントディレクトリと, チルダ及びプラス記号を除いた元の文字列を順に追加 */
        if (!dynamic_string_append(&dyn_result, pwd)) {
            print_error(__func__, "dynamic_string_append() failed\n");
            free_dynamic_string(&dyn_result);
            return false;
        }

        if (!dynamic_string_append(&dyn_result, str + 2)) {
            print_error(__func__, "dynamic_string_append() failed\n");
            free_dynamic_string(&dyn_result);
            return false;
        }

        /* 文字列の所有権を移動 */
        *result = move_dynamic_string(&dyn_result);

        return true;
    }

    /* 特定のユーザのホームディレクトリで置き換える場合 */
    if ((username = strndup(str + 1, username_len)) == NULL) {
        print_error(__func__, "strndup() failed: %s\n", strerror(errno));
        return false;
    }

    /* 指定されたユーザが見つからない場合はそのまま返す */
    if ((pw = getpwnam(username)) == NULL) {
        free(username);
        return true;
    }

    /* 動的文字列を作成 */
    if (!initialize_dynamic_string(&dyn_result)) {
        print_error(__func__, "initialize_dynamic_string() failed\n");
        free(username);
        return false;
    }

    /* ユーザのホームディレクトリ, チルダとユーザ名を除いた元の文字列を順に追加 */
    if (!dynamic_string_append(&dyn_result, pw->pw_dir)) {
        print_error(__func__, "dynamic_string_append() failed\n");
        free_dynamic_string(&dyn_result);
        free(username);
        return false;
    }

    if (!dynamic_string_append(&dyn_result, str + username_len + 1)) {
        print_error(__func__, "dynamic_string_append() failed\n");
        free_dynamic_string(&dyn_result);
        free(username);
        return false;
    }
    
    /* 文字列の所有権を移動 */
    *result = move_dynamic_string(&dyn_result);

    return true;
}

/*
 * 環境変数を展開
 */
bool expand_variable(const char* str, char** result)
{
    int i;
    int len;
    int begin;
    int env_name_len;
    char ch;
    char next_ch;
    char* env_name = NULL;
    char* env_val = NULL;

    bool is_env = false;

    struct dynamic_string dyn_result;

    /* 展開後の文字列へのポインタ */
    *result = NULL;

    /* 文字列が空である場合は何もしない */
    if (str[0] == '\0')
        return true;

    /* 動的文字列を作成 */
    if (!initialize_dynamic_string(&dyn_result)) {
        print_error(__func__, "initialize_dynamic_string() failed\n");
        return false;
    }

    len = strlen(str);
    
    /* 文字列の解析 */
    for (i = 0; i < len; ++i) {
        ch = str[i];
        next_ch = (i < len - 1) ? str[i + 1] : '\0';
        
        if (!is_env) {
            /* 環境変数ではない場合 */
            if (ch == '$' && (isalpha(next_ch) || next_ch == '_')) {
                /* 環境変数の開始 */
                /* 環境変数の変数名には, アルファベットまたはアンダースコアで開始し,
                 * 後ろにアルファベット, 数値, アンダースコアのいずれかが0文字以上続くものを想定 */
                is_env = true;
                begin = i + 1;
            } else {
                /* それ以外の文字の場合は動的文字列に追加 */
                if (!dynamic_string_append_char(&dyn_result, ch)) {
                    print_error(__func__, "dynamic_string_append_char() failed\n");
                    free_dynamic_string(&dyn_result);
                    return false;
                }
            }
        } else {
            /* 環境変数の候補 */
            if (i == len - 1) {
                /* 環境変数の名前の長さを取得(文字列の末尾の場合は1文字増える) */
                env_name_len = i - begin + 1;
            } else if (!(isalnum(ch) || ch == '_')) {
                /* 環境変数の名前の長さを取得 */
                env_name_len = i - begin;
                /* 環境変数ではないのでフラグをリセット */
                is_env = false;
                /* 同じ文字(環境変数として不正な文字)を再度処理 */
                --i;
            } else {
                continue;
            }
            
            /* 環境変数の名前を切り出し */
            if ((env_name = strndup(str + begin, env_name_len)) == NULL) {
                print_error(__func__, "strndup() failed: %s\n", strerror(errno));
                free_dynamic_string(&dyn_result);
                return false;
            }

            /* 環境変数の値を取得 */
            if ((env_val = getenv(env_name)) == NULL) {
                /* 指定された名前の環境変数は存在しない */
                /* 動的文字列にドルマークで開始するが環境変数ではない文字列を追加 */
                if (!dynamic_string_append_char(&dyn_result, '$')) {
                    print_error(__func__, "dynamic_string_append_char() failed\n");
                    free_dynamic_string(&dyn_result);
                    free(env_name);
                    return false;
                }

                if (!dynamic_string_append(&dyn_result, env_name)) {
                    print_error(__func__, "dynamic_string_append() failed\n");
                    free_dynamic_string(&dyn_result);
                    free(env_name);
                    return false;
                }
            } else {
                /* 指定された名前の環境変数が見つかった */
                /* 動的文字列に環境変数の値を追加 */
                if (!dynamic_string_append(&dyn_result, env_val)) {
                    print_error(__func__, "dynamic_string_append() failed\n");
                    free_dynamic_string(&dyn_result);
                    free(env_name);
                    return false;
                }
            }
            
            /* 切り出した部分文字列を解放 */
            free(env_name);
        }
    }

    /* 動的文字列の所有権を移動 */
    *result = move_dynamic_string(&dyn_result);
    
    return true;
}

/*
 * ワイルドカードを展開
 * ワイルドカードの展開を一から実装するのは大変であるため,
 * 代わりにGNU C ライブラリ(glibc)のglob()関数を使用している
 */
bool expand_wildcard(
    const struct simple_command* simple_cmd,
    struct simple_command* result)
{
    size_t i;
    size_t j;
    int ret;
    glob_t results;

    /* 展開後の新たなコマンドライン引数 */
    if (!initialize_simple_command(result)) {
        print_error(__func__, "initialize_simple_command() failed\n");
        return false;
    }

    for (i = 0; i < simple_cmd->num_arguments; ++i) {
        ret = glob(simple_cmd->arguments[i], 0, NULL, &results);
        
        if (ret == 0) {
            /* パターンに合致するパスが見つかった場合 */
            /* パス名を展開後のコマンドライン引数に追加 */
            for (j = 0; j < results.gl_pathc; ++j) {
                if (!append_argument(result, results.gl_pathv[j])) {
                    print_error(__func__, "append_argument() failed\n");
                    globfree(&results);
                    return false;
                }
            }
        } else if (ret == GLOB_NOMATCH) {
            /* パターンに合致するパスが見つからなかった場合 */
            /* 元のコマンドライン引数をそのまま展開後の結果に追加 */
            if (!append_argument(result, simple_cmd->arguments[i])) {
                print_error(__func__, "append_argument() failed\n");
                globfree(&results);
                return false;
            }
        } else {
            /* それ以外の場合はエラー */
            print_error(__func__, "glob() failed: %s\n",
                        ret == GLOB_NOSPACE ? "GLOB_NOSPACE" :
                        ret == GLOB_ABORTED ? "GLOB_ABORTED" :
                        strerror(errno));
        }

        globfree(&results);
    }

    return true;
}

/*
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit)
{
    size_t i;
    size_t j;

    int fd_tty = -1;
    int fd_actual_stdin = -1;
    int fd_actual_stdout = -1;
    int fd_in = -1;
    int fd_out = -1;
    int fd_pipe[2];

    int status = -1;
    int exit_status = -1;

    pid_t cpid = -1;
    pid_t ppid = -1;
    pid_t pgid_parent = -1;
    pid_t pgid_old = -1;
    pid_t pgid_child = -1;

    char* path = NULL;

    struct shell_command* shell_cmd = NULL;
    struct redirect_info* redir_info = NULL;
    struct simple_command* simple_cmd = NULL;

    builtin_command builtin_cmd = NULL;

    /* 標準入力と標準出力のファイル記述子を保存 */
    if ((fd_actual_stdin = dup(STDIN_FILENO)) == -1) {
        print_error(__func__, "dup(STDIN_FILENO) failed: %s\n", strerror(errno));
        goto cleanup;
    }

    if ((fd_actual_stdout = dup(STDOUT_FILENO)) == -1) {
        print_error(__func__, "dup(STDOUT_FILENO) failed: %s\n", strerror(errno));
        goto cleanup;
    }

    /* 制御端末のファイル記述子を取得 */
    if ((fd_tty = open("/dev/tty", O_RDWR)) == -1) {
        print_error(__func__, "open(\"/dev/tty\") failed: %s\n", strerror(errno));
        goto cleanup;
    }

    /* 親プロセスのプロセスIDを取得 */
    ppid = getppid();

    /* 親プロセスのプロセスグループIDを取得 */
    if ((pgid_parent = getpgid(ppid)) < 0) {
        print_error(__func__, "getpgid() failed: %s\n", strerror(errno));
        goto cleanup;
    }

    /* 端末のフォアグラウンドプロセスグループを取得 */
    if ((pgid_old = tcgetpgrp(fd_tty)) == (pid_t)-1) {
        print_error(__func__, "tcgetpgrp() failed: %s\n", strerror(errno));
        goto cleanup;
    }

    for (i = 0; i < cmd->num_shell_commands; ++i) {
        shell_cmd = &cmd->shell_commands[i];
        redir_info = &shell_cmd->redir_info;

        /* 標準入力のリダイレクト処理 */
        if (redir_info->input_file_name != NULL) {
            if ((fd_in = open(redir_info->input_file_name, O_RDONLY)) == -1) {
                print_error(__func__, "could not open \'%s\': %s\n",
                    redir_info->input_file_name, strerror(errno));
                goto cleanup;
            }
        } else {
            /* リダイレクト指定がない場合は, 標準入力のファイル記述子を複製 */
            if ((fd_in = dup(fd_actual_stdin)) == -1) {
                print_error(__func__, "dup() failed: %s\n", strerror(errno));
                goto cleanup;
            }
        }
        
        for (j = 0; j < shell_cmd->num_simple_commands; ++j) {
            simple_cmd = &shell_cmd->simple_commands[j];
            
            /* 標準入力のファイル記述子の処理 */
            if (dup2_close(fd_in, STDIN_FILENO) < 0)
                goto cleanup;

            if (j == shell_cmd->num_simple_commands - 1) {
                /* 標準出力のリダイレクト処理 */
                if (redir_info->output_file_name != NULL) {
                    if (redir_info->append_output) {
                        /* ファイルを追加モードでオープン */
                        fd_out = open(redir_info->output_file_name,
                                      O_WRONLY | O_CREAT | O_APPEND, 0666);
                    } else {
                        /* ファイルを長さ0に切り詰めてオープン */
                        fd_out = open(redir_info->output_file_name,
                                      O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    }

                    if (fd_out == -1) {
                        print_error(__func__, "could not open \'%s\': %s\n",
                            redir_info->output_file_name, strerror(errno));
                        goto cleanup;
                    }
                } else {
                    /* リダイレクト指定がない場合は, 標準出力のファイル記述子を複製 */
                    if ((fd_out = dup(fd_actual_stdout)) == -1) {
                        print_error(__func__, "dup() failed: %s\n", strerror(errno));
                        goto cleanup;
                    }
                }
            } else {
                /* パイプの作成 */
                if (pipe(fd_pipe) == -1) {
                    print_error(__func__, "pipe() failed: %s\n", strerror(errno));
                    goto cleanup;
                }
                
                fd_in = fd_pipe[PIPE_READ];
                fd_out = fd_pipe[PIPE_WRITE];
            }

            /* 標準出力のファイル記述子の処理 */
            if (dup2_close(fd_out, STDOUT_FILENO) < 0)
                goto cleanup;

            /* コマンドの絶対パスを取得 */
            if ((path = search_path(simple_cmd->arguments[0])) == NULL) {
                /* ビルトインコマンドを探索 */
                if ((builtin_cmd = search_builtin_command(simple_cmd->arguments[0])) == NULL) {
                    print_error(__func__,
                        "search_path() failed: No such file or directory: '%s'\n",
                        simple_cmd->arguments[0]);
                    continue;
                }
                
                if (shell_cmd->exec_mode != EXECUTE_IN_BACKGROUND) {
                    /* ビルトインコマンドを呼び出し */
                    (*builtin_cmd)(simple_cmd->num_arguments,
                                   simple_cmd->arguments,
                                   is_exit);

                    if (is_exit)
                        goto cleanup;
                    else
                        continue;
                }
            }

            /* プロセスをフォーク */
            cpid = fork();

            if (cpid < 0) {
                print_error(__func__, "fork() failed: %s\n", strerror(errno));
                goto cleanup;
            }
            
            if (cpid == 0) {
                /* 子プロセスの処理 */
                /* ビルトインコマンドをバックグラウンド実行 */
                if (shell_cmd->exec_mode == EXECUTE_IN_BACKGROUND &&
                    builtin_cmd != NULL) {
                    (*builtin_cmd)(simple_cmd->num_arguments,
                                   simple_cmd->arguments,
                                   is_exit);
                    exit(EXIT_SUCCESS);
                }
                
                if (app_config.is_debug_mode)
                    print_message(__func__, "search_path(): %s\n", path);

                /* 出力バッファの内容が破棄される前に表示 */
                fflush(NULL);

                /* シグナルハンドラの設定を元に戻す */
                reset_signal_handlers();

                /* コマンドを実行 */
                execve(path, simple_cmd->arguments, environ);
                print_error(__func__, "execvp() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            } else {
                /* 親プロセスの処理 */
                if (shell_cmd->exec_mode != EXECUTE_IN_BACKGROUND) {
                    /* 子プロセスのプロセスグループIDを設定 */
                    pgid_child = cpid;
                } else {
                    /* 最初に作成されたプロセスをプロセスグループリーダに設定 */
                    pgid_child = (j == 0) ? cpid : pgid_child;
                }

                /* 子プロセスのプロセスグループIDを設定 */
                if (setpgid(cpid, pgid_child) == -1) {
                    print_error(__func__, "setpgid() failed: %s\n", strerror(errno));
                    goto cleanup;
                }

                /* 制御端末のフォアグラウンドプロセスグループを設定 */
                if (shell_cmd->exec_mode != EXECUTE_IN_BACKGROUND) {
                    /* 子プロセスのプロセスグループIDを指定 */
                    if (tcsetpgrp(fd_tty, pgid_child) == -1) {
                        print_error(__func__, "tcsetpgrp() failed: %s\n", strerror(errno));
                        goto cleanup;
                    }
                }

                /* 子プロセスを待機 */
                if (shell_cmd->exec_mode != EXECUTE_IN_BACKGROUND)
                    if (wait_child_process(cpid, &status) < 0)
                        goto cleanup;
            }
        }

        /* フォアグラウンドプロセスグループを復元 */
        if (tcsetpgrp(fd_tty, pgid_old) == -1) {
            print_error(__func__, "tcsetpgrp() failed: %s\n", strerror(errno));
            goto cleanup;
        }

        /* 終了状態をみて次のコマンドを実行するかどうかを決定 */
        exit_status = WEXITSTATUS(status);

        if (shell_cmd->exec_mode == EXECUTE_NEXT_WHEN_SUCCEEDED &&
            exit_status != EXIT_SUCCESS)
            break;

        if (shell_cmd->exec_mode == EXECUTE_NEXT_WHEN_FAILED &&
            exit_status == EXIT_SUCCESS)
            break;
    }

cleanup:
    /* フォアグラウンドプロセスグループを復元 */
    if (tcsetpgrp(fd_tty, pgid_old) == -1) {
        print_error(__func__, "tcsetpgrp() failed: %s\n", strerror(errno));
        goto cleanup;
    }

    if (fd_actual_stdin != -1)
        dup2_close(fd_actual_stdin, STDIN_FILENO);

    if (fd_actual_stdout != -1)
        dup2_close(fd_actual_stdout, STDOUT_FILENO);

    if (fd_tty != -1)
        if (close(fd_tty) == -1)
            print_error(__func__, "close() failed\n", strerror(errno));
}

