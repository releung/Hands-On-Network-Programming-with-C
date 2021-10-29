smtp_send.c  
    used to deliver mail directly to the recipient's email provider  

The SMTP protocol we describe in this chapter is unsecured and not encrypted.   
This is convenient for explanation and learning purposes, but in the real world,  
you may want to secure your email transfer


SMTP(Simple Mail Transfer Protocol) 协议. 基于 TCP 协议, 使用　25 端口.  

何时用到　SMTP ?  
    Gmail 给 Gmail 用户发送邮件, 并不需要用到 SMTP;  
    Gmail 给 Yahoo! 用户发送邮件, 需要用到 SMTP;  

Retrieving your email from your mail service provider is a different issue than delivering email between service providers.

Webmail 目前比较流行. 通过浏览器使用　HTTP or HTTPS 通讯.


A typical desktop client connects to a mail provider using either:   
Internet Message Access Protocol (IMAP) or Post Office Protocol (POP) and SMTP.  

For more information about SMTP and email formats, please refer to the following links:  
    RFC 821: Simple Mail Transfer Protocol (https://tools.ietf.org/html/rfc821)  
    RFC 2822: Internet Message Format (https://tools.ietf.org/html/rfc2822)  

