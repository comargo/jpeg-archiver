#ifndef JPEGLIB_CPP_H
#define JPEGLIB_CPP_H

#include <cstdio>
#include <jpeglib.h>

struct jpeg_decompress_struct_RAII : jpeg_decompress_struct
{
    jpeg_decompress_struct_RAII(jpeg_error_mgr err)
        : _err(err)
    {
        this->err = &_err;
        jpeg_create_decompress(this);
    }
    ~jpeg_decompress_struct_RAII()
    {
        jpeg_destroy_decompress(this);
    }
private:
    jpeg_error_mgr _err;
};

struct jpeg_compress_struct_RAII : jpeg_compress_struct
{
    jpeg_compress_struct_RAII(jpeg_error_mgr err)
        : _err(err)
    {
        this->err = &_err;
        jpeg_create_compress(this);
    }
    ~jpeg_compress_struct_RAII()
    {
        jpeg_destroy_compress(this);
    }
private:
    jpeg_error_mgr _err;
};

#endif // JPEGLIB_CPP_H
