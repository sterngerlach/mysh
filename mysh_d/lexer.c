
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* lexer.c */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "util.h"

#define INITIAL_TOKEN_CAPACITY      2
#define INCREMENT_TOKEN_CAPACITY    2

/*
 * トークンの種別を文字列に変換
 */
const char* token_type_to_string(enum token_type type)
{
    static const char* const token_type_str[] = {
        "Unknown", "Great", "Less", "LessAnd",
        "GreatGreat", "GreatAnd", "Pipe", "Semicolon",
        "And", "AndAnd", "Or", "Argument", "Eol"
    };

    assert((int)type >= 0);
    assert((int)type < sizeof(token_type_str) / sizeof(token_type_str[0]));

    return token_type_str[(int)type];
}

/*
 * token構造体の操作関数
 */

/*
 * トークンを作成
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool create_token(
    const char* input, size_t index, size_t len,
    enum token_type type, struct token* new_token)
{
    char* token_str;

    assert(input != NULL);
    assert(new_token != NULL);
    assert(index < strlen(input));
    assert(index + len <= strlen(input));
    assert(type != TOKEN_TYPE_UNKNOWN);
    
    /* トークンの文字列の長さが0である(特別なトークン)場合 */
    if (len == 0) {
        new_token->type = type;
        new_token->str = NULL;
        return true;
    }
    
    /* トークンの文字列を複製 */
    if ((token_str = strndup(input + index, len)) == NULL) {
        print_error(__func__, "strndup() failed: %s\n", strerror(errno));
        return false;
    }

    /* トークンを作成 */
    new_token->type = type;
    new_token->str = token_str;

    return true;
}

/*
 * トークンを破棄
 */
void free_token(struct token* tok)
{
    if (tok->str != NULL) {
        free(tok->str);
        tok->str = NULL;
    }
}

/*
 * トークンの妥当性を検査
 */
bool is_token_valid(const struct token* tok)
{
    if (tok == NULL)
        return false;

    if (tok->type == TOKEN_TYPE_EOL)
        return (tok->str == NULL);

    return (tok->type != TOKEN_TYPE_UNKNOWN && tok->str != NULL);
}

/*
 * token_stream構造体の操作関数
 */

/*
 * トークンストリームを初期化
 */
bool initialize_token_stream(struct token_stream* tok_stream)
{
    assert(tok_stream != NULL);

    tok_stream->tokens = NULL;
    tok_stream->num_tokens = 0;
    tok_stream->capacity_tokens = 0;
    tok_stream->current_index = 0;

    return true;
}

/*
 * トークンストリームを破棄
 */
void free_token_stream(struct token_stream* tok_stream)
{
    size_t i;

    assert(tok_stream != NULL);

    if (tok_stream->tokens != NULL) {
        for (i = 0; i < tok_stream->num_tokens; ++i)
            free_token(&tok_stream->tokens[i]);

        free(tok_stream->tokens);
        tok_stream->tokens = NULL;
    }

    tok_stream->num_tokens = 0;
    tok_stream->capacity_tokens = 0;
    tok_stream->current_index = 0;
}

/*
 * トークンストリームにトークンを追加
 * 成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool token_stream_append_token(
    struct token_stream* tok_stream, struct token* new_token)
{
    size_t capacity_tokens;
    struct token* new_tokens;

    assert(tok_stream != NULL);
    assert(new_token != NULL);

    /* トークンストリームが空である場合 */
    if (tok_stream->tokens == NULL) {
        capacity_tokens = INITIAL_TOKEN_CAPACITY;
        new_tokens = (struct token*)calloc(
            capacity_tokens, sizeof(struct token));

        if (new_tokens == NULL) {
            print_error(__func__, "calloc() failed: %s\n", strerror(errno));
            return false;
        }

        tok_stream->capacity_tokens = capacity_tokens;
        tok_stream->tokens = new_tokens;
    }
    
    /* 必要に応じて適宜メモリサイズを拡張 */
    if (tok_stream->num_tokens >= tok_stream->capacity_tokens) {
        capacity_tokens = tok_stream->capacity_tokens + INCREMENT_TOKEN_CAPACITY;
        new_tokens = (struct token*)realloc(
            tok_stream->tokens, sizeof(struct token) * capacity_tokens);

        if (new_tokens == NULL) {
            print_error(__func__, "realloc() failed: %s\n", strerror(errno));
            return false;
        }

        tok_stream->capacity_tokens = capacity_tokens;
        tok_stream->tokens = new_tokens;
    }

    tok_stream->tokens[tok_stream->num_tokens] = *new_token;
    tok_stream->num_tokens++;

    return true;
}

/*
 * トークンストリームにトークンが残っているかどうかを判定
 * トークンが現在のインデックスよりも後ろに続いている場合はtrueを返す
 */
