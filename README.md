<meta http-equiv="content-type" content="text/html; charset=UTF-8">
Coffee client
=======
A client to [HTCPCP](https://www.ietf.org/rfc/rfc2324.txt) server

**由于总所周知的原因 强烈建议使用 翻墙服务器来运行!!!**

HTCPCP : Hyper Text Coffee Pot Control Protocol 
被视为是早期的 物联网的尝试

you could:
## Usage:
    
    coffe [action]
    
actions:

-       < omit >        show help
-       brew            要求远程服务器酝酿咖啡
-       get             获取你的咖啡
-       propfind        获取与咖啡有关的元数据 ( 详情: 见 rfc2324 )
-       when            要求咖啡服务器现在停止倒牛奶

## Tink:
**严重注意: 其实现在HTCPCP 最严重的问题是:缺少有效的服务器支援**
**大多数谎称是 HTCPCP 的服务器 实际上都是一个茶杯! (详见 rfc2324)**
请选择服务器时 格外注意
coffe　已经在默认配置中添加了一个实现的较为完好的服务器, 但是貌似收到了大陆的封锁 = =
**强烈建议使用 翻墙服务器来运行!!!**
