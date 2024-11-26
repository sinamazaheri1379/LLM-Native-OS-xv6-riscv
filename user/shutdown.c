//
// Created by sina-mazaheri on 11/6/24.
//

// Include types.h first for uint definitions

#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc > 1) {
        fprintf(2, "Usage: shutdown\n");
        exit(1);
    }

    if(shutdown() < 0) {
        fprintf(2, "shutdown failed\n");
        exit(1);
    }
    exit(0);
}