bool token_stream_has_more_tokens(
    struct token_stream* tok_stream)
{
    assert(tok_stream != NULL);
    return (tok_stream->current_index < tok_stream->num_tokens - 1);
}

/*
 * トークンストリームのトークンを最後まで読み取ったかどうかを判定
 */
bool token_stream_end_of_stream(
    struct token_stream* tok_stream)
{
    assert(tok_stream != NULL);
    return (tok_stream->current_index >= tok_stream->num_tokens);
}

/*
 * トークンストリームから現在のトークンを取得
 */
struct token* token_stream_get_current_token(
    struct token_stream* tok_stream)
{
    assert(tok_stream != NULL);
    return (tok_stream->num_tokens == 0 ? NULL :
            tok_stream->current_index >= tok_stream->num_tokens ? NULL :
            /* tok_stream->current_index < 0 ? NULL : */
            &tok_stream->tokens[tok_stream->current_index]);
}

/*
 * トークンストリームのインデックスを指定
 */
bool token_stream_set_current_index(
    struct token_stream* tok_stream, size_t index)
{
    assert(tok_stream != NULL);
    tok_stream->current_index = index;
    return true;
}

/*
 * トークンストリームを1つ次に進める
 */
bool token_stream_move_next(struct token_stream* tok_stream)
{
    return token_stream_move_forward(tok_stream, 1);
}

/*
 * トークンストリームを1つ前へ進める
 */
bool token_stream_move_previous(struct token_stream* tok_stream)
{
    return token_stream_move_back(tok_stream, 1);
}

/*
 * トークンストリームを指定された回数分だけ後へ進める
 * これ以上インデックスを進めることができない場合はfalse,
 * それ以外の場合はtrueを返す
 */
bool token_stream_move_forward(struct token_stream* tok_stream, size_t times)
{
    assert(tok_stream != NULL);
    
    if (tok_stream->current_index + times > tok_stream->num_tokens - 1) {
        tok_stream->current_index = tok_stream->num_tokens;
        return false;
    }

    tok_stream->current_index += times;

    return true;
}

/*
 * トークンストリームを指定された回数分だけ前へ進める
 * これ以上インデックスを戻すことができない場合はfalse,
 * それ以外の場合はtrueを返す
 */
bool token_stream_move_back(struct token_stream* tok_stream, size_t times)
{
    assert(tok_stream != NULL);

    if (times > tok_stream->current_index) {
        tok_stream->current_index = 0;
        /* tok_stream->current_index = -1; */
        return false;
    }

    tok_stream->current_index -= times;

    return true;
}

/*
 * トークンストリームに含まれるトークンを全て出力する
 */
void dump_token_stream(FILE* fp, const struct token_stream* tok_stream)
{
    size_t i;
    struct token* tok;

    for (i = 0; i < tok_stream->num_tokens; ++i) {
        tok = &tok_stream->tokens[i];
        fprintf(fp, "token %zu: str: \'%s\', type: %s\n",
                i, tok->str, token_type_to_string(tok->type));
    }
}

/*
 * 字句解析の実装
 */

/*
 * 文字列をトークン列に変換
 * 変換に成功した場合はtrue, 失敗した場合はfalseを返す
 */
