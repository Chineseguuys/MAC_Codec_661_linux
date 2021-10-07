如果要使用 lldb 查看具体的内存当中的数据。可以使用 memory 命令。这个命令的基本格式如下：

```
memory <subcommand> [<subcommand-options>]
```

你可以在 lldb 当中使用 ```help memory 来查看这个命令的具体的用法```

## --format 选项

> --format 用来设定显示的格式，可以使用的格式如下所示。

```
"default"
'B' or "boolean"
'b' or "binary"
'y' or "bytes"
'Y' or "bytes with ASCII"
'c' or "character"
'C' or "printable character"
'F' or "complex float"
's' or "c-string"
'd' or "decimal"
'E' or "enumeration"
'x' or "hex"
'X' or "uppercase hex"
'f' or "float"
'o' or "octal"
'O' or "OSType"
'U' or "unicode16"
"unicode32"
'u' or "unsigned decimal"
'p' or "pointer"
"char[]"
"int8_t[]"
"uint8_t[]"
"int16_t[]"
"uint16_t[]"
"int32_t[]"
"uint32_t[]"
"int64_t[]"
"uint64_t[]"
"float16[]"
"float32[]"
"float64[]"
"uint128_t[]"
'I' or "complex integer"
'a' or "character array"
'A' or "address"
"hex float"
'i' or "instruction"
'v' or "void"
'u' or "unicode8"
```

> 给出一个应用的实例，比如说，我想查看 seektable 当中存储的数据。通过查看 seekTableEmelent 我们已经知道当前的音频文件一共有 158个 frame。seektable 当中存储的都是 uint32 类型的数据。用十进制的方式来打印它们。

```bash 
memory read --size 4 --count 158 --format d 0x000000000021d2c0
```

其他的使用方法可以继续摸索