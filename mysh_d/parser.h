
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* parser.h */

#ifndef PARSER_H
#define PARSER_H

/*
 * コマンドの文法
 * 以下の各文法要素を解析する関数が実装されている
 */

/*
 * <Command> ::= <ShellCommand> [<ShellCommandOp> <ShellCommand>]* <ShellCommandOp>?
 * <ShellCommand> ::= <SimpleCommandList>
 * <ShellCommandOp> ::= '&&' | '||' | '&' | ';'
 * <SimpleCommandList> ::= <SimpleCommand> ['|' <SimpleCommand>]*
 * <SimpleCommand> ::= <SimpleCommandElement> [<SimpleCommandElement>]*
 * <SimpleCommandElement> ::= <Identifier> | <Redirection>
 * <Redirection> ::= '>' <Identifier>
 *                 | '<' <Identifier>
 *                 | '>>' <Identifier>
 */

/*
 * 各コマンドの実行方法を指定するための列挙体
 */
enum execution_flags {
    /* 指定なし */
    EXECUTION_FLAGS_NONE        = 0x0000,

    /* 現在のコマンドの正常終了時に次のコマンドを実行(&&) */
    EXECUTE_NEXT_WHEN_SUCCEEDED = 0x0001,

    /* 現在のコマンドの異常終了時に次のコマンドを実行(||) */
    EXECUTE_NEXT_WHEN_FAILED    = 0x0002, 

    /* コマンドを前から順番に実行(;) */
    EXECUTE_SEQUENTIALLY        = 0x0004,

    /* コマンドをバックグラウンドで実行(&) */
    EXECUTE_IN_BACKGROUND       = 0x0008,
};

/*
 * 各コマンド(実行ファイル名と引数)を表す構造体
 */
struct simple_command {
    char** arguments;           /* コマンドの引数 */
    int    num_arguments;       /* コマンドの引数の個数 */
    int    capacity_arguments;  /* 引数の配列の最大要素数 */
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
    int                    num_simple_commands;      /* コマンドの個数 */
    int                    capacity_simple_commands; /* コマンドの配列の最大要素数 */
    struct redirect_info   redir_info;               /* リダイレクトに関する情報 */
    enum execution_flags   exec_flags;               /* 実行方法 */
    /* int                    job_no; */
};

/*
 * コマンド全体を表す構造体
 */
struct command {
    struct shell_command* shell_commands;            /* シェルコマンドの配列 */
    int                   num_shell_commands;        /* シェルコマンドの個数 */
    int                   capacity_shell_commands;   /* シェルコマンドの配列の最大要素数 */
};

#endif /* PARSER_H */

