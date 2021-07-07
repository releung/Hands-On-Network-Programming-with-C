lookup.c  
    This program takes a name or IP address for its only argument.  
    It then uses getaddrinfo() to resolve that name or that IP address into an address structure,  
    and the program prints that IP address using getnameinfo() for the text conversion.  
    If multiple addresses are associated with a name, it prints each of them.  
    It also indicates any errors  

dns_query.c  
    Printing a DNS message name  
    根据 dns 协议规范, 打印 dns 字段数据  

    Note:  
        udp 是不可靠的. 这里并没有用 select 处理 timeout 情况, 有可能会一直被阻塞的情况, 用 select 处理就可以了.  

dns_query_select.c  
    将 dns_query.c 修改成 select 监听, 加入 timeout 设置  
