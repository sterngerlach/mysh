
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* lexer.h */

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdio.h>

/*
 * 字句解析器の状態を表す列挙型
 */
enum lexer_state {
    LEXER_STATE_NONE,
    LEXER_STATE_TOKEN,
    LEXER_STATE_SINGLE_QUOTED_STRING,
    LEXER_STATE_DOUBLE_QUOTED_STRING,
};

/*
 * トークンの種別を表す列挙型
 */
enum token_type {
    TOKEN_TYPE_UNKNOWN,
    TOKEN_TYPE_GREAT,
    TOKEN_TYPE_LESS,
    TOKEN_TYPE_LESS_AND,
    TOKEN_TYPE_GREAT_GREAT,
    TOKEN_TYPE_GREAT_AND,
    TOKEN_TYPE_PIPE,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_AND,
    TOKEN_TYPE_AND_AND,
    TOKEN_TYPE_OR,
    TOKEN_TYPE_ARGUMENT,
    TOKEN_TYPE_EOL,
};

/*
 * トークンを表す構造体
 */
struct token {
    enum token_type type; /* トークンの種別 */
    char*           str;  /* トークンの文字列 */
};

/*
 * トークンの初期化
 */

/*
 * トークン列を扱う構造体
 */
struct token_stream {
    struct token* tokens;          /* トークンの配列 */
    size_t        num_tokens;      /* トークンの個数 */
    size_t        capacity_tokens; /* トークンの配列の要素数 */
    size_t        current_index;   /* 現在のインデックス */
};

/*
 * トークンの種別を文字列に変換
 */
const char* token_type_to_string(enum token_type type);

/*
 * トークンを作成
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool create_token(
    const char* input, size_t index, size_t len,
    enum token_type type, struct token* new_token);

/*
 * トークンを破棄
 */
void free_token(struct token* tok);

/*
 * トークンの妥当性を検査
 */
bool is_token_valid(const struct token* tok);

/*
 * トークンストリームを初期化
 */
bool initialize_token_stream(struct token_stream* tok_stream);

/*
 * トークンストリームを破棄
 */
void free_token_stream(struct token_stream* tok_stream);

/*
 * トークンストリームにトークンを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool token_stream_append_token(
    struct token_stream* tok_stream, struct token* new_token);

/*
 * トークンストリームにトークンが残っているかどうかを判定
 */
bool token_stream_has_more_tokens(
    struct token_stream* tok_stream);

/*
 * トークンストリームのトークンを最後まで読み取ったかどうかを判定
 */
bool token_stream_end_of_stream(
    struct token_stream* tok_stream);

/*
 * トークンストリームから現在のトークンを取得
 */
struct token* token_stream_get_current_token(
    struct token_stream* tok_stream);

/*
 * トークンストリームのインデックスを指定
 */
bool token_stream_set_current_index(
    struct token_stream* tok_stream, size_t index);

/*
 * トークンストリームを1つ次に進める
 */
bool token_stream_move_next(struct token_stream* tok_stream);

/*
 * トークンストリームを1つ前へ進める
 */
bool token_stream_move_previous(struct token_stream* tok_stream);

/*
 * トークンストリームを指定された回数分だけ後へ進める
 * これ以上インデックスを進めることができない場合はfalse,
 * それ以外の場合はtrueを返す
 */
bool token_stream_move_forward(struct token_stream* tok_stream, size_t times);

/*
 * トークンストリームを指定された回数分だけ前へ進める
 * これ以上インデックスを戻すことができない場合はfalse,
 * それ以外の場合はtrueを返す
 */
bool token_stream_move_back(struct token_stream* tok_stream, size_t times);

/*
 * トークンストリームに含まれるトークンを全て出力する
 */
void dump_token_stream(FILE* fp, const struct token_stream* tok_stream);

/*
 * 文字列をトークン列に変換
 * 変換に成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool get_token_stream(char* input, struct token_stream* tok_stream);

#endif /* LEXER_H */

