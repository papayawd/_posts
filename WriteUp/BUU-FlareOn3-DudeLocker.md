---
title: 'BUU-re [FlareOn3]DudeLocker'
date: 2020-10-23 15:40:21
tags: 十月姐姐温柔要求每天一道BUU-re
---

WINAPI yyds 魔鬼一样的API接口 魔鬼一样的各种函数 还好有师傅的exp

​	<!-- more -->

最终的exp借鉴 n00bzx 师傅

程序的分析都在[idb](https://github.com/papayawd/files_used_in_MyBlog/raw/master/DudeLocker.idb)文件里了，这里说一下程序的大致流程

首先是获取了不知道什么路径，然后不知道有什么判断，不妨我们直接动态调试修改进入else：主要代码部分

![](https://i.loli.net/2020/10/23/9ygWfjqXVNoeOmD.png)

接下来是那个xor函数，结果放在了key_data，因为这里用到了v20，v20=v4，v4来自函数040，040返回得非零，进入查看为0x7DAB1D35，这里动态的时候修改eax(返回值)，然后dump出xor后的值

![](https://i.loli.net/2020/10/23/lR2Q6Xjf89xok1T.png)

然后这个值进行sha1加密后 执行了CryptDeriveKey 设置了后边需要用到的phkey

![](https://i.loli.net/2020/10/23/efkVw3Sojt61gdr.png)

后边又获取了某文件的文件名，估计就是给的文件，然后转为小写(businesspapers.doc)

770函数进行md5加密，并且把结果作为某加密的密匙

500函数就是那个加密函数

![](https://i.loli.net/2020/10/23/7w5yvGXzkdK4thu.png)

如图，有分组，每次分组大小为0x4000，而且CryptGetKeyParam函数的参数8 查询一下

![](https://i.loli.net/2020/10/23/zI5fPsVadDXHTiZ.jpg)

结合分组加密，判断为DES的CBC加密算法，然后给的BusinessPapers.doc文件听该是加密后的文件。

最后释放了一个jpg文件不知道是什么意思。。。图片挺好康的

现在的思路就是先sha1，然后CryptDeriveKey一下phkey，然后md5作为DES CBC的密匙，最后把BusinessPapers.doc文件读出来分组解密，保存到另一个文件里面。

```python
#include <stdio.h>
#include <windef.h>
/*

"businesspapers.doc"
md5加密后 设置为DES CBC的key 然后去解密 BusinessPapers.doc文件
文件路径修改一下别出现中文
C:\abc\BusinessPapers.doc
C:\abc\res.doc
  
*/
#include <windows.h>
#include <wincrypt.h>
int main(int argc, char *argv[])
{
	int i;
	char* filename = "businesspapers.doc";
	char* str = "thosefilesreallytiedthefoldertogether";
	BYTE md5_data[100] = {0};
	BYTE sha1_data[100] = {0};
	DWORD md5_len = 16;
	DWORD sha1_len = 20;
	BYTE pbData[4];
	*(DWORD *)pbData = 1;
	HCRYPTKEY phKey = 0;
	HCRYPTPROV phProv = 0;
	HCRYPTHASH phHash = 0;
	DWORD NumberOfBytesRead=0;
	HANDLE hFile = 0;
	HANDLE  hObject = 0;
	BYTE lpBuffer[0x5000]={0};
	int v18 = 0;
	if (!CryptAcquireContextW(&phProv, 0, 0, 0x18u,0))
	{
		printf("wrong1\n");
		return 0;
	}
	CryptCreateHash(phProv, 0x8004u, 0, 0, &phHash);
	CryptHashData(phHash, (BYTE*)str, 37, 0);
	CryptGetHashParam(phHash, 2u, sha1_data, &sha1_len, 0);
	CryptDeriveKey(phProv,0x6610u, phHash, 1u, &phKey);
	CryptDestroyHash(phHash);
	
	CryptSetKeyParam(phKey, 4u, pbData, 0);
	phHash = 0;
	CryptCreateHash(phProv, 0x8003u, 0, 0, &phHash);
	CryptHashData(phHash, (BYTE*)filename, strlen(filename), 0);
	CryptGetHashParam(phHash, 2u, md5_data, &md5_len, 0);
	for(i = 0;i < 16;i+=2)
	{
		printf("%02x",(DWORD)md5_data[i]);
	}
	printf("\n");
	//CryptDestroyHash(phHash);
	if(!CryptSetKeyParam(phKey, 1u, md5_data, 0))
	{
		printf("wrong2\n");
		return 0;
	}
	hFile = CreateFile("C:\\abc\\BusinessPapers.doc", 0x80000000, 3u, 0, 3u, 0x80u, 0);
	if(hFile == (HANDLE)-1)
	{
		printf("wrong3\n");
		return 0;
	}
	hObject = CreateFile("C:\\abc\\res.doc", 0x40000004u, 3u, 0, 3u, 0x80u, 0);
    if(hObject == (HANDLE)-1)
	{
		printf("wrong4\n");
		return 0;
	}
	while ( ReadFile(hFile, lpBuffer,0x4000, &NumberOfBytesRead, 0) )
    {
		if ( NumberOfBytesRead < 0x4000) { v18 = 1;}
		CryptDecrypt(phKey, 0, v18, 0, (BYTE *)lpBuffer, &NumberOfBytesRead);
		//CryptEncrypt(phKey, 0, v18, 0, (BYTE *)lpBuffer, &NumberOfBytesRead, 0x4010);
		WriteFile(hObject, lpBuffer, NumberOfBytesRead, &NumberOfBytesRead, 0);
		if ( v18 )
		{
			break;
		}
    }
	return 0;
}
/*
cl0se_t3h_f1le_0n_th1s_0ne@flare-on.com
*/
```

文件应该是jpg文件，最终我的res.doc修改一下后缀，打开就有flag

太难搞了，再次感谢n00bzx师傅



