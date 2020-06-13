http.h
------

http.h is a single header http(Hyper Text Transfer Protocol) parser written in only C99. 
http.h features easy HTTP request, response, header parse and construct.

files
-----

http.h    - http parser/constructor, bytebuf
example.c - simple webserver only using linux socket library and http.h
    Note) This example webserver is not multi-threaded, which mean it only
          handles one request at a time. Implementing mutli-thread, however, 
          is easy.
