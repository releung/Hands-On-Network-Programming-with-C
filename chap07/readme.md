web_server.c
    仅仅接受 HTTP GET 请求服务

web_server2.c
    改进:
        不用全局变量链表

Note:
    One of the most important rules, when developing networked code,
is that your program should never trust the connected peer. Your code should never assume that the
connected peer sends data in a particular format. This is especially vital for server code that
may communicate with multiple clients at once

    If your code doesn't carefully check for errors and unexpected conditions, then it will be vulnerable to exploits.

    Another issue with server software is that the server wants to allow access to some files on the system, but not others


用于部署于公网的 web 服务, 建议使用开源的服务, 例如: Nginx 或者 Apache
If you must deploy a web server on the internet,
I suggest you consider using a free and open source implementation that's already available.
The web servers Nginx and Apache,
for example, are highly performant, cross-platform, secure, written in C, and completely free. They are also well-documented and easy to find support for

[参考: 关于CGI和FastCGI的理解](https://www.cnblogs.com/tssc/p/10255590.html)  
在网站的整体架构中，Web Server（如nginx，apache）只是内容的分发者，对客户端的请求进行应答。  
如果客户端请求的是 index.html 这类静态页面，那么 Web Server 就去文件系统中找对应的文件，找到返回给客户端（一般是浏览器），在这里 Web Server 分发的就是是静态数据

对于像index.php这类的动态页面请求，Web Server根据配置文件知道这个不是静态文件，则会调用PHP 解析器进行处理然后将返回的数据转发给客户端（浏览器）。

在这个过程中，Web Server并不能直接处理静态或者动态请求，对于静态请求是直接查找然后返回数据或者报错信息，对于动态数据也是交付给其他的工具（这里的PHP解析器）进行处理。


    CGI（Common Gateway Interface）全称是“通用网关接口”，  
是一种让客户端（web浏览器）与Web服务器（nginx等）程序进行通信（数据传输）的协议。

    FastCGI（Fast Common Gateway Interface）全称是“快速通用网关接口”,  
是通用网关接口（CGI）的增强版本，由CGI发展改进而来，主要用来提高CGI程序性能，  
类似于CGI，FastCGI也是一种让交互程序与Web服务器通信的协议

If you want to expose your program to the internet, you can communicate to a web server using either CGI or FastCGI.
With CGI, the web server handles the HTTP request.
When a request comes in, it runs your program and returns your program's output in the HTTP response body


反向代理  
many web servers (such as Nginx or Apache) work as a reverse proxy


For more information about HTTP and HTML, please refer to the following:  

RFC 7230: Hypertext Transfer Protocol (HTTP/1.1):   
    Message Syntax and Routing (https://tools.ietf.org/html/rfc7230)

RFC 7231: Hypertext Transfer Protocol (HTTP/1.1):
    Semantics and Content (https://tools.ietf.org/html/rfc7231) 

Media Types (https://www.iana.org/assignments/media-types/media-types.xhtml)

