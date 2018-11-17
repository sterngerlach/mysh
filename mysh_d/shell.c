
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* shell.c */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "shell.h"
#include "util.h"

int fd_tty = -1;

/*
 * シグナルSIGCHLDの処理
 */
void sigchld_handler(int sig, siginfo_t* info, void* q)
{
    pid_t pid;
    int status;

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
            break;
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
    sigact.sa_sigaction = sigchld_handler;
    sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGCHLD, &sigact, NULL);

    /* シグナルSIGINTのハンドラを設定 */
    /* sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = 0; */

    /* sigaction(SIGINT, &sigact, NULL); */

    /* シグナルSIGTTOUのハンドラを設定 */
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = 0;

    sigaction(SIGTTOU, &sigact, NULL);
}

/*
 * コマンドを実行
 */
void execute_command(const struct command* cmd, bool* is_exit)
{
    int i;
    int j;

    int fd_actual_stdin = -1;
    int fd_actual_stdout = -1;
    int fd_in = -1;
    int fd_out = -1;
    int fd_pipe[2];

    int status;
    int exit_code;

    pid_t cpid;
    pid_t wpid;
    pid_t ppid;
    pid_t pgid_parent;
    pid_t pgid_old;
    pid_t pgid_child = (pid_t)-1;

    struct shell_command* shell_cmd;
    struct redirect_info* redir_info;
    struct simple_command* simple_cmd;

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
                
                /* 標準出力と標準入力のファイル記述子を設定 */
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
            
            /* プロセスをフォーク */
            cpid = fork();

            if (cpid < 0) {
                print_error(__func__, "fork() failed: %s\n", strerror(errno));
                goto cleanup;
            }
            
            if (cpid == 0) {
                /* 子プロセスの処理 */
                /* コマンドを実行 */
                execvp(simple_cmd->arguments[0], simple_cmd->arguments);
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
                if (shell_cmd->exec_mode == EXECUTE_IN_BACKGROUND) {
                    /* バックグラウンド実行の場合は, 親プロセスのプロセスグループIDを指定 */
                    if (tcsetpgrp(fd_tty, pgid_parent) == -1) {
                        print_error(__func__, "tcsetpgrp() failed: %s\n", strerror(errno));
                        goto cleanup;
                    }
                } else {
                    /* それ以外の場合は, 子プロセスのプロセスグループIDを指定 */
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
                                break;
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
    }

cleanup:
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

