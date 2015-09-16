#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

/**
 *  戻り値の型
 */
typedef enum Result {
    /** 成功 */
    R_OK,

    /** 失敗 */
    R_NG,

    /** 見つからなかった */
    R_NOTFOUND,
} Result;

/**
 * 文字配列
 */
typedef struct Array {
    /** 配列の始点 */
    void *p;

    /** 配列の長さ */
    size_t length;
} Array;

Result Array_resize(Array *a, size_t n);
Result Array_free(Array *a);
size_t Array_findFirstBit(Array *a, size_t i);

/** 配列の指定位置のポインタを取得する */
#define Array_pointer(a, i) ((void *)(&((char *)((a)->p))[(i)]))
#define Array_get(a, i) (((char *)((a)->p))[(i)])
#define Array_set(a, i, c) (((char *)((a)->p))[(i)]=(c))

/** ハッシュテーブルの要素の型  */
typedef struct HashEntry {
    /** 次の要素 */
    struct HashEntry *next;

    /** 文字列の始点 */
    const char *p;

    /** 文字列の長さ */
    size_t length;
} HashEntry;

/** ハッシュテーブル要素用のキャッシュ*/
typedef struct HashEntryCache {
    /** 空き要素のリスト */
    HashEntry *freeList;

    /** 拡張サイズ */
    size_t extendSize;
} HashEntryCache;

/** ハッシュテーブル */
typedef struct Hash {
    /** 要素のルート配列 */
    HashEntry **entries;

    /** ルート配列のサイズ */
    size_t factor;

    /** ハッシュテーブルのサイズ */
    size_t length;

    /** キャッシュ */
    HashEntryCache *cache;
} Hash;

Result HashEntryCache_initialize(HashEntryCache *cache, size_t exsize);
Result HashEntryCache_extend(HashEntryCache *cache);
Result HashEntryCache_allocate(HashEntryCache *cache, HashEntry **dest);
Result HashEntryCache_deallocate(HashEntryCache *cache, HashEntry *e);

Result Hash_initialize(Hash *hash, size_t factor, HashEntryCache *cache);
Result Hash_putIfAbsent(Hash *hash, const char *p, size_t length, const HashEntry **e);
size_t Hash_hashFunction(const char *p, size_t length);
Result Hash_clear(Hash *hash);
Result Hash_free(Hash *hash);

Result readSource(Array *dest, FILE *file);
Result scanSameSubstrings(const Array *source, Array *curBits, const Array *oldBits, size_t len, Hash *hash);

/**
 *  @param argc コマンドライン引数の数
 *  @param argv コマンドライン引数
 */
