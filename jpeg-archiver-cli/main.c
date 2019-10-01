#include <getopt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jpeg-recompress.h>

bool process_file(const char *inPath, const char *outPath)
{
    FILE *inFile = NULL;
    errno_t err = fopen_s(&inFile, inPath, "rb");
    if(err != 0) {
        char errorMsg[256];
        strerror_s(errorMsg, _countof(errorMsg), err);
        fprintf(stderr, "ERR  %s %s\n", inPath, errorMsg);
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
    FILE* outFile;
    err = fopen_s(&outFile, outPath, "wb");
    if(err != 0) {
        char errorMsg[256];
        strerror_s(errorMsg, _countof(errorMsg), err);
        fprintf(stderr, "ERR  %s %s\n", inPath, errorMsg);
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

    if(!process_file(argv[1], argv[2]))
        return EXIT_FAILURE;
    return  EXIT_SUCCESS;
}
