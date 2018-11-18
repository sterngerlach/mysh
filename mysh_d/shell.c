
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* shell.c */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtin.h"
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
    static const char* const option_str = "d";

    /* 有効なオプションの配列 */
    static const struct option long_options[] = {
        { "debug",  no_argument,    NULL, 'd' },
        { NULL,     0,              NULL, 0   }
    };
    
    int opt;
    int opt_index;
    
    /* オプションの取得 */
    while ((opt = getopt_long(argc, argv,
            option_str, long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'd':
                /* シェルをデバッグモードで起動 */
                app_config.is_debug_mode = true;
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
    
    /* シグナルSIGCHLDのハンドラを設定 */
    /* sigact.sa_handler = SIG_DFL;
    sigact.sa_flags = SA_NOCLDWAIT; */

    /* getline関数が途中で割り込まれないようにSA_RESTARTを設定 */
    /* SA_RESTARTを設定していなかったせいで数時間が潰れた */
    sigact.sa_sigaction = sigchld_handler;
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;

    sigaction(SIGCHLD, &sigact, NULL);
    
    /* シグナルSIGTTOUのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = 0;
    
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
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit)
{
    int i;
    int j;

    int fd_tty = -1;
    int fd_actual_stdin = -1;
    int fd_actual_stdout = -1;
    int fd_in = -1;
    int fd_out = -1;
    int fd_pipe[2];

    int status;
    int exit_status;

    pid_t cpid;
    pid_t wpid;
    pid_t ppid;
    pid_t pgid_parent;
    pid_t pgid_old;
    pid_t pgid_child;

    char* path;

    struct shell_command* shell_cmd;
    struct redirect_info* redir_info;
    struct simple_command* simple_cmd;

    builtin_command builtin_cmd;

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
            if (dup2(fd_in, STDIN_FILENO) == -1) {
                print_error(__func__, "dup2() failed: %s\n", strerror(errno));
                close(fd_in);
                goto cleanup;
            }

            if (close(fd_in) == -1) {
                print_error(__func__, "close() failed: %s\n", strerror(errno));
                goto cleanup;
            }

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
            if (dup2(fd_out, STDOUT_FILENO) == -1) {
                print_error(__func__, "dup2() failed: %s\n", strerror(errno));
                close(fd_out);
                goto cleanup;
            }

            if (close(fd_out) == -1) {
                print_error(__func__, "close() failed: %s\n", strerror(errno));
                goto cleanup;
            }

            /* ビルトインコマンドを探索 */
            if ((builtin_cmd = search_builtin_command(simple_cmd->arguments[0])) != NULL) {
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
                
                /* ビルトインコマンドをバックグラウンド実行(用途不明) */
                if (shell_cmd->exec_mode == EXECUTE_IN_BACKGROUND &&
                    builtin_cmd != NULL) {
                    (*builtin_cmd)(simple_cmd->num_arguments,
                                   simple_cmd->arguments,
                                   is_exit);
                    exit(EXIT_SUCCESS);
                }

                /* コマンドの絶対パスを取得 */
                if ((path = search_path(simple_cmd->arguments[0])) == NULL) {
                    print_error(__func__, "search_path() failed: No such file or directory\n");
                    exit(EXIT_FAILURE);
                }

                if (app_config.is_debug_mode)
                    print_message(__func__, "search_path(): %s\n", path);

                /* 出力バッファの内容が破棄される前に表示 */
                fflush(NULL);

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

                if (app_config.is_debug_mode)
                    print_message(__func__, "cpid: %d, pgid_child: %d\n", cpid, pgid_child);

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
                if (shell_cmd->exec_mode != EXECUTE_IN_BACKGROUND) {
                    while ((wpid = waitpid(cpid, &status, 0)) != 0) {
                        if (wpid == -1) {
                            if (errno == EINTR) {
                                continue;
                            } else if (errno == ECHILD) {
                                break;
                            } else {
                                print_error(__func__, "waitpid() failed: %s\n", strerror(errno));
                                goto cleanup;
                            }
                        } else {
                            break;
                        }
                    }
                }
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

    if (fd_actual_stdin != -1) {
        if (dup2(fd_actual_stdin, STDIN_FILENO) == -1)
            print_error(__func__, "dup2() failed\n", strerror(errno));
        if (close(fd_actual_stdin) == -1)
            print_error(__func__, "close() failed\n", strerror(errno));
    }

    if (fd_actual_stdout != -1) {
        if (dup2(fd_actual_stdout, STDOUT_FILENO) == -1)
            print_error(__func__, "dup2() failed\n", strerror(errno));
        if (close(fd_actual_stdout) == -1)
            print_error(__func__, "close() failed\n", strerror(errno));
    }

    if (fd_tty != -1) {
        if (close(fd_tty) == -1)
            print_error(__func__, "close() failed\n", strerror(errno));
    }
}

