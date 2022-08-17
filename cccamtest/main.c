//
//  main.c
//  cccamtest
//

#include "cccam.h"
#include "util.h"

int WAIT_SECONDS = 9;
bool isOscamFile = false, isCfgFile = false;
const char * oscamFile = NULL;

/*
 *          CCcam C Line Tester
 * cccamtest is a tester of cccam c lines 
 * returns 0 for success, error code for a failure 
 * 
 *      GNU GENERAL PUBLIC LICENSE
 *        Version 3, 29 June 2007
 */
int main(int argc, const char * argv[]) {
    int result = 0;
    FILE* filePointer = stdin;
    for(int i = 1; i < argc; i++) {
        const char * par = argv[i];
        int s = 0;
        for(;par[s] == '-'; s++);
        if(s > 0) {
            switch(par[s]) {
                case 'o':
                oscamFile = "oscam.server";
                isOscamFile = true;
                break;
                case 'c':
                isCfgFile = true;
                break;
            }
            continue;
        }
        if(isOscamFile) {
            oscamFile = par;
            isOscamFile = false;
            continue;
        }
        if(isCfgFile) {
            filePointer = fopen(par, "r");
            WAIT_SECONDS = -1;
            isCfgFile = false;
            continue;
        }
        struct cline cline = cc_parse_line(argv[i]);
        const bool isValid = cc_test_line(&cline);
        printf("C line %s:%d is %s!\n", cline.host, cline.port, (isValid ? "valid" : "invalid"));
        result += (isValid ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    //for loop is just to have a scoped int variable...
    for (int n =0; ioctl(0, FIONREAD, &n) == 0 && n <= 0; ) {
        printf("Enter CCcam C line: ");
        fflush(stdout);
        break;
    }

    char line[235];
    while(getUserInput(filePointer, line, 235, WAIT_SECONDS) > 5) {
        if(!cc_validate_line(line)) {
            printf("Illegal CCcam line -> %s!\n", line);
            result = EXIT_FAILURE;
            continue;
        }
        struct cline cline = cc_parse_line(line);
        const bool isValid = cc_test_line(&cline);
        printf("C line %s:%d is %s!\n", cline.host, cline.port, (isValid ? "valid" : "invalid"));
        result += (isValid ? EXIT_SUCCESS : EXIT_FAILURE);
        if(isValid && oscamFile)
            writeOscamLine(&cline, oscamFile);
    }
    fclose(filePointer);
    return result;
}
