
#include "benchmark.h"
#include "md5.h"

gchar *get_test_data(gsize min_size) {
    gchar *bdata_path, *data;
    gsize data_size;

    gchar *exp_data, *p;
    gsize sz;

    bdata_path = g_build_filename(params.path_data, "benchmark.data", NULL);
    if (!g_file_get_contents(bdata_path, &data, &data_size, NULL)) {
        g_free(bdata_path);
        return NULL;
    }

    if (data_size < min_size) {
        DEBUG("expanding %lu bytes of test data to %lu bytes", data_size, min_size);
        exp_data = g_malloc(min_size + 1);
        memcpy(exp_data, data, data_size);
        p = exp_data + data_size;
        sz = data_size;
        while (sz < (min_size - data_size) ) {
            memcpy(p, data, data_size);
            p += data_size;
            sz += data_size;
        }
        strncpy(p, data, min_size - sz);
        g_free(data);
        data = exp_data;
    }
    g_free(bdata_path);

    return data;
}

char *digest_to_str(const char *digest, int len) {
    int max = len * 2;
    char *ret = malloc(max+1);
    char *p = ret;
    memset(ret, 0, max+1);
    int i = 0;
    for(; i < len; i++) {
        unsigned char byte = digest[i];
        p += sprintf(p, "%02x", byte);
    }
    return ret;
}

char *md5_digest_str(const char *data, unsigned int len) {
    struct MD5Context ctx;
    guchar digest[16];
    MD5Init(&ctx);
    MD5Update(&ctx, (guchar *)data, len);
    MD5Final(digest, &ctx);
    return digest_to_str(digest, 16);
}
