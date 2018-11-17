
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
 * コマンドの実行方法を表す列挙体を文字列に変換
 */
const char* execution_mode_to_string(enum execution_mode exec_mode)
{
    static const char* const execution_mode_str[] = {
        "EXECUTION_MODE_NONE", "EXECUTE_NEXT_WHEN_SUCCEEDED",
        "EXECUTE_NEXT_WHEN_FAILED", "EXECUTE_SEQUENTIALLY",
        "EXECUTE_IN_BACKGROUND" };

    assert((int)exec_mode >= 0);
    assert((int)exec_mode < sizeof(execution_mode_str) / sizeof(execution_mode_str[0]));

    return execution_mode_str[(int)exec_mode];
}

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

    return true;
}

/*
 * simple_command構造体の内容を出力
 */
void dump_simple_command(FILE* fp, const struct simple_command* simple_cmd)
{
    int i;
    
    fprintf(fp, "simple_command: num_arguments: %d\n",
            simple_cmd->num_arguments);

    for (i = 0; i < simple_cmd->num_arguments; ++i)
        fprintf(fp, "simple_command: argument %d: \'%s\'\n",
                i, simple_cmd->arguments[i]);
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
    shell_cmd->exec_mode = EXECUTION_MODE_NONE;
    /* shell_cmd->job_no = -1; */
    
    /* リダイレクトに関する情報の初期化 */
    shell_cmd->redir_info.input_file_name = NULL;
    shell_cmd->redir_info.output_file_name = NULL;
    shell_cmd->redir_info.append_output = false;

    return true;
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

    shell_cmd->exec_mode = EXECUTION_MODE_NONE;
    /* shell_cmd->job_no = -1; */
}

/*
 * コマンドを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_simple_command(
    struct shell_command* shell_cmd, struct simple_command* simple_cmd)
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
 * shell_command構造体の内容を出力
 */
void dump_shell_command(FILE* fp, struct shell_command* shell_cmd)
{
    int i;

    fprintf(fp, "shell_command: num_simple_commands: %d\n",
            shell_cmd->num_simple_commands);
    fprintf(fp, "shell_command: redir_info: input_file_name: %s\n",
            shell_cmd->redir_info.input_file_name != NULL ?
            shell_cmd->redir_info.input_file_name : "stdin");
    fprintf(fp, "shell_command: redir_info: output_file_name: %s\n",
            shell_cmd->redir_info.output_file_name != NULL ?
            shell_cmd->redir_info.output_file_name : "stdout");
    fprintf(fp, "shell_command: redir_info: append_output: %d\n",
            shell_cmd->redir_info.append_output);
    fprintf(fp, "shell_command: exec_mode: %s\n",
            execution_mode_to_string(shell_cmd->exec_mode));

    for (i = 0; i < shell_cmd->num_simple_commands; ++i)
        dump_simple_command(fp, &shell_cmd->simple_commands[i]);
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
    struct command* cmd, struct shell_command* shell_cmd)
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
 * command構造体の内容を出力
 */
