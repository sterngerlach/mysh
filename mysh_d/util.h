
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* util.h */

#ifndef UTIL_H
#define UTIL_H

#ifndef max
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#endif /* max */

#ifndef min
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#endif /* min */

/*
 * 空白文字かどうかを判定
 * シェルにおける空白文字である場合は0を, そうでない場合は0を返す
 */
int is_blank_char(char c);

/*
 * メタ文字かどうかを判定
 * シェルにおけるメタ文字である場合は1を, そうでない場合は0を返す
 */
int is_meta_char(char c);

/*
* 文字列の末尾の改行文字を除去
*/
void chomp(char* str);

/*
 * 文字列(10進数の整数表現)を整数に変換
 */
bool strict_strtol(const char* nptr, long* valptr);

/*
 * メッセージを標準エラー出力に表示
 */
void print_message(const char* function, const char* format, ...);

/*
 * エラーメッセージを標準エラー出力に表示
 */
void print_error(const char* function, const char* format, ...);

#endif /* UTIL_H */