int main(int argc, const char * const *argv)
{
    Array source = {0};
    Array bitArray1 = {0};
    Array bitArray2 = {0};
    Array *curBits = &bitArray1;
    Array *oldBits = &bitArray2;
    HashEntryCache cache = {0};
    Hash hash = {0};
    int result = EXIT_SUCCESS;
    size_t len = 0;
    size_t i = 0;
    size_t j = 0;

    /* 入力ファイルを読み込む */
    if(readSource(&source, stdin) != R_OK) {
        perror("error at readSource");
        result = EXIT_FAILURE;
        goto Lerror;
    }

    /* ハッシュを初期化する */
    if(HashEntryCache_initialize(&cache, 65536) != R_OK) {
        perror("error at HashEntryCache_initialize");
        goto Lerror;
    }
    if(Hash_initialize(&hash, 65536, &cache) != R_OK) {
        perror("error at Hash_initialize");
        goto Lerror;
    }

    /* ビット配列を確保する */
    if(Array_resize(&bitArray1, source.length) != R_OK) {
        perror("error at Array_resize(bitArray1)");
        result = EXIT_FAILURE;
        goto Lerror;
    }

    if(Array_resize(&bitArray2, bitArray1.length) != R_OK) {
        perror("error at Array_resize(bitArray1)");
        result = EXIT_FAILURE;
        goto Lerror;
    }

    /* 最初は全文字位置をチェックするので、以前のビット列は全て1で初期化 */
    memset(oldBits->p, 0x1, oldBits->length);

    /* 長さが入力全体-1までの部分文字列全てについて、重複が見つからなくなるまでループ */
    /* ループ終了時点で、oldBitsには最長の重複部分文字列が存在する位置が格納されている */
    for(len = 1; len < source.length; ++len) {
        memset(curBits->p, 0x00, curBits->length);
        if(scanSameSubstrings(&source, curBits, oldBits, len, &hash) == R_NOTFOUND) {
            --len;
            break;
        }

        /* ビット配列を入れ替え */
        if(curBits == &bitArray1) {
            curBits = &bitArray2;
            oldBits = &bitArray1;
        } else {
            curBits = &bitArray1;
            oldBits = &bitArray2;
        }
    }

    /* 最初の重複位置を探す */
    i = Array_findFirstBit(oldBits, 0);

    /* 最初の重複位置の部分文字列と同じ部分文字列を探す */
    for(j = i + 1; j < oldBits->length; ++j) {
        j = Array_findFirstBit(oldBits, j);
        if(memcmp(Array_pointer(&source, i), Array_pointer(&source, j), len) == 0) {
            const char * const cp = (const char *)source.p;
            printf("length: %ld, %.*s[%ld] == %.*s[%ld]\n", len, (int)len, &cp[i], i, (int)len, &cp[j], j);
        }
    }

Lerror:
    Array_free(&bitArray2);
    Array_free(&bitArray1);
    Array_free(&source);
    return result;
}

/**
 * 入力された文字列を全て読み込む
 *
 * @param dest 読込先配列。読込後は入力文字列のサイズになる。
 * @param file 読込元ファイル
 * @return 成功時はR_OK
 */
Result readSource(Array *dest, FILE *file)
{
    char buffer[256] = {0};
    size_t i = 0;

    for(;;) {
        const size_t len = fread(buffer, 1, sizeof(buffer), file);

        /* 配列サイズが足りなければ拡張する */
        if(dest->length - i < len) {
            if(Array_resize(dest, (dest->length + len) * 2) != R_OK) {
                return R_NG;
            }
        }

        /* 読み込んだ内容をコピー */
        memcpy(Array_pointer(dest, i), buffer, len);
        i += len;

        /* 読み込む文字が残っていなければ終了 */
        if(len < sizeof(buffer)) {
            break;
        }
    }

    /* 配列サイズを実際に読んだサイズに合わせて終了 */
    return Array_resize(dest, i);
}

/**
 * 指定サイズの部分文字列で、2つ以上存在するものの位置をビット配列にセットする
 *
 * @param source 文字列全体
 * @param dest 出力先ビット列
 * @param oldBits 前回検索時のビット列
 * @param len 部分文字列の長さ
 * @param hash ハッシュテーブル
 *
 * @return 一致する部分文字列があればR_OK。なければR_NOTFOUND
 */
Result scanSameSubstrings(const Array *source, Array *dest, const Array *oldBits, size_t len, Hash *hash)
{
    size_t slen = source->length - len + 1;
    size_t i = 0;
    size_t j = 0;
    int found = 0;
    Result result = R_NG;

    Hash_clear(hash);

    /* 指定サイズの全部分文字列について、ハッシュを使用して重複しているものを洗い出す */
    for(i = 0; i < (slen - 1); ++i) {
        const char *p;
        const HashEntry *before = NULL;

        /* 前回検査時の重複文字列の位置でなければスキップ */
        if(!Array_get(oldBits, i)) {
            continue;
        }

        /* 現在位置の部分文字列が重複しているか検査する。未登録であればハッシュに登録する */
        p = Array_pointer(source, i);
        if(Hash_putIfAbsent(hash, p, len, &before) != R_OK) {
            return R_NG;
        }

        /* 重複している部分文字列だった場合、以前と今回の出現箇所のビットを立てる */
        if(before) {
            Array_set(dest, before->p - (const char *)source->p, 1);
            Array_set(dest, i, 1);
            found = 1;
        }
    }

    return found ? R_OK : R_NOTFOUND;
}

