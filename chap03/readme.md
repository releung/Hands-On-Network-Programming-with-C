tcp_client.c                接收终端输入信息，发送给服务器，接收服务器发过来的信息，并打印  
tcp_serve_toupper.c         将客户端发送过来的字符转换成大写，并返回大写字符给客户端. 使用 select 并发处理.  
tcp_serve_toupper_fork.c    功能和 tcp_serve_toupper 一样. 有新的连接就 fork 进程. 取代 select. 不建议这样, 推荐使用 select   
tcp_serve_chat.c            将 tcp_serve_toupper.c 修改成: 让 clients 可以相互发消息的 chat room  

Note:  
    这里都没有处理 send() 只发送了一部分消息的情况.  
    因为可能因为缓存满了，导致 send() 只能发送部分的数据出去，需要处理再次发送剩余的数据情况.  

    还有 recv() 并不是每次都一次性接收完所有数据。  
    例如: 可能发送 '123456' 给客户读, recv() 可能接收了 '1234', 剩下的 '56' 还没有接收. 还需要处理该情况.  
