---
title: 'BUU-re [Zer0pts2020]QR Puzzle'
date: 2020-10-23 20:03:47
tags: 十月姐姐温柔要求每天一道BUU-re
---

根据题目名 "QR Puzzle" 称推测，应该是一个关于二维码的题目

​	<!-- more -->

我们先看输出，是一个25*25的01串，应该就是被加密之后的二维码

![](https://i.loli.net/2020/10/23/cD3CERMhH6zsuwP.png)

就像提示符所说的，先读入二维码，然后读入key，然后用key给二维码加密，然后储存加密后的二维码

我们看函数AF0，key的读入

![](https://i.loli.net/2020/10/23/iF31vsSB9HUL5rt.png)

判断出是一个链表储存方式，我们再看加密函数

![](https://i.loli.net/2020/10/23/W98ILOZClzv1Sp6.png)

最后的三个操作都是可逆的，类c语言，复制下来，稍微修改即可

链表是从最后往前反序遍历，我们需要正序遍历

以下是解密代码

```c++
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
using namespace std;
char buf_key[625 * 20];
char buf_encrypt[30][300];
int key[700][4];
int work2(int l, int r)
{
    int p = 1, res = 0;
    while (r >= l)
    {
        res += (buf_key[r] - '0') * p;
        p *= 10;
        r--;
    }
    return res;
}
void work(int p, int l, int r)
{
    key[p][3] = buf_key[l] - '0';
    l += 3;
    int k = l;
    while (buf_key[++k] != ',')
        ;
    key[p][1] = work2(l, k - 1);
    l = k + 1;
    key[p][2] = work2(l, r - 1);
    //printf("%d#(%d,%d)\n", key[p][3], key[p][1], key[p][2]);
}
int main()
{
    for (int i = 0; i < 256; i++)
    {
        printf("%c\n", i);
    }

    return 0;
    int fd = open("./key", O_RDWR);
    if (fd == -1)
    {
        printf("open error\n");
        exit(0);
    }
    read(fd, buf_key, 625 * 20);
    int len = strlen(buf_key);
    buf_key[len] = '\n';
    buf_key[len + 1] = '\0';
    //printf("%s", buf_key);
    close(fd);
    int last = 0, num = 0;
    for (int i = 0; i < strlen(buf_key); i++)
    {
        if (buf_key[i] == '\n')
        {
            work(num++, last, i - 1);
            last = i + 1;
        }
        if (i == strlen(buf_key) - 3)
            break;
    }
    // read key    end
    fd = open("./encrypted.qr", O_RDWR);
    if (fd == -1)
    {
        printf("open error\n");
        exit(0);
    }
    for (int i = 0; i < 25; i++)
    {
        read(fd, buf_encrypt[i], 26);
        printf("%s", buf_encrypt[i]);
    }
    printf("\n\n");
    close(fd);
    for (int i = 0; i < 625; i++)
    {
        int key1 = key[i][3];
        int key2 = key[i][1];
        int key3 = key[i][2];
        int p1, p2;
        char *v4;
        char *v5;
        if (key1 == 1)
        {
            p1 = key2 + 1;
            p2 = key3;
            goto LABEL_4;
        }
        if (key1 <= 1)
        {
            if (!key1)
            {
                p1 = key2 - 1;
                p2 = key3;
                goto LABEL_4;
            }
            goto LABEL_11;
        }
        if (key1 != 2)
        {
            p2 = key3 + 1;
            p1 = key2;
            if (key1 == 3)
                goto LABEL_4;
        LABEL_11:
            p2 = key3;
            p1 = key2;
            goto LABEL_4;
        }
        p2 = key3 - 1;
        p1 = key2;
    LABEL_4:

        v4 = buf_encrypt[key3];
        v5 = buf_encrypt[p2];
        *(char *)(v4 + key2) += *(char *)(v5 + p1);
        *(char *)(p1 + v5) = *(char *)(v4 + key2) - *(char *)(p1 + v5);
        *(char *)(key2 + v4) -= *(char *)(v5 + p1);
        /*
        *(char *)(key2 + v4) += *(char *)(v5 + p1);
        *(char *)(p1 + v5) = *(char *)(v4 + key2) - *(char *)(p1 + v5);
        *(char *)(v4 + key2) -= *(char *)(v5 + p1);
        */
    }
    for (int i = 0; i < 25; i++)
    {
        printf("%s", buf_encrypt[i]);
    }
    return 0;
}
```

最后输出的就是原来的二维码01串了

可以单独验证一下（是正确的）  传参数依次是 QR_file key output_file

然后搜一份25*25的01串转二维码的python脚本跑一跑，扫出来得到flag

```python
from PIL import Image
MAX = 25
pic = Image.new("RGB", (MAX, MAX))
str = "1111111001000101001111111100000101100100100100000110111010011100111010111011011101001100010101011101101110101100011100101110110000010011101111010000011111111010101010101111111000000000010110110000000010101010000011100000100100011100010000110101001001100100110001111011101001111101101100011110001010000101001011011100011101011010110010111101010100101110010110110110000001001110101010010001110011110011100110100111001011111100000000000111100001000110011111111001100101101010111100000100101000110001000010111010111010001111110111011101000111101000010000101110101011101101011010110000010010011111110100101111111011110011100011011"
i = 0
for y in range(0, MAX):
    for x in range(0, MAX):
        if(str[i] == '1'):
            pic.putpixel([x, y], (0, 0, 0))
        else:
            pic.putpixel([x, y], (255, 255, 255))
        i = i+1
pic.show()
#pic.save("flag.png")

```