bool get_token_stream(char* input, struct token_stream* tok_stream)
{
    int i;
    size_t input_length;
    size_t token_begin_index = 0;

    enum lexer_state current_state = LEXER_STATE_NONE;
    struct token new_token;

    char ch;
    char next_ch;
    /* char prev_ch; */
    
    assert(input != NULL);
    input_length = strlen(input);

    /* トークンストリームの初期化 */
    if (!initialize_token_stream(tok_stream)) {
        free_token_stream(tok_stream);
        return false;
    }

    /* 文字列の解析 */
    for (i = 0; i < input_length; ++i) {
        ch = input[i];
        /* prev_ch = (i > 1) ? input[i - 1] : '\0'; */
        next_ch = (i < input_length - 1) ? input[i + 1] : '\0';

reprocess:
        switch (current_state) {
            case LEXER_STATE_NONE:
                /* 初期状態 */
                if (ch == '"') {
                    /* ダブルクォーテーションで囲まれた文字列 */
                    /* TODO: エスケープシーケンスの処理を追加 */
                    current_state = LEXER_STATE_DOUBLE_QUOTED_STRING;
                    token_begin_index = i + 1;
                } else if (ch == '\'') {
                    /* シングルクォーテーションで囲まれた文字列 */
                    current_state = LEXER_STATE_SINGLE_QUOTED_STRING;
                    token_begin_index = i + 1;
                } else if (ch == '|' && next_ch == '|') {
                    /* 以前のコマンドの異常終了時に実行 */
                    if (!create_token(input, i, 2, TOKEN_TYPE_OR, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                    /* 文字を更に1つ進める */
                    ++i;
                } else if (ch == '|' && next_ch != '|') {
                    /* パイプ */
                    if (!create_token(input, i, 1, TOKEN_TYPE_PIPE, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                } else if (ch == '&' && next_ch == '&') {
                    /* 以前のコマンドの正常終了時に実行 */
                    if (!create_token(input, i, 2, TOKEN_TYPE_AND_AND, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                    /* 文字を更に1つ進める */
                    ++i;
                } else if (ch == '&' && next_ch != '&') {
                    /* バックグラウンド実行 */
                    /* トークンを作成 */
                    if (!create_token(input, i, 1, TOKEN_TYPE_AND, &new_token))
                        goto fail;
                    /* トークンストリームにトークンを追加 */
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                } else if (ch == '<' && next_ch != '<') {
                    /* 標準入力のリダイレクト */
                    if (!create_token(input, i, 1, TOKEN_TYPE_LESS, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                } else if (ch == '>' && next_ch == '&') {
                    /* 出力先を特定のファイルディスクリプタに指定 */
                    if (!create_token(input, i, 2, TOKEN_TYPE_GREAT_AND, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                    /* 文字を更に1つ進める */
                    ++i;
                } else if (ch == '>' && next_ch == '>') {
                    /* 標準出力のリダイレクト(追記) */
                    if (!create_token(input, i, 2, TOKEN_TYPE_GREAT_GREAT, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                    /* 文字を更に1つ進める */
                    ++i;
                } else if (ch == '>' && next_ch != '>') {
                    /* 標準出力のリダイレクト */
                    if (!create_token(input, i, 1, TOKEN_TYPE_GREAT, &new_token))
                        goto fail;
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;
                } else if (!is_meta_char(input[i])) {
                    /* メタ文字で開始しない場合は通常のトークン */
                    current_state = LEXER_STATE_TOKEN;
                    token_begin_index = i;
                    /* もう一度同じ文字を処理 */
                    goto reprocess;
                } else if (is_blank_char(input[i])) {
                    /* 空白文字である場合は無視 */
                }
                break;

            case LEXER_STATE_DOUBLE_QUOTED_STRING:
                /* ダブルクォーテーションで囲まれた文字列 */
                if (ch == '"') {
                    /* トークンの作成 */
                    if (!create_token(input, token_begin_index,
                                      i - token_begin_index - 1,
                                      TOKEN_TYPE_ARGUMENT, &new_token))
                        goto fail;
                    /* トークンストリームにトークンを追加 */
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;

                    current_state = LEXER_STATE_NONE;
                    token_begin_index = i + 1;
                }
                break;

            case LEXER_STATE_SINGLE_QUOTED_STRING:
                /* シングルクォーテーションで囲まれた文字列 */
                if (ch == '\'') {
                    /* トークンの作成 */
                    if (!create_token(input, token_begin_index,
                                      i - token_begin_index - 1,
                                      TOKEN_TYPE_ARGUMENT, &new_token))
                        goto fail;
                    /* トークンストリームにトークンを追加 */
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;

                    current_state = LEXER_STATE_NONE;
                    token_begin_index = i + 1;
                }
                break;

            case LEXER_STATE_TOKEN:
                /* 通常のトークン */
                if (is_meta_char(input[i])) {
                    /* トークンの作成 */
                    if (!create_token(input, token_begin_index, i - token_begin_index,
                                      TOKEN_TYPE_ARGUMENT, &new_token))
                        goto fail;
                    /* トークンストリームにトークンを追加 */
                    if (!token_stream_append_token(tok_stream, &new_token))
                        goto fail;

                    current_state = LEXER_STATE_NONE;
                    token_begin_index = i;
                    /* もう一度同じ文字を処理 */
                    goto reprocess;
                }
                break;
        }
    }

    /* 最後のトークンの処理 */
    switch (current_state) {
        case LEXER_STATE_DOUBLE_QUOTED_STRING:
        case LEXER_STATE_SINGLE_QUOTED_STRING:
            /* 入力の末尾にクォーテーションがない場合はエラーとして扱う */
            /* bashでは追加の入力を待つ */
            goto fail;

        case LEXER_STATE_TOKEN:
            /* トークンの作成 */
            if (!create_token(input, token_begin_index,
                              input_length - token_begin_index,
                              TOKEN_TYPE_ARGUMENT, &new_token))
                goto fail;
            /* トークンストリームにトークンを追加 */
            if (!token_stream_append_token(tok_stream, &new_token))
                goto fail;

            break;
        default:
            break;
    }
    
    return true;

fail:
    /* トークンストリームの破棄 */
    free_token_stream(tok_stream);
    return false;
}

