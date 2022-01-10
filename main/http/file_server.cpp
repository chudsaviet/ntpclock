/*
    Code based on
    https://github.com/espressif/esp-idf/blob/master/examples/protocols/http_server/file_serving/main/file_server.c
*/
#include "file_server.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <esp_spiffs.h>

#define TAG "file_server.cpp"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define SCRATCH_BUFSIZE_BYTES  8192

#define SPIFFS_BASE_PATH "/spiffs/w"

#define INDEX_PAGE "/index.html"

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

#define GZIP_EXTENSION ".gz"
#define GZIP_EXTENSION_LEN 3
#define GZIP_CONTENT_ENCODING "gzip"

static char scratch[SCRATCH_BUFSIZE_BYTES];

static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".png")) {
        return httpd_resp_set_type(req, "image/png");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } 

    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static esp_err_t download_get_handler(httpd_req_t *req)
{
    char vfs_path[FILE_PATH_MAX+1];
    memset(vfs_path, 0, sizeof(vfs_path));
    FILE *fd = NULL;
    struct stat file_stat;

    const char *file_name = get_path_from_uri(vfs_path, SPIFFS_BASE_PATH, req->uri, sizeof(vfs_path));
    // get_path_from_uri() returns NULL if file path is too.
    if (!file_name) {
        ESP_LOGI(TAG, "Requested file path is too long.");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 NOT FOUND - server does not support this long file names.");
        // We are actually OK, we just don't have this file.
        return ESP_OK;
    }

    // By default send index page.
    if (strcmp(file_name, "/") == 0) {
        file_name = get_path_from_uri(vfs_path, SPIFFS_BASE_PATH, INDEX_PAGE, sizeof(vfs_path));
    }

    size_t vfs_path_len = strlen(vfs_path);
    set_content_type_from_file(req, file_name);

    // We will look for compressed file only if there is enought space for the extension in the max path len.
    if (vfs_path_len < FILE_PATH_MAX - GZIP_EXTENSION_LEN) {
        strcpy(vfs_path+vfs_path_len, GZIP_EXTENSION);
        if (stat(vfs_path, &file_stat) != -1) {
            ESP_LOGI(TAG, "Compressed file found.");
            httpd_resp_set_hdr(req, "Content-Encoding", GZIP_CONTENT_ENCODING);
        } else {
            // Cut compression extension from path.
            vfs_path[vfs_path_len] = 0;
        }
    } else {
        if (stat(vfs_path, &file_stat) == -1) {
            ESP_LOGI(TAG, "Failed to stat file : %s", vfs_path);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 NOT FOUND");
            // We are actually OK, we just don't have this file.
            return ESP_OK;
        }
    }

    fd = fopen(vfs_path, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", vfs_path);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "500 Failed to read existing file.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", file_name, file_stat.st_size);
    
    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE_BYTES, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void register_file_server_handlers(httpd_handle_t server) {
    httpd_uri_t handler_params = {0};

    ESP_LOGD(TAG, "Registering file GET handler.");
    handler_params = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = download_get_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);
}