void dump_command(FILE* fp, struct command* cmd)
{
    int i;

    fprintf(fp, "command: num_shell_commands: %d\n",
            cmd->num_shell_commands);

    for (i = 0; i < cmd->num_shell_commands; ++i)
        dump_shell_command(fp, &cmd->shell_commands[i]);
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
    struct token* tok_op;
    struct shell_command shell_cmd;
    
    assert(tok_stream != NULL);
    assert(cmd != NULL);
    
    /* print_message(__func__, "called\n"); */

    /* コマンドを初期化 */
    if (!initialize_command(cmd)) {
        print_error(__func__, "initialize_command() failed\n");
        return false;
    }

    /* シェルコマンドを初期化 */
    if (!initialize_shell_command(&shell_cmd)) {
        print_error(__func__, "initialize_shell_command() failed\n");
        return false;
    }

    /* 最低1つ以上のシェルコマンドがない場合はエラー */
    if (!parse_shell_command(tok_stream, &shell_cmd)) {
        print_error(__func__, "no command is given\n");
        return false;
    }
    
    while (1) {
        /* トークンがなければ終了 */
        if (token_stream_end_of_stream(tok_stream)) {
            /* シェルコマンドの実行方法を指定 */
            shell_cmd.exec_mode = EXECUTION_MODE_NONE;

            /* シェルコマンドを追加 */
            if (!append_shell_command(cmd, &shell_cmd)) {
                print_error(__func__, "append_shell_command() failed\n");
                return false;
            }

            return true;
        }

        /* トークンストリームからトークンを取得 */
        /* トークンがなければエラー */
        if ((tok_op = token_stream_get_current_token(tok_stream)) == NULL) {
            print_error(__func__, "no more tokens, operator expected\n");
            return false;
        }

        /* トークンが'&&', '||', '&', ';'のいずれかではない場合はエラー */
        if (tok_op->type != TOKEN_TYPE_AND_AND &&
            tok_op->type != TOKEN_TYPE_OR &&
            tok_op->type != TOKEN_TYPE_AND &&
            tok_op->type != TOKEN_TYPE_SEMICOLON) {
            print_error(__func__,
                "invalid token type: %s, type %s, %s, %s, %s expected\n",
                token_type_to_string(tok_op->type),
                token_type_to_string(TOKEN_TYPE_AND_AND),
                token_type_to_string(TOKEN_TYPE_OR),
                token_type_to_string(TOKEN_TYPE_AND),
                token_type_to_string(TOKEN_TYPE_SEMICOLON));
            return false;
        }

        /* トークンの種別に従って実行方法を決定 */
        switch (tok_op->type) {
            case TOKEN_TYPE_AND_AND:
                /* コマンドの正常終了時に次のコマンドを実行 */
                shell_cmd.exec_mode = EXECUTE_NEXT_WHEN_SUCCEEDED;
                break;
            case TOKEN_TYPE_OR:
                /* コマンドの異常終了時に次のコマンドを実行 */
                shell_cmd.exec_mode = EXECUTE_NEXT_WHEN_FAILED;
                break;
            case TOKEN_TYPE_AND:
                /* コマンドをバックグラウンドで実行 */
                shell_cmd.exec_mode = EXECUTE_IN_BACKGROUND;
                break;
            case TOKEN_TYPE_SEMICOLON:
                /* コマンドを前から順番に実行 */
                shell_cmd.exec_mode = EXECUTE_SEQUENTIALLY;
                break;
            default:
                assert(false);
                break;
        }

        /* ここでシェルコマンドを追加 */
        if (!append_shell_command(cmd, &shell_cmd)) {
            print_error(__func__, "append_shell_command() failed\n");
            return false;
        }

        /* トークンを1つ先に進める */
        if (!token_stream_move_next(tok_stream)) {
            /* 末尾がバックグラウンド実行またはセミコロンである場合は返す */
            /* それ以外の場合は後続のシェルコマンドが必要なのでエラー */
            if (tok_op->type == TOKEN_TYPE_AND ||
                tok_op->type == TOKEN_TYPE_SEMICOLON)
                return true;

            print_error(__func__, "no more tokens, command expected\n");
            return false;
        }

        /* シェルコマンドを再度初期化 */
        if (!initialize_shell_command(&shell_cmd)) {
            print_error(__func__, "initialize_shell_command() failed\n");
            return false;
        }

        /* シェルコマンドを解析 */
        if (!parse_shell_command(tok_stream, &shell_cmd)) {
            print_error(__func__, "parse_shell_command() failed\n");
            return false;
        }
    }
    
    return true;
}

/*
 * シェルコマンドの構文解析
 * <ShellCommand> ::= <SimpleCommandList>
 * <SimpleCommandList> ::= <SimpleCommand> ['|' <SimpleCommand>]*
 */
bool parse_shell_command(
    struct token_stream* tok_stream, struct shell_command* shell_cmd)
{
    struct token* tok;
    struct simple_command simple_cmd;

    assert(tok_stream != NULL);
    assert(shell_cmd != NULL);

    /* print_message(__func__, "called\n"); */

    /* コマンドを初期化 */
    if (!initialize_simple_command(&simple_cmd)) {
        print_error(__func__, "initialize_simple_command() failed\n");
        return false;
    }
    
    /* 最低1つ以上のコマンドがない場合はエラー */
    if (!parse_simple_command(tok_stream, &simple_cmd, &shell_cmd->redir_info)) {
        print_error(__func__, "no command is given\n");
        return false;
    }

    /* コマンドを追加 */
    if (!append_simple_command(shell_cmd, &simple_cmd)) {
        print_error(__func__, "append_simple_command() failed\n");
        return false;
    }
    
    while (1) {
        /* トークンがなければ終了 */
        if (token_stream_end_of_stream(tok_stream))
            return true;

        /* トークンストリームからトークンを取得 */
        /* トークンがなければエラー */
        if ((tok = token_stream_get_current_token(tok_stream)) == NULL) {
            print_error(__func__, "no more tokens, pipe expected\n");
            return false;
        }

        /* トークンがパイプでない場合は返す */
        if (tok->type != TOKEN_TYPE_PIPE)
            return true;

        /* トークンを1つ先に進める */
        /* トークンがなければエラー */
        if (!token_stream_move_next(tok_stream)) {
            print_error(__func__, "no more tokens, identifier expected\n");
            return false;
        }

        /* コマンドを再度初期化 */
        if (!initialize_simple_command(&simple_cmd)) {
            print_error(__func__, "initialize_simple_command() failed\n");
            return false;
        }

        /* コマンドを解析 */
        if (!parse_simple_command(tok_stream, &simple_cmd, &shell_cmd->redir_info)) {
            print_error(__func__, "parse_simple_command() failed\n");
            return false;
        }

        /* コマンドを追加 */
        if (!append_simple_command(shell_cmd, &simple_cmd)) {
            print_error(__func__, "append_simple_command() failed\n");
            return false;
        }
    }