/**
 * 配列の長さを変える
 * 
 * @param a 長さを変える配列
 * @param n 新しい配列の長さ
 *
 * @return 成功時はR_OK
 */
Result Array_resize(Array *a, size_t n)
{
    void *p = NULL;
    size_t oldLen = 0;

    if(!a) {
        return R_NG;
    }

    oldLen = a->length;
    p = realloc(a->p, n);
    if(!p) {
        return R_NG;
    }

    a->p = p;
    a->length = n;

    /* 拡張した分を初期化 */
    if(oldLen < a->length) {
        memset(Array_pointer(a, oldLen), 0, a->length - oldLen);
    }

    return R_OK;
}

/**
 * 配列を解放する
 *
 * @param a 解放する配列
 *
 * @return 成功時はR_OK
 */
Result Array_free(Array *a)
{
    if(!a) {
        return R_NG;
    }
    free(a->p);
    return R_OK;
}

/**
 * 配列の指定位置から見て先頭に立っているビットの位置を返す
 *
 * @param a 立っているビットを探す配列
 * @param begin ビット探索の開始位置
 *
 * @return ビットの立っている位置。見つからなければa->length
 */
size_t Array_findFirstBit(Array *a, size_t begin)
{
    size_t i;
    if(!a) {
        return 0;
    }

    for(i = begin; i < a->length; ++i) {
        if(Array_get(a, i)) {
            return i;
        }
    }
    return a->length;
}

/**
 * キャッシュの初期化
 * 
 * @param cache 初期化対象のキャッシュ
 * @param exsize 拡張サイズ
 */
Result HashEntryCache_initialize(HashEntryCache *cache, size_t exsize)
{
    if(exsize == 0) {
        return R_NG;
    }

    cache->freeList = NULL;
    cache->extendSize = exsize;
    return HashEntryCache_extend(cache);
}

/**
 * キャッシュを拡張する
 * 
 * @param cache 拡張対象のキャッシュ
 *
 * @return 成功時はR_OK
 */
Result HashEntryCache_extend(HashEntryCache *cache)
{
    size_t i;

    if(!cache || cache->freeList != NULL) {
        return R_NG;
    }

    /* フリーリストの確保  */
    cache->freeList = malloc(sizeof(HashEntry) * cache->extendSize);
    if(!cache->freeList) {
        return R_NG;
    }

    /* 確保した全要素を、次の要素を指すよう初期化する。(最後の要素はNULLを指す) */
    for(i = 0; i < cache->extendSize; ++i) {
        memset(&cache->freeList[i], 0, sizeof(HashEntry));
        cache->freeList[i].next = &cache->freeList[i + 1];
    }
    cache->freeList[cache->extendSize - 1].next = NULL;

    return R_OK;
}

/**
 * 要素をキャッシュから確保する
 * 
 * @param cache キャッシュ
 * @param dest 確保したキャッシュへのポインタの格納先
 *
 * @return 成功時はR_OK
 */
Result HashEntryCache_allocate(HashEntryCache *cache, HashEntry **dest)
{
    if(!cache || !dest) {
        return R_NG;
    }

    if(!cache->freeList) {
        if(HashEntryCache_extend(cache) != R_OK) {
            return R_NG;
        }
    }

    *dest = cache->freeList;
    cache->freeList = cache->freeList->next;
    memset(*dest, 0, sizeof(HashEntry));

    return R_OK;
}

/**
 * 要素をキャッシュに戻す
 * 
 * @param cache キャッシュ
 * @param e 戻す要素
 *
 * @return 成功時はR_OK
 */
Result HashEntryCache_deallocate(HashEntryCache *cache, HashEntry *e)
{
    if(!cache || !e) {
        return R_NG;
    }

    e->next = cache->freeList;
    cache->freeList = e;

    return R_OK;
}

