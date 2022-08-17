//
//  cccam.c
//  cccamtest
//

#include "cccam.h"
#include "network.h"
#include <ctype.h>
#include <regex.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <openssl/sha.h>

uint8_t cc_node_id[8];
struct cc_crypt_block block[2]; // crypto state blocks

char* ip;

void cc_xor(uint8_t *buf)
{
    const char cccam[] = "CCcam";
    uint8_t i;

    for(i = 0; i < 8; i++)
    {
        buf[8 + i] = i * buf[i];

        if(i <= 5)
        {
            buf[i] ^= cccam[i];
        }
    }
}

/* This function encapsulates malloc. It automatically adds an error message
   to the log if it failed and calls cs_exit(quiterror) if quiterror > -1.
   result will be automatically filled with the new memory position or NULL
   on failure. */
bool cs_malloc(void *result, size_t size)
{
    void **tmp = result;
    *tmp = malloc(size);

    if(*tmp == NULL)
    {
        fprintf(stderr, "%s: ERROR: Can't allocate %zu bytes!", __func__, size);
    }
    else
    {
        memset(*tmp, 0, size);
    }
    return !!*tmp;
}

/* strlen is wrongly used in oscam. Calling strlen directly
 * e.g. strlen(somechar) can cause segmentation fault if pointer to somechar is NULL!
 * This one looks better?
 */
size_t cs_strlen(const char *c)
{
    if (c == NULL)
    {
        return 0;
    }
    else
    {
        if (c[0] == '\0')
        {
            return 0;
        }
        else
        {
            return strlen(c);
        }
    }
}

int32_t cc_connect(struct cline* line) {
    ip = lookup_host(line->host);
    int32_t fd = network_tcp_connection_open(ip, line->port);
    
    if(fd <= 0)
        return -1;

    if(errno == EISCONN)
        return -1;
    
    uint8_t data[20];
    int32_t n;
    
    // get init seed
    if((n = cc_recv_to(fd, data, 16)) != 16)
    {
        cc_cli_close(fd);
        return -2;
    }
    
    cc_xor(data); // XOR init bytes with 'CCcam'

    uint8_t hash[SHA_DIGEST_LENGTH];
    
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, 16);
    SHA1_Final(hash, &ctx);
    
    // initialisate crypto states
    cc_init_crypt(&block[DECRYPT], hash, 20);
    cc_crypt(&block[DECRYPT], data, 16, DECRYPT);
    cc_init_crypt(&block[ENCRYPT], data, 16);
    cc_crypt(&block[ENCRYPT], hash, 20, DECRYPT);

    cc_cmd_send(fd, hash, 20, MSG_NO_HEADER); // send crypted hash to server

    uint8_t buf[CC_MAXMSGSIZE];
    char pwd[65];
    
    memset(buf, 0, CC_MAXMSGSIZE);
    memcpy(buf, line->user, strlen(line->user));
    cc_cmd_send(fd, buf, 20, MSG_NO_HEADER); // send usr '0' padded -> 20 bytes

    memset(buf, 0, CC_MAXMSGSIZE);
    memset(pwd, 0, sizeof(pwd));

    //cs_log_dbg(D_CLIENT, "cccam: 'CCcam' xor");
    memcpy(buf, "CCcam", 5);
    strncpy(pwd, line->pass, sizeof(pwd));
    int32_t pwd_size = (int32_t)strlen(pwd);
    cc_crypt(&block[ENCRYPT], (uint8_t *)pwd, pwd_size, ENCRYPT);
    cc_cmd_send(fd, buf, 6, MSG_NO_HEADER); // send 'CCcam' xor w/ pwd

    if((cc_recv_to(fd, data, 20)) != 20)
    {
        //cs_log("%s login failed, usr/pwd invalid", getprefix());
        cc_cli_close(fd);
        return -2;
    }

    cc_crypt(&block[DECRYPT], data, 20, DECRYPT);

    if(memcmp(data, buf, 5)) // check server response
    {
        //Login failed, usr/pwd invalid
        cc_cli_close(fd);
        return -2;
    }
   
    //login succeeded, send client data
    if(cc_send_cli_data(fd, line) <= 0)
    {
        //login failed, could not send client data
        cc_cli_close(fd);
        return -3;
    }
    
    return fd;
}

void cc_cli_close(int32_t fd) {
    network_tcp_connection_close(fd);
    if(ip) {
        free(ip);
        ip = NULL;
    }
}

int32_t cc_recv_to(int32_t fd, uint8_t *buf, int32_t len)
{
    int32_t rc;
    struct pollfd pfd;

    while(1)
    {
        pfd.fd = fd;
        pfd.events = POLLIN | POLLPRI;

        rc = poll(&pfd, 1, 3000);

        if(rc < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }

            return -1; // error!!
        }

        if(rc == 1)
        {
            if(pfd.revents & POLLHUP)
            {
                return -1; // hangup = error!!
            }
            else
            {
                break;
            }
        }
        else
        {
            return -2; // timeout!!
        }
    }
    return recv(fd, buf, len, MSG_WAITALL);
}

/**
 * reader + server
 * send a message
 */
