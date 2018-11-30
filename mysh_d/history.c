
/* 情報工学科3年 学籍番号61610117 杉浦 圭祐 */
/* history.c */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"
#include "util.h"

/*
 * コマンドの履歴のリンクリストを初期化
 */
void initialize_command_history(struct command_history_entry* history)
{
    assert(history != NULL);

    history->command = NULL;
    initialize_list_head(&history->entry);
}

/*
 * コマンドの履歴に要素を追加
 */
bool append_command_history(struct command_history_entry* history, char* command)
{
    struct command_history_entry* new_entry;

    assert(history != NULL);
    assert(command != NULL);

    if ((new_entry = (struct command_history_entry*)calloc(
        1, sizeof(struct command_history_entry))) == NULL) {
        print_error(__func__, "calloc() failed: %s\n", strerror(errno));
        return false;
    }

    if ((new_entry->command = strdup(command)) == NULL) {
        print_error(__func__, "strdup() failed: %s\n", strerror(errno));
        free(new_entry);
        return false;
    }
    
    /* リンクリストの末尾に挿入 */
    insert_list_entry_tail(&new_entry->entry, &history->entry);

    return true;
}

/*
 * コマンドの履歴を更新
 */
bool update_command_history(struct command_history_entry* history, char* command)
{
    struct command_history_entry* last;

    assert(history != NULL);

    /* コマンドが空であれば何もしない */
    if (command == NULL)
        return true;
    
    if (command[0] == '\n' || command[0] == '\0')
        return true;
    
    /* コマンドの履歴が空であれば追加 */
    if (is_list_empty(&history->entry))
        return append_command_history(history, command);
    
    /* 最後に入力されたコマンドを取得 */
    last = get_last_list_entry(history, struct command_history_entry, entry);
    
    /* 最後に入力されたコマンドと一致する場合は履歴に追加しない */
    if (!strcmp(last->command, command))
        return true;

    /* コマンドの履歴に追加 */
    return append_command_history(history, command);
}

/*
 * コマンドの履歴を破棄
 */
void free_command_history(struct command_history_entry* history)
{
    struct command_history_entry* iter;
    struct command_history_entry* iter_next;

    assert(history != NULL);

    /* リンクリストに含まれる要素を全て削除 */
    list_for_each_entry_safe(iter, iter_next,
        &history->entry, struct command_history_entry, entry) {
        /* リンクリストから要素を削除 */
        remove_list_entry(&iter->entry);
        /* 要素が確保していたメモリ領域を解放 */
        free(iter->command);
        iter->command = NULL;
        free(iter);
        iter = NULL;
    }
}

