# 计网课程设计作业 - 协议格式解析
1851746 范千惠 计科
### 完成情况
- client-base : 
  - 已完成
- client-adv  : 
  - 已完成
- server-base : 
  - 未完成
  - 只完成了数据库读写类、主进程与子进程控制模块
- server-adv  : 未完成

### 工具类说明
- common/my_encrpty.cpp
  - 加密解密的公共函数实现
- include/my_encrpty.h
  - 加密解密公共函数的声明
- include/my_socket.h
  - socket相关结构体声明
- include/my_getconf.h
  - 工具类：取配置文件参数（支持不同注释符、分割符）
- include/my_getproc.h
  - 工具类：取proc文件系统中的参数
- include/my_log.h
  - 工具类：写日志文件
- include/my_pack.h
  - 工具类：拆封包基础工具
- include/my_cltpack.h
  - 工具类：基于my_pack.h的用于客户端的拆封包工具
- include/my_svrpack.h
  - 工具类：基于my_pack.h的用于服务端的拆封包工具
- include/my_mysql.h
  - 工具类：用于数据库读写