int32_t cc_cmd_send(int32_t fd, uint8_t *buf, int32_t len, cc_msg_type_t cmd)
{
    if(!fd) // disconnected
    {
        return -1;
    }

    int32_t n;
    uint8_t *netbuf;
    if(!cs_malloc(&netbuf, len + 4))
    {
        return -1;
    }

    if(cmd == MSG_NO_HEADER)
    {
        memcpy(netbuf, buf, len);
    }
    else
    {
        uint8_t g_flag = 0;
        // build command message
        netbuf[0] = g_flag; // flags?
        netbuf[1] = cmd & 0xff;
        netbuf[2] = len >> 8;
        netbuf[3] = len & 0xff;

        if(buf)
        {
            memcpy(netbuf + 4, buf, len);
        }
        len += 4;
    }

    cc_crypt(&block[ENCRYPT], netbuf, len, ENCRYPT);

    n = send(fd, netbuf, len, 0);

    NULLFREE(netbuf);

    if(n != len)
    {
        cc_cli_close(fd);
        n = -1;
    }

    return n;
}

void cc_init_crypt(struct cc_crypt_block *block, uint8_t *key, int32_t len)
{
    int32_t i = 0;
    uint8_t j = 0;

    for(i = 0; i < 256; i++)
    {
        block->keytable[i] = i;
    }

    for(i = 0; i < 256; i++)
    {
        j += key[i % len] + block->keytable[i];
        SWAPC(&block->keytable[i], &block->keytable[j]);
    }

    block->state = *key;
    block->counter = 0;
    block->sum = 0;
}

void cc_crypt(struct cc_crypt_block *block, uint8_t *data, int32_t len, cc_crypt_mode_t mode)
{
    int32_t i;
    uint8_t z;

    for(i = 0; i < len; i++)
    {
        block->counter++;
        block->sum += block->keytable[block->counter];
        SWAPC(&block->keytable[block->counter], &block->keytable[block->sum]);

        z = data[i];
        data[i] = z ^ block->keytable[(block->keytable[block->counter] + block->keytable[block->sum]) & 0xff];
        data[i] ^= block->state;

        if(!mode)
        {
            z = data[i];
        }

        block->state = block->state ^ z;
    }
}

/**
 * reader
 * sends own version information to the CCCam server
 */
int32_t cc_send_cli_data(int32_t fd, struct cline* line)
{
    const int32_t size = 20 + 8 + 6 + 26 + 4 + 28 + 1;
    uint8_t buf[size];

    //cccam: send client data
    uint8_t node_id[8] = { 1, 2, 1, 2, 1, 9, 8, 7 };
    memcpy(node_id, cc_node_id, sizeof(cc_node_id));

    memcpy(buf, line->user, sizeof(line->user));
    memcpy(buf + 20, node_id, 8);
    buf[28] = 0; // <-- Client wants to have EMUs, 0 - NO; 1 - YES
    char cc_version[7]; // cccam version
    char cc_build[7];   // cccam build number
    strcpy(cc_version, "2.2.1");
    strcpy(cc_build, "3316");
    memcpy(buf + 29, cc_version, sizeof(cc_version)); // cccam version (ascii)
    memcpy(buf + 61, cc_build, sizeof(cc_build)); // build number (ascii)

    return cc_cmd_send(fd, buf, size, MSG_CLI_DATA);
}

bool cc_test_line(struct cline* line)
{
    if(strlen(line->host) < 3 || strlen(line->user) < 1 || strlen(line->pass) < 1)
        return false;

    int32_t fc = cc_connect(line);
    
    const bool result = fc > 0;
    
    cc_cli_close(fc);
    return result;
}

int cc_extract_line_field(char *str, char *field)
{
  int s = 0;
  for(;str[s] == ' ';s++)
    continue;  
  int d = 0;
  while(str[s] != ' ' && isprint(str[s]))
    field[d++] = str[s++];  
  field[d] = '\0';   
  return s;
}

struct cline cc_parse_line(const char *line) 
{  
  struct cline result;  
  const int l_len = strlen(line);
  if(l_len < 5)
    return result;  

  int beginIndex = 0;  
  while(line[beginIndex++] != ' ');
  if(beginIndex > l_len || beginIndex > 100)
    return result;
  char str[100];   
  strncpy(str, line + beginIndex, strlen(line) - beginIndex);  

  int s = cc_extract_line_field(str,result.host);
  char *ptr;
  long ret = strtol(str + s, &ptr, 10);
  if(ret < 1000)
    ret = 12000; //set the default CCcam port
  result.port = (int)ret;

  s = cc_extract_line_field(ptr,result.user);
  cc_extract_line_field(ptr+s,result.pass);

  return result;
}

bool cc_validate_line(const char* line) {
    bool result = false;
    regex_t regex;
    int reti = regcomp(&regex, "^[Cc]:[ ]+[^ ]+ +[0-9]+ +[^ ]+ +[^ ]+$", REG_EXTENDED);
    if (reti) {
        return false;
    }

    /* Execute regular expression */
    reti = regexec(&regex, line, 0, NULL, 0);
    if (!reti) {
        result = true;
    }
    else if (reti == REG_NOMATCH) {
        result = false;
    }
    else {
        regerror(reti, &regex, (char *)line, sizeof(line));
        return false;
    }

    /* Free memory allocated to the pattern buffer by regcomp() */
    regfree(&regex);
    return result;
}

bool writeOscamLine(struct cline* line, const char* filename) {
    FILE *fp = fopen(filename, "a");
    
    if(fp == NULL)
        return false;
    
    fprintf(fp, "[reader]\n");
    fprintf(fp, "protocol = cccam\n");
    fprintf(fp, "group = 1\n");
    fprintf(fp, "ccckeepalive = 1\n");
    fprintf(fp, "device = %s,%d\n", line->host, line->port);
    fprintf(fp, "user = %s\n", line->user);
    fprintf(fp, "password = %s\n", line->pass);
    fclose(fp);
    return true;
}