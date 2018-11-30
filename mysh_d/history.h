
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* history.h */

#ifndef HISTORY_H
#define HISTORY_H

#include "linked_list.h"

/*
 * 入力されたコマンドの履歴を保持する構造体
 */
struct command_history_entry {
    char* command;
    struct list_entry entry;
};

/*
 * コマンドの履歴のリンクリストを初期化
 */
void initialize_command_history(struct command_history_entry* history);

/*
 * コマンドの履歴に要素を追加
 */
bool append_command_history(struct command_history_entry* history, char* command);

/*
 * コマンドの履歴を更新
 */
bool update_command_history(struct command_history_entry* history, char* command);

/*
 * コマンドの履歴を破棄
 */
void free_command_history(struct command_history_entry* history);

#endif /* HISTORY_H */

