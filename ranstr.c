#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 *  @param argc コマンドライン引数の数
 *  @param argv コマンドライン引数
 */
int main(int argc, const char * const *argv)
{
    char buffer[32] = {0};
    int curLen = 0;

    /* コマンドライン引数で文字数が指定されていればそちらを使用する */
    const int n = (argc > 1) ? atoi(argv[1]) : 1000;

    /* 乱数の初期化 */
    srand((unsigned int)time(NULL));

    while(curLen < n) {
        /* 乱数をバッファに書き出す */
        int len = snprintf(buffer, sizeof(buffer), "%d", rand());
        if(len < 0) {
            fputs("output error.\n", stderr);
            return -1;
        }

        /* 乱数が長くなりすぎたら切り詰める */
        if(len + curLen > n) {
            len = n - curLen;
        }

        /* 乱数を出力し、出力した分だけカウント */
        fwrite(buffer, 1, len, stdout);
        curLen += len;
    }
    return 0;
}

