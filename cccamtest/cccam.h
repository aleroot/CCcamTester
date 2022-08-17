//
//  cccam.h
//  cccamtest
//

#ifndef cccam_h
#define cccam_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define CC_MAXMSGSIZE 0x400 // by Project::Keynation: Buffer size is limited on "O" CCCam to 1024 bytes
#define CC_MAX_PROV 32
#define SWAPC(X, Y) do { char p; p = *X; *X = *Y; *Y = p; } while(0)
// checking if (X) free(X) unneccessary since freeing a null pointer doesnt do anything
#define NULLFREE(X) {if (X) {void *tmpX=X; X=NULL; free(tmpX); }}

struct cline {
    char host[100];
    unsigned int port;
    char user[64];
    char pass[64];
};

typedef enum
{
    DECRYPT, ENCRYPT
} cc_crypt_mode_t;

typedef enum
{
    MSG_CLI_DATA = 0,
    MSG_CW_ECM = 1,
    MSG_EMM_ACK = 2,
    MSG_CARD_REMOVED = 4,
    MSG_CMD_05 = 5,
    MSG_KEEPALIVE = 6,
    MSG_NEW_CARD = 7,
    MSG_SRV_DATA = 8,
    MSG_CMD_0A = 0x0a,
    MSG_CMD_0B = 0x0b,
    MSG_CMD_0C = 0x0c, // CCCam 2.2.x fake client checks
    MSG_CMD_0D = 0x0d, // "
    MSG_CMD_0E = 0x0e, // "
    MSG_NEW_CARD_SIDINFO = 0x0f,
    MSG_SLEEPSEND = 0x80, // Sleepsend support
    MSG_CACHE_PUSH = 0x81, // CacheEx Cache-Push In/Out
    MSG_CACHE_FILTER = 0x82, // CacheEx Cache-Filter Request
    MSG_CW_NOK1 = 0xfe, // Node no more available
    MSG_CW_NOK2 = 0xff, // No decoding
    MSG_NO_HEADER = 0xffff
} cc_msg_type_t;

struct cc_crypt_block
{
    uint8_t keytable[256];
    uint8_t state;
    uint8_t counter;
    uint8_t sum;
};

bool writeOscamLine(struct cline* line, const char* filename);

bool cc_validate_line(const char* line);
struct cline cc_parse_line(const char *line);
bool cc_test_line(struct cline* line);
int32_t cc_connect(struct cline* line);
void cc_cli_close(int32_t fd);
int32_t cc_recv_to(int32_t fd, uint8_t *buf, int32_t len);
int32_t cc_cmd_send(int32_t fd, uint8_t *buf, int32_t len, cc_msg_type_t cmd);
int32_t cc_send_cli_data(int32_t fd, struct cline* line);
void cc_init_crypt(struct cc_crypt_block *block, uint8_t *key, int32_t len);
void cc_crypt(struct cc_crypt_block *block, uint8_t *data, int32_t len, cc_crypt_mode_t mode);

#endif /* cccam_h */
