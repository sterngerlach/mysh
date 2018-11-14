
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* parser.c */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "util.h"

#define INITIAL_ARRAY_CAPACITY      2
#define INCREMENT_ARRAY_CAPACITY    2

/*
 * simple_command構造体の操作関数
 */

/*
 * simple_command構造体の初期化
 */
bool initialize_simple_command(struct simple_command* simple_cmd)
{
    assert(simple_cmd != NULL);

    simple_cmd->arguments = NULL;
    simple_cmd->num_arguments = 0;
    simple_cmd->capacity_arguments = 0;

    return true;
}

/*
 * simple_command構造体の破棄
 */
void free_simple_command(struct simple_command* simple_cmd)
{
    int i;

    assert(simple_cmd != NULL);

    if (simple_cmd->arguments != NULL) {
        for (i = 0; i < simple_cmd->num_arguments; ++i) {
            free(simple_cmd->arguments[i]);
            simple_cmd->arguments[i] = NULL;
        }

        free(simple_cmd->arguments);
        simple_cmd->arguments = NULL;
    }

    simple_cmd->num_arguments = 0;
    simple_cmd->capacity_arguments = 0;
}

/*
 * コマンドに渡す引数の追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_argument(struct simple_command* simple_cmd, const char* arg)
{
    int capacity_arguments;
    char** new_arguments;
    char* new_argument;

    assert(simple_cmd != NULL);
    assert(arg != NULL);

    /* 引数の配列が空である場合 */
    if (simple_cmd->arguments == NULL) {
        capacity_arguments = INITIAL_ARRAY_CAPACITY;
        new_arguments = (char**)calloc(capacity_arguments, sizeof(char*));

        if (new_arguments == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        simple_cmd->capacity_arguments = capacity_arguments;
        simple_cmd->arguments = new_arguments;
    }

    /* 必要に応じて適宜メモリを拡張 */
    if (simple_cmd->num_arguments >= simple_cmd->capacity_arguments - 1) {
        capacity_arguments = simple_cmd->capacity_arguments + INCREMENT_ARRAY_CAPACITY;
        new_arguments = (char**)realloc(
            simple_cmd->arguments, sizeof(char*) * capacity_arguments);

        if (new_arguments == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        simple_cmd->capacity_arguments = capacity_arguments;
        simple_cmd->arguments = new_arguments;
    }

    /* 引数の文字列を複製 */
    if ((new_argument = strdup(arg)) == NULL) {
        print_error(__func__, "strdup() failed: %s\n", strerror(errno));
        return false;
    }
    
    /* exec関数群の引数に渡せるように末尾の要素をNULLとする */
    simple_cmd->arguments[simple_cmd->num_arguments] = new_argument;
    simple_cmd->num_arguments++;
    simple_cmd->arguments[simple_cmd->num_arguments] = NULL;
}

/*
 * shell_command構造体の操作関数
 */

/*
 * shell_command構造体の初期化
 */
bool initialize_shell_command(struct shell_command* shell_cmd)
{
    assert(shell_cmd != NULL);

    shell_cmd->simple_commands = NULL;
    shell_cmd->num_simple_commands = 0;
    shell_cmd->capacity_simple_commands = 0;
    shell_cmd->exec_flags = EXECUTION_FLAGS_NONE;
    /* shell_cmd->job_no = -1; */
    
    /* リダイレクトに関する情報の初期化 */
    shell_cmd->redir_info.input_file_name = NULL;
    shell_cmd->redir_info.output_file_name = NULL;
    shell_cmd->redir_info.append_output = false;
}

/*
 * shell_command構造体の破棄
 */
void free_shell_command(struct shell_command* shell_cmd)
{
    int i;

    assert(shell_cmd != NULL);

    if (shell_cmd->simple_commands != NULL) {
        for (i = 0; i < shell_cmd->num_simple_commands; ++i)
            free_simple_command(&shell_cmd->simple_commands[i]);

        free(shell_cmd->simple_commands);
        shell_cmd->simple_commands = NULL;
    }

    shell_cmd->num_simple_commands = 0;
    shell_cmd->capacity_simple_commands = 0;

    free(shell_cmd->redir_info.input_file_name);
    shell_cmd->redir_info.input_file_name = NULL;
    
    free(shell_cmd->redir_info.output_file_name);
    shell_cmd->redir_info.output_file_name = NULL;

    shell_cmd->redir_info.append_output = false;

    shell_cmd->exec_flags = EXECUTION_FLAGS_NONE;
    /* shell_cmd->job_no = -1; */
}

/*
 * コマンドを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_simple_command(struct shell_command* shell_cmd, struct simple_command* simple_cmd)
{
    int capacity_simple_commands;
    struct simple_command* new_simple_commands;

    assert(shell_cmd != NULL);
    assert(simple_cmd != NULL);

    /* コマンドが空である場合 */
    if (shell_cmd->simple_commands == NULL) {
        capacity_simple_commands = INITIAL_ARRAY_CAPACITY;
        new_simple_commands = (struct simple_command*)calloc(
            capacity_simple_commands, sizeof(struct simple_command));

        if (new_simple_commands == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        shell_cmd->capacity_simple_commands = capacity_simple_commands;
        shell_cmd->simple_commands = new_simple_commands;
    }

    /* 必要に応じて適宜メモリサイズを拡張 */
    if (shell_cmd->num_simple_commands >= shell_cmd->capacity_simple_commands) {
        capacity_simple_commands =
            shell_cmd->capacity_simple_commands + INCREMENT_ARRAY_CAPACITY;
        new_simple_commands = (struct simple_command*)realloc(
            shell_cmd->simple_commands,
            sizeof(struct simple_command) * capacity_simple_commands);

        if (new_simple_commands == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        shell_cmd->capacity_simple_commands = capacity_simple_commands;
        shell_cmd->simple_commands = new_simple_commands;
    }

    shell_cmd->simple_commands[shell_cmd->num_simple_commands] = *simple_cmd;
    shell_cmd->num_simple_commands++;

    return true;
}

/*
 * command構造体の操作関数
 */

/*
 * command構造体の初期化
 */
bool initialize_command(struct command* cmd)
{
    assert(cmd != NULL);

    cmd->shell_commands = NULL;
    cmd->num_shell_commands = 0;
    cmd->capacity_shell_commands = 0;

    return true;
}

/*
 * command構造体の破棄
 */
void free_command(struct command* cmd)
{
    int i;

    assert(cmd != NULL);

    if (cmd->shell_commands != NULL) {
        for (i = 0; i < cmd->num_shell_commands; ++i)
            free_shell_command(&cmd->shell_commands[i]);

        free(cmd->shell_commands);
        cmd->shell_commands = NULL;
    }

    cmd->num_shell_commands = 0;
    cmd->capacity_shell_commands = 0;
}

/*
 * 1行のシェルコマンドを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_shell_command(
    struct command* cmd, struct simple_command_list* shell_cmd)
{
    int capacity_shell_commands;
    struct shell_command* new_shell_commands;
    
    /* シェルコマンドが空である場合 */
    if (cmd->shell_commands == NULL) {
        capacity_shell_commands = INITIAL_ARRAY_CAPACITY;
        new_shell_commands = (struct shell_command*)calloc(
            capacity_shell_commands, sizeof(struct shell_command));

        if (new_shell_commands == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        cmd->shell_commands = new_shell_commands;
        cmd->capacity_shell_commands = capacity_shell_commands;
    }

    /* 必要に応じて適宜メモリを拡張 */
    if (cmd->num_shell_commands >= cmd->capacity_shell_commands) {
        capacity_shell_commands = cmd->capacity_shell_commands + INCREMENT_ARRAY_CAPACITY;
        new_shell_commands = (struct shell_command*)realloc(
            cmd->shell_commands, sizeof(struct shell_command) * capacity_shell_commands);

        if (new_shell_commands == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        cmd->shell_commands = new_shell_commands;
        cmd->capacity_shell_commands = capacity_shell_commands;
    }

    cmd->shell_commands[cmd->num_shell_commands] = *shell_cmd;
    cmd->num_shell_commands++;

    return true;
}

/*
 * 構文解析の実装
 */

/*
 * コマンドの構文解析
 * <Command> ::= <ShellCommand> [<ShellCommandOp> <ShellCommand>]* <ShellCommandOp>?
 * <ShellCommandOp> ::= '&&' | '||' | '&' | ';'
 */
bool parse_command(struct token_stream* tok_stream, struct command* cmd)
{
    struct token* tok;
    struct shell_command shell_cmd;
    
    do {
        /* シェルコマンドの解析 */
        if (!parse_shell_command(tok_stream, &shell_cmd))
            return false;
        
        /* TODO */
    } while (token_stream_move_next(tok_stream));

    return true;
}

/*
 * シェルコマンドの構文解析
 * <ShellCommand> ::= <SimpleCommandList>
 * <SimpleCommandList> ::= <SimpleCommand> ['|' <SimpleCommand>]*
 */
bool parse_shell_command(struct token_stream* tok_stream, struct shell_command* shell_cmd)
{
    return true;
}

/*
 * 各コマンドの構文解析
 * <SimpleCommand> ::= <SimpleCommandElement> [<SimpleCommandElement>]*
 * <SimpleCommandElement> ::= <Identifier> | <Redirection>
 */
bool parse_simple_command(struct token_stream* tok_stream, struct simple_command* simple_cmd)
{
    return true;
}

/*
 * リダイレクトの構文解析
 * <Redirection> ::= '>' <Identifier>
 *                 | '<' <Identifier>
 *                 | '>>' <Identifier>
 */
bool parse_redirect(struct token_stream* tok_stream, struct redirect_info* redir_info)
{
    struct token* tok_redir_op;
    struct token* tok_file_name;

    assert(tok_stream != NULL);
    assert(redirect_info != NULL);

    /* トークンストリームからトークンを取得 */
    /* トークンがなければエラー */
    if ((tok_redir_op = token_stream_get_current_token(tok_stream)) == NULL) {
        print_error(__func__, "no more tokens, redirect operator expected\n");
        return false;
    }

    /* トークンがリダイレクト記号でなければエラー */
    if (tok_redir_op->type != TOKEN_TYPE_GREAT &&
        tok_redir_op->type != TOKEN_TYPE_LESS &&
        tok_redir_op->type != TOKEN_TYPE_GREAT_GREAT) {
        print_error(__func__, "invalid token type: %s, redirect operator expected\n",
                    token_type_to_string(tok_redir_op->type));
        return false;

    /* トークンを1つ読み進める */
    /* トークンがなければエラー */
    if (!token_stream_move_next(tok_stream)) {
        print_error(__func__, "no more tokens, identifier (file name) expected\n");
        return false;
    }

    /* トークンストリームからトークンを取得 */
    /* トークンがなければエラー */
    if ((tok_file_name = token_stream_get_current_token(tok_stream)) == NULL) {
        print_error(__func__, "no more tokens, identifier (file name) expected\n");
        return false;
    }

    /* トークンが識別子(ファイル名)でなければエラー */
    if (tok_file_name->type != TOKEN_TYPE_ARGUMENT) {
        print_error(__func__, "invalid token type: %s, identifier (file name) expected\n",
                    token_type_to_string(tok_file_name->type));
        return false;
    }

    switch (tok_redir_op->type) {
        case TOKEN_TYPE_GREAT:
            /* 標準出力のリダイレクト */
            /* コマンド内で最後に指定されたリダイレクトが実際に有効になるので,
             * 複数のリダイレクト指定がある場合は以前の設定を上書き */

            /* 以前の設定内容が残っている場合は消去 */
            if (redir_info->output_file_name != NULL) {
                free(redir_info->output_file_name);
                redir_info->output_file_name = NULL;
            }

            /* 出力先のファイル名を指定 */
            redir_info->output_file_name = strdup(tok_file_name->str);

            if (redir_info->output_file_name == NULL) {
                print_error(__func__, "strdup() failed: %s\n", strerror(errno));
                return false;
            }

            /* 出力先のファイルには追記されない(サイズを0に切り詰める) */
            redir_info->append_output = false;
            break;
        case TOKEN_TYPE_GREAT_GREAT:
            /* 標準出力のリダイレクト(追記) */
            break;
        case TOKEN_TYPE_LESS:
            /* 標準入力のリダイレクト */
            break;

    /* トークンを1つ読み進める */
    token_stream_move_next(tok_stream);

    return true;
}

