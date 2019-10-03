#include <getopt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jpeg-recompress.h>

bool process_file(const char *inPath, const char *outPath, unsigned int nAttempts)
{
    FILE *inFile = fopen(inPath, "rb");
    if(!inFile) {
        fprintf(stderr, "ERR  %s %s\n", inPath, strerror(errno));
        return false;
    }

    fseek(inFile, 0, SEEK_END);
    size_t inFileSize = (size_t)ftell(inFile);
    rewind(inFile);

    unsigned char *inBuf = malloc(inFileSize);
    fread(inBuf, 1, inFileSize, inFile);
    fclose(inFile);

    struct jpeg_recompress_config_t config;
    jr_default_config(&config);
    if(nAttempts != 0)
        config.attempts = nAttempts;

    unsigned char *outBuffer = NULL;
    size_t outBufferSize = 0;
    int quality = jpeg_recompress(inBuf, inFileSize, &config, &outBuffer, &outBufferSize);

    if(!outBufferSize) {
        fprintf(stderr, "SKIP %s\n", inPath);
        free(inBuf);
        return false;
    }

    if(!quality) {
        fprintf(stderr, "COPY %s\n",inPath);
    }
    else {
        fprintf(stderr,
                "DONE %s, q=%d, original=%d, new=%d, ratio=%d%%\n",
                inPath, quality, (int)inFileSize, (int)outBufferSize, (int)(100*outBufferSize/inFileSize));
    }
    FILE* outFile = fopen(outPath, "wb");
    if(!outFile) {
        fprintf(stderr, "ERR  %s %s\n", inPath, strerror(errno));
        free(inBuf);
        free(outBuffer);
        return false;
    }
    fwrite(outBuffer, 1, outBufferSize, outFile);
    fclose(outFile);
    return true;
}

int main(int argc, char *argv[])
{
    if(argc < 3) {
        fprintf(stderr,
                "Not enought parameters\n"
                "jpeg-archiver-cli <input> <output>\n");
        return EXIT_FAILURE;
    }

    unsigned int nAttempts = 0;
    if(argc == 4) {
        nAttempts = (unsigned int)atoi(argv[3]);
    }

    if(!process_file(argv[1], argv[2], nAttempts))
        return EXIT_FAILURE;
    return  EXIT_SUCCESS;
}