/**
 * ハッシュテーブルの初期化
 *
 * @param hash 初期化対象のハッシュ
 * @param factor ルート配列サイズ
 * @param cache キャッシュ
 *
 * @return 初期化に成功すればR_OK
 */
Result Hash_initialize(Hash *hash, size_t factor, HashEntryCache *cache)
{
    const size_t rootLen = factor * sizeof(HashEntry*);

    if(!hash) {
        return R_NG;
    }

    hash->entries = (HashEntry**) malloc(rootLen);
    if(!hash->entries) {
        return R_NG;
    }

    memset(hash->entries, 0, rootLen);
    hash->factor = factor;
    hash->length = 0;
    hash->cache = cache;

    return R_OK;
}

/**
 * 文字列がハッシュに未登録であれば登録する
 *
 * @param hash 登録先のハッシュ
 * @param p 文字列の始点。文字列は実行中変化しないことが前提となっているのに注意
 * @param length 文字列の長さ
 * @param e 文字列が登録済みの場合は、登録済みエントリへのアドレスがセットされる
 *
 * @return 成功時はR_OK
 */
Result Hash_putIfAbsent(Hash *hash, const char *p, size_t length, const HashEntry **e)
{
    size_t h;
    HashEntry *entry;
    HashEntry **dest;

    if(!hash || !hash->entries || hash->factor == 0 || !p) {
        return R_NG;
    }

    /* ハッシュ値の計算 */
    h = Hash_hashFunction(p, length) % hash->factor;

    /* 同値エントリーを検索する */
    entry = hash->entries[h];
    dest = &hash->entries[h];
    for(; entry; dest = &entry->next, entry = entry->next) {
        /* 同値エントリーが見つかったら中断 */
        if(entry->length == length && memcmp(entry->p, p, length) == 0) {
            break;
        }
    }

    if(!entry) {
        if(!dest) {
            return R_NG; /* ハッシュテーブルの整合性が崩れている */
        }

        /* 同値エントリーがなければ新たに追加 */
        if(HashEntryCache_allocate(hash->cache, dest) != R_OK) {
            return R_NG;
        }

        (*dest)->next = NULL;
        (*dest)->p = p;
        (*dest)->length = length;
        ++hash->length;
    } else if(e) {
        /* 同値エントリーが見つかれば引数を通して返す */
        *e = entry;
    }

    return R_OK;
}

/**
 * 文字列のハッシュ値を計算する
 *
 * @param p 文字列の始点
 * @param length 文字列の長さ
 *
 * @return 文字列のハッシュ値
 */
size_t Hash_hashFunction(const char *p, size_t length)
{
    size_t i;
    size_t result = 0;
    for(i = 0; i < length; ++i) {
        result = result * 31 + p[i];
    }
    return result;
}

/**
 * ハッシュテーブルの全要素を解放する
 *
 * @param hash 要素を解放するハッシュテーブル。解放後も使用可能
 *
 * @return 成功時はR_OK
 */
Result Hash_clear(Hash *hash)
{
    size_t i;

    if(!hash) {
        return R_NG;
    }

    /* 全エントリの破棄  */
    for(i = 0; i < hash->factor; ++i) {
        HashEntry *e;
        for(e = hash->entries[i]; e;) {
            HashEntry * const n = e->next;
            HashEntryCache_deallocate(hash->cache, e);
            e = n;
        }
        hash->entries[i] = NULL;
    }

    return R_OK;
}


/**
 * ハッシュテーブルを解放する
 *
 * @param hash 解放するハッシュテーブル。解放後は使用不能になる。
 */
Result Hash_free(Hash *hash)
{
    size_t i;

    if(!hash) {
        return R_NG;
    }

    /* 全エントリの破棄  */
    if(Hash_clear(hash) != R_OK) {
        return R_OK;
    }

    /* ルート配列の破棄 */
    free(hash->entries);
    hash->entries = NULL;
    hash->factor = 0;
    hash->length = 0;

    return R_OK;
}

