# leak_checker

# 使い方

ソースコードに、１つのヘッダーファイルと2つの関数を追加します。

```C:sample.c
#include <stdio.h>
#include <stdlib.h>
#include "leak_checker.h" //追加

int main(int argc, char *argv[])
{
  void *p;

  leak_checker_init(argv[0]);  //追加

  p = malloc(1024);
  p = calloc(2048, 2);
  p = realloc(p, 4096);
  free(p);
  free(p);

  leak_checker_finish();  //追加

  return 0;
}
```

デバック情報付き(-g)でコンパイルします。

```
gcc -g a.c leak_checker.c
```

アプリケーションを実行します。

```
./a.out
```

以下のような結果が得られます。

```
malloc(100) called from /pathto/a.c:10 returns 0x17f22a0
malloc(200) called from /pathto/a.c:11 returns 0x17f2830
realloc(0x17f2830, 300) called from /pathto/a.c:12 returns 0x17f2830
free(0x17f22a0) called from /pathto/a.c:16

===== Memory Leak Summary =====
no free area(addr:0x17f2830, size:300) called from /pathto/a.c:12
```
