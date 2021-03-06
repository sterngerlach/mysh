
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* parser.h */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdio.h>

#include "lexer.h"

/*
 * コマンドの文法
 * 以下の各文法要素を解析する関数が実装されている
 */

/*
 * <Command> ::= <ShellCommand> [<ShellCommandOp> <ShellCommand>]* <ShellCommandOp>?
 * <ShellCommand> ::= <SimpleCommandList>
 * <ShellCommandOp> ::= '&&' | '||' | '&' | ';'
 * <SimpleCommandList> ::= <SimpleCommand> ['|' <SimpleCommand>]*
 * <SimpleCommand> ::= <Identifier> [<SimpleCommandElement>]*
 * <SimpleCommandElement> ::= <Identifier> | <Redirection>
 * <Redirection> ::= '>' <Identifier>
 *                 | '<' <Identifier>
 *                 | '>>' <Identifier>
 */

/*
 * 各コマンドの実行方法を指定するための列挙体
 */
enum execution_mode {
    /* 指定なし */
    EXECUTION_MODE_NONE,

    /* 現在のコマンドの正常終了時に次のコマンドを実行(&&) */
    EXECUTE_NEXT_WHEN_SUCCEEDED,

    /* 現在のコマンドの異常終了時に次のコマンドを実行(||) */
    EXECUTE_NEXT_WHEN_FAILED, 

    /* コマンドを前から順番に実行(;) */
    EXECUTE_SEQUENTIALLY,

    /* コマンドをバックグラウンドで実行(&) */
    EXECUTE_IN_BACKGROUND,
};

/*
 * 各コマンド(実行ファイル名と引数)を表す構造体
 */
struct simple_command {
    char** arguments;           /* コマンドの引数 */
    size_t num_arguments;       /* コマンドの引数の個数 */
    size_t capacity_arguments;  /* 引数の配列の最大要素数 */
};

/*
 * リダイレクトの情報を保持する構造体
 */
struct redirect_info {
    char* input_file_name;      /* 入力元のファイル名 */
    char* output_file_name;     /* 出力先のファイル名 */
    bool  append_output;        /* 出力先のファイルに追記するかどうか */
};

/*
 * 1行のシェルコマンドを表す構造体
 */
struct shell_command {
    struct simple_command* simple_commands;          /* コマンドの配列 */
    size_t                 num_simple_commands;      /* コマンドの個数 */
    size_t                 capacity_simple_commands; /* コマンドの配列の最大要素数 */
    struct redirect_info   redir_info;               /* リダイレクトに関する情報 */
    enum execution_mode    exec_mode;                /* 実行方法 */
};

/*
 * コマンド全体を表す構造体
 */
struct command {
    struct shell_command* shell_commands;            /* シェルコマンドの配列 */
    size_t                num_shell_commands;        /* シェルコマンドの個数 */
    size_t                capacity_shell_commands;   /* シェルコマンドの配列の最大要素数 */
};

/*
 * コマンドの実行方法を表す列挙体を文字列に変換
 */
const char* execution_mode_to_string(enum execution_mode exec_mode);

/*
 * simple_command構造体の操作関数
 */

/*
 * simple_command構造体の初期化
 */
bool initialize_simple_command(struct simple_command* simple_cmd);

/*
 * simple_command構造体の破棄
 */
void free_simple_command(struct simple_command* simple_cmd);

/*
 * コマンドに渡す引数の追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_argument(struct simple_command* simple_cmd, const char* arg);

/*
 * simple_command構造体の内容を出力
 */
void dump_simple_command(FILE* fp, const struct simple_command* simple_cmd);

/*
 * shell_command構造体の操作関数
 */

/*
 * shell_command構造体の初期化
 */
bool initialize_shell_command(struct shell_command* shell_cmd);

/*
 * shell_command構造体の破棄
 */
void free_shell_command(struct shell_command* shell_cmd);

/*
 * コマンドを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_simple_command(
    struct shell_command* shell_cmd, struct simple_command* simple_cmd);

/*
 * shell_command構造体の内容を出力
 */
void dump_shell_command(FILE* fp, struct shell_command* shell_cmd);

/*
 * command構造体の操作関数
 */

/*
 * command構造体の初期化
 */
bool initialize_command(struct command* cmd);

/*
 * command構造体の破棄
 */
void free_command(struct command* cmd);

/*
 * 1行のシェルコマンドを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool append_shell_command(
    struct command* cmd, struct shell_command* shell_cmd);

/*
 * command構造体の内容を出力
 */
void dump_command(FILE* fp, struct command* cmd);

/*
 * 構文解析の実装
 */

/*
 * コマンドの構文解析
 * <Command> ::= <ShellCommand> [<ShellCommandOp> <ShellCommand>]* <ShellCommandOp>?
 * <ShellCommandOp> ::= '&&' | '||' | '&' | ';'
 */
bool parse_command(struct token_stream* tok_stream, struct command* cmd);

/*
 * シェルコマンドの構文解析
 * <ShellCommand> ::= <SimpleCommandList>
 * <SimpleCommandList> ::= <SimpleCommand> ['|' <SimpleCommand>]*
 */
bool parse_shell_command(
    struct token_stream* tok_stream, struct shell_command* shell_cmd);

/*
 * 各コマンドの構文解析
 * <SimpleCommand> ::= <Identifier> [<SimpleCommandElement>]*
 * <SimpleCommandElement> ::= <Identifier> | <Redirection>
 */
bool parse_simple_command(
    struct token_stream* tok_stream, struct simple_command* simple_cmd,
    struct redirect_info* redir_info);

/*
 * リダイレクトの構文解析
 * <Redirection> ::= '>' <Identifier>
 *                 | '<' <Identifier>
 *                 | '>>' <Identifier>
 */
bool parse_redirect(
    struct token_stream* tok_stream, struct redirect_info* redir_info);

#endif /* PARSER_H */

