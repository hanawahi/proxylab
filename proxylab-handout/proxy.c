  /*
   *  Author: Hana Wahi-Anwar 
   *  Email:  HQW5245@psu.edu
   *
   */

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <errno.h>
  #include "csapp.h"
  #include "cache.h"

  /* You won't lose style points for including this long line in your code */
  static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
  void doit(int fd, CacheList *cache);
  int parse_url(const char *url, char *host, char *port, char *path);


  int main(int argc, char **argv){
    /* Variables: */
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Cache Variables: */
    CacheList cache_store;
    cache_init(&cache_store);



    /* Check command line args */
    if (argc != 2) {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
      exit(1);
    }

    Signal(SIGPIPE, SIG_IGN);
    listenfd = open_listenfd(argv[1]);
    while (1) {
      clientlen = sizeof(clientaddr);
      if ((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) != -1) { 
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
          port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      doit(connfd, &cache_store);  
      close(connfd);
      }
      else {
      printf("Was unable to accept connection from (%s, %s)\n", hostname, port);
      }                                         
      
    }
}

  void doit(int fd, CacheList *cache) {
    /* Buffers: */
    char buf[MAXLINE] = "";
    char method[MAXLINE] = "";
    char uri[MAXLINE] = "";
    char version[MAXLINE] = "";
    /* RIO Variables: */
    rio_t rio;
    rio_t rio_server;
    /* Buffers for parse_url(): */
    char host[MAXLINE] = "";
    char port[MAXLINE] = "";
    char path[MAXLINE] = "";
    /* Buffers for processing headers: */
    char header[MAXLINE] = ""; 
    char body[MAXLINE] = ""; 
    /* Buffer for headers to forward to server: */
    char requestHeaders[MAXLINE] = "";
    /* Flags for if added certain headers: */
        /* Key: 0 = false; 1 = true. */
    int addedHost = 0; 
    /* Server file descriptor: */
    int fd_serv;
    /* Readnb return value: */
    size_t len = 0;

    /* Cache Variables: */
    /* Flags: */
    /* Flag key: 0 = false; 1 = true. */
    /* Flag for if has corrrect server's response status (200 OK): */
    int hasCorrectStatus = 0;
    /* Flag for if header with "Content-Length:" is present: */
    int hasSizeHeader = 0;
    /* Flag determining if storing to cache or not: */
    int doCache = 0;
    
    /* Holds binary object size: */
    int objectSize = 0;
    /* Buffers for response headers and body: */
    char tempHeader[MAXLINE] = "";
    char tempBody[MAXLINE] = "";
    char responseHeaders[MAXLINE] = "";
    /* Holds pointer to memory location of binary file content (for cache): */
    void *binContent = NULL; 
    /* Pointer to cached item: */
    struct CachedItem *item;





    /* Read request line and headers */
    /* Get first header: */
    rio_readinitb(&rio, fd);
    if ( (rio_readlineb(&rio, buf, MAXLINE)) < 1) {
      return;
    }
    
    /* Parse first line into method, uri, and version respectively: */
    sscanf(buf, "%s %s %s", method, uri, version);

    /* If not found in cache, continue: */
    item = find(uri, cache);
    if (item == NULL) {
    

      /* If first line is anything else but GET: */
      if ((strcasecmp(method, "GET")) != 0) {
        /* This is an error. Terminate this connection: */
        return;
      }

        /* Now we can begin reading all request headers: */
        
      /* Parse URI from GET request */
      parse_url(uri, host, port, path);

      /* Fill 1st line in variable (for GET line): */
      sprintf(requestHeaders + strlen(requestHeaders), "GET %s HTTP/1.0\r\n", path);

      /* Open connection: */
        if ((fd_serv = open_clientfd(host, port)) == -1) {
          /*Error: */
          return;
        }

      /* Read request headers & store into requestHeaders: */
      while(rio_readlineb(&rio, buf, MAXLINE) > 2) {
        if (errno == ECONNRESET) {
          /* If error, reset errno, close connection, and return: */
          errno = 0;
          close(fd_serv);
          return;
        }
        /* Get request header type and body of header: */
        sscanf(buf, "%s", header);
        strncpy(body, (buf + strlen(header) +1), (strlen(buf)-(strlen(header))-2));
        body[(strlen(buf)-(strlen(header))-2)] = '\0';
        /* Note: body will NOT contain \r\n. */


        /* Begin adding to requestHeaders: */

        /* If header is Host: */
        if ((strcasecmp(header, "Host:")) == 0) {
          /* Add Host to requestHeaders, and SET FLAG: */
          sprintf((requestHeaders + strlen(requestHeaders)), "%s %s\r\n", header, body);
          addedHost = 1;
        }

        /* Replace w/ our values:
         * User-Agent: | Connection: | Proxy-Connection: */
         else if ((strcasecmp(header, "User-Agent:")) == 0) {
           continue;
         }
         else if ((strcasecmp(header, "Connection:")) == 0) {
           continue;
         }
         else if ((strcasecmp(header, "Proxy-Connection:")) == 0) {
           continue;
         }

        /* SKIP:
         * If-Modified-Since: | If-None-Match: */
         else if ((strcasecmp(header, "If-Modified-Since:")) == 0) {
           continue;
         }
         else if ((strcasecmp(header, "If-None-Match:")) == 0) {
           continue;
         }

        /* Other lines: collect & forward. */
        else {
          sprintf(requestHeaders + strlen(requestHeaders), "%s %s\r\n", header, body);
        }

      } //END while loop.
      
      /* Check FLAG to see if Host: header was added: */
      if (addedHost == 0) {
        /* If HOST has not been added yet, add it: */
        sprintf(requestHeaders + strlen(requestHeaders), "Host: %s\r\n", host);
        addedHost = 1;
      }
      sprintf(requestHeaders + strlen(requestHeaders), "Connection: close\r\n");
      sprintf(requestHeaders + strlen(requestHeaders), "Proxy-Connection: close\r\n");
      sprintf(requestHeaders + strlen(requestHeaders), user_agent_hdr);

      /* Add last line: */
      sprintf(requestHeaders + strlen(requestHeaders), "\r\n");

      /* Writing to server: */
       if ( (rio_writen(fd_serv, requestHeaders, strlen(requestHeaders))) == -1 ){
       /* If error, reset errno, close connection, and return: */
        errno = 0;
        close(fd_serv);
        return;
       }


      /* WHILE LOOP #2: Reading response headers from server:  */
        /* Init: */
      rio_readinitb(&rio_server, fd_serv);
      /* Read line by line: */
      while(rio_readlineb(&rio_server, buf, MAXLINE) > 2) { 
        if (errno == ECONNRESET) {
          /* If error, reset errno, close connection, and return: */
          errno = 0;
          close(fd_serv);
          return;
        }
        if ( (rio_writen(fd, buf, strlen(buf))) == -1) {
          /* If error, reset errno, close connection, and return: */
          errno = 0;
          close(fd_serv);
          return;
        }
          /* Fill responseHeaders array: */
        sprintf(responseHeaders + strlen(responseHeaders), buf);

        /* Cache: */
        /* Checking if URL status matches: */
        if((strncasecmp((buf + 9), "200 OK", 6)) == 0) {
          /* If 200 OK, set status to true: */
          hasCorrectStatus = 1;
        }
        /* Checking if content length header is present: */
        if((strncasecmp(buf, "Content-Length:", 15)) == 0) {
          hasSizeHeader = 1;
          /* If present, store object size: */
          sscanf(buf, "%s", tempHeader);
          strncpy(tempBody, (buf + strlen(tempHeader)), (strlen(buf)-(strlen(tempHeader))-2));
          tempBody[(strlen(buf)-(strlen(tempHeader))-2)] = '\0';
          objectSize = atoi(tempBody);
        }

    }//END while loop #2.
    /* Add last line: */
      if ( (rio_writen(fd, buf, strlen(buf))) == -1) {
        /* If error, reset errno, close connection, and return: */
        errno = 0;
        close(fd_serv);
        return;
      } 
      sprintf(responseHeaders + strlen(responseHeaders), "\r\n");


      /* Check first 3 conditions for if we should store binary file for cache: */
      if ( (hasCorrectStatus == 1) && (hasSizeHeader == 1) &&
          (objectSize <= MAX_OBJECT_SIZE)  ) {
        /* Matches all conditions: set flag and MALLOC: */
        if ((binContent = malloc(objectSize)) == NULL) {
          /* if NULL, memory not successfully allocated: */
          doCache = 0;
        }
        else {
          doCache = 1;
        }


      }
      /* WHILE LOOP #3: one more while loop for counting bytes: */
      int bytesRead = 0;
      int count = 0;
      while((len = rio_readnb(&rio_server, buf, MAXLINE)) > 0) { 
      if (errno == ECONNRESET) {
        /* If error, reset errno, close connection, and return: */
          errno = 0;
          close(fd_serv);
          return;
      }
      if ( (rio_writen(fd, buf, len)) == -1) {
        /* If error, reset errno, close connection, and return: */
        errno = 0;
        close(fd_serv);
        return;
      }
      count = count + len;
      //Cache: //
      /* If doCache = true (1), store into successfully allocated space. */
      if(doCache == 1) {
        memcpy(binContent + bytesRead, buf, len);
        bytesRead = bytesRead + len;
      }

    }//END while loop #3.
      if (count != objectSize) {
        free(binContent);
        doCache = 0;
      }

      /* Checking all cache conditions are true: */
      if (doCache == 1) {
        /* Matches all conditions, store into cache: */
        cache_URL(uri, responseHeaders, binContent, objectSize, cache);
      }
        
      /* Close connection: */  
      if (close(fd_serv) == -1) {
          return;
      }

  }//END OF IF STATEMENT: ITEM Found in CACHE:
  else {
    /* If item is found, no need to connect to webserver;
     * just take directly from cache.
     */
    if ( (rio_writen(fd, item->headers, strlen(item->headers))) == -1) {
      /* If error, reset errno and return: */
        errno = 0;
        return;
        }
    if ( (rio_writen(fd, item->item_p, item->size)) == -1) {
      /* If error, reset errno and return: */
        errno = 0;
        return;
        }


  }

} //END function DOIT.

  /* Parse_url */
  int parse_url(const char *url, char *host, char *port, char *path) {
    int len = strlen(url);
    if (strncasecmp(url, "http://", 7) == 0) {
      /* Proper http scheme url handling: */
      int i;
      for (i = 7; i < len; i++) {

        /* If host ends with : */
        if (url[i] == ':') {
          strncpy(host, (url+1)+6, (i-1)-6);
          host[i-1-6] = '\0';
          /* Port: */
          int j;
          for(j = i; j <= len; j++) {
            /* If port ends with / */
            if (url[j] == '/'){
              strncpy(port, (url+1)+6+strlen(host)+1, j-1-6-strlen(host)-1); //(i-2)-6);
              port[j-1-6-strlen(host)-1] = '\0';
              strncpy(path, url +strlen(host)+1+strlen(port)+6+1,(len-1)-(6+strlen(host)+strlen(port)));
              path[(len-1)-(6+strlen(host)+strlen(port))] = '\0';
              /* Done */
              break;
            }
            /* If port ends at end of string: */
            else if(url[j] == '\0'){
              strncpy(port, (url+1)+6+strlen(host)+1, (j-1)-6-strlen(host)-1);
              port[j-1-6-strlen(host)-1] = '\0';
              /* Default path: */
              strncpy(path, "/", 1);
              path[1] = '\0';
              /* Done */
              break;
            }
          }
          break;
        }
        /* If host ends with / */
        else if (url[i] == '/') {
          strncpy(host, (url+1)+6, (i-1)-6);
          host[i-1-6] = '\0';
          /* Default port: */
          strncpy(port, "80", 2);
          port[2] = '\0';
          /* If host ends with / */
          if (url[strlen(host)+6+1] == '/'){
            strncpy(path, url+strlen(host)+6+1, ((strlen(url)) - (strlen(host)+6+1)));
            path[((strlen(url))-(strlen(host)+6+1))] = '\0';
            /* Done */
            break;
          }
          /* If host ends with end of string: */
          else{
            /* Default path: */
            strncpy(path, "/", 1);
            path[1] = '\0';
            /* Done */
            break;
          }
         break;
        }
      /* if reach end of string and no : or /: */
      else {
        if (i == strlen(url)-1){
          /* host = entire string after scheme: */
          strncpy(host, url+1+6, i-6);
          host[i-6] = '\0';
          /* Default port and path: */
         strncpy(port, "80", 2);
         port[2] = '\0';
         strncpy(path, "/", 1);
         path[1] = '\0';
         /* Done */
         break;
       }
      }
     }
     /* Deal with "http://" : */
      if (i == strlen(url)){
        strncpy(host, url+1+6, i-6);
        host[i-6] = '\0';
        strncpy(port, "80", 2);
        port[2] = '\0';
        strncpy(path, "/", 1);
        path[1] = '\0';
      }
      return 1;
    }
    else
      return 0;
}