    return true;
}

/*
 * 各コマンドの構文解析
 * <SimpleCommand> ::= <Identifier> [<SimpleCommandElement>]*
 * <SimpleCommandElement> ::= <Identifier> | <Redirection>
 */
bool parse_simple_command(
    struct token_stream* tok_stream, struct simple_command* simple_cmd,
    struct redirect_info* redir_info)
{
    struct token* tok;

    assert(tok_stream != NULL);
    assert(simple_cmd != NULL);

    /* print_message(__func__, "called\n"); */

    /* トークンストリームからトークンを取得 */
    /* トークンがなければエラー */
    if ((tok = token_stream_get_current_token(tok_stream)) == NULL) {
        print_error(__func__, "no more tokens, identifier or redirection expected\n");
        return false;
    }

    /* 識別子(実行ファイル名)でなければエラー */
    if (tok->type != TOKEN_TYPE_ARGUMENT) {
        print_error(__func__, "invalid token: %s, identifier expected\n", tok->str);
        return false;
    }

    /* 識別子をコマンド引数として追加 */
    if (!append_argument(simple_cmd, tok->str)) {
        print_error(__func__, "append_argument() failed\n");
        return false;
    }

    /* トークンを1つ読み進める */
    /* トークンが残っていなければ返す */
    if (!token_stream_move_next(tok_stream))
        return true;

    while (1) {
        /* 存在するはずのトークンがなければエラー */
        if ((tok = token_stream_get_current_token(tok_stream)) == NULL) {
            print_error(__func__, "no more tokens, identifier or redirection expected\n");
            return false;
        }

        if (tok->type != TOKEN_TYPE_ARGUMENT) {
            /* トークンが識別子またはリダイレクト記号でなければ返す */
            /* parse_redirect関数はトークンストリームのインデックスを,
             * 次に読むべきトークンの位置まで進めてくれる */
            if (tok->type != TOKEN_TYPE_GREAT &&
                tok->type != TOKEN_TYPE_LESS && 
                tok->type != TOKEN_TYPE_GREAT_GREAT)
                return true;

            if (!parse_redirect(tok_stream, redir_info))
                return false;

            /* トークンが残っていなければ返す */
            if (token_stream_end_of_stream(tok_stream))
                return true;
        } else {
            /* 識別子をコマンド引数として追加 */
            if (!append_argument(simple_cmd, tok->str)) {
                print_error(__func__, "append_argument() failed\n");
                return false;
            }

            /* トークンを1つ読み進める */
            /* トークンが残っていなければ返す */
            if (!token_stream_move_next(tok_stream))
                return true;
        }
    }

    return true;
}

/*
 * リダイレクトの構文解析
 * <Redirection> ::= '>' <Identifier>
 *                 | '<' <Identifier>
 *                 | '>>' <Identifier>
 */
bool parse_redirect(
    struct token_stream* tok_stream, struct redirect_info* redir_info)
{
    struct token* tok_redir_op;
    struct token* tok_file_name;

    assert(tok_stream != NULL);
    assert(redir_info != NULL);

    /* print_message(__func__, "called\n"); */

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
    }

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
        /* 正常に読み取れたところまでトークンストリームのインデックスを復元 */
        token_stream_move_previous(tok_stream);
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
                token_stream_move_previous(tok_stream);
                return false;
            }

            /* 出力先のファイルには追記されない(サイズを0に切り詰める) */
            redir_info->append_output = false;
            break;
        case TOKEN_TYPE_GREAT_GREAT:
            /* 標準出力のリダイレクト(追記) */
            /* 以前の設定内容が残っている場合は消去 */
            if (redir_info->output_file_name != NULL) {
                free(redir_info->output_file_name);
                redir_info->output_file_name = NULL;
            }
            
            /* 出力先のファイル名を指定 */
            redir_info->output_file_name = strdup(tok_file_name->str);

            if (redir_info->output_file_name == NULL) {
                print_error(__func__, "strdup() failed: %s\n", strerror(errno));
                token_stream_move_previous(tok_stream);
                return false;
            }

            /* 出力先のファイルには追記される */
            redir_info->append_output = true;
            break;
        case TOKEN_TYPE_LESS:
            /* 標準入力のリダイレクト */
            /* 以前の設定内容が残っている場合は消去 */
            if (redir_info->input_file_name != NULL) {
                free(redir_info->input_file_name);
                redir_info->input_file_name = NULL;
            }

            /* 入力元のファイル名を指定 */
            redir_info->input_file_name = strdup(tok_file_name->str);

            if (redir_info->input_file_name == NULL) {
                print_error(__func__, "strdup() failed: %s\n", strerror(errno));
                token_stream_move_previous(tok_stream);
                return false;
            }

            break;
        default:
            assert(false);
            break;
    }

    /* トークンを1つ読み進める */
    token_stream_move_next(tok_stream);

    return true;
}

