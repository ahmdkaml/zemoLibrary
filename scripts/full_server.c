#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <sqlite3.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

static sqlite3 *db = NULL;
static CRITICAL_SECTION db_cs;

// Simple JSON string extractor
static int json_extract_string(const char *json, const char *key, char *out, size_t max_len) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return 0;

    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ':')) p++;
    if (*p != '"') return 0;
    p++; // skip quote

    size_t i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            if (*p == '"') out[i++] = '"';
            else if (*p == '\\') out[i++] = '\\';
            else if (*p == 'n') out[i++] = '\n';
            else if (*p == 't') out[i++] = '\t';
            else out[i++] = *p;
        } else {
            out[i++] = *p;
        }
        p++;
    }
    out[i] = '\0';
    return 1;
}

// Generate 16 random bytes and base64 encode
static int generate_salt_base64(char *out_salt, size_t max_len) {
    BYTE salt[16];
    BCRYPT_ALG_HANDLE hRng = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hRng, BCRYPT_RNG_ALGORITHM, NULL, 0))) {
        return 0;
    }
    BCryptGenRandom(hRng, salt, sizeof(salt), 0);
    BCryptCloseAlgorithmProvider(hRng, 0);

    DWORD outLen = (DWORD)max_len;
    if (!CryptBinaryToStringA(salt, sizeof(salt), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out_salt, &outLen)) {
        return 0;
    }
    return 1;
}

// Compute PBKDF2-HMAC-SHA512 with 10000 iterations, 32 bytes (256 bits) key
static int hash_password_pbkdf2(const char *password, const char *salt_b64, char *out_hash_b64, size_t max_len) {
    BYTE salt[64];
    DWORD saltLen = sizeof(salt);
    if (!CryptStringToBinaryA(salt_b64, 0, CRYPT_STRING_BASE64, salt, &saltLen, NULL, NULL)) {
        return 0;
    }

    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA512_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG))) {
        return 0;
    }

    BYTE hash[32];
    NTSTATUS status = BCryptDeriveKeyPBKDF2(
        hAlg,
        (PUCHAR)password,
        (ULONG)strlen(password),
        salt,
        saltLen,
        10000,
        hash,
        sizeof(hash),
        0
    );
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (!BCRYPT_SUCCESS(status)) {
        return 0;
    }

    DWORD outLen = (DWORD)max_len;
    if (!CryptBinaryToStringA(hash, sizeof(hash), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out_hash_b64, &outLen)) {
        return 0;
    }
    return 1;
}

static void send_response(SOCKET sock, int status_code, const char *status_text, const char *content_type, const char *body) {
    char header[1024];
    int body_len = body ? (int)strlen(body) : 0;
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization, Accept, X-Requested-With\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len
    );
    send(sock, header, (int)strlen(header), 0);
    if (body_len > 0) {
        send(sock, body, body_len, 0);
    }
}

// Client request worker thread
static unsigned __stdcall handle_client(void *param) {
    SOCKET sock = (SOCKET)param;
    char buf[8192];
    int total_read = 0;

    // Read HTTP request headers
    while (total_read < sizeof(buf) - 1) {
        int r = recv(sock, buf + total_read, sizeof(buf) - 1 - total_read, 0);
        if (r <= 0) break;
        total_read += r;
        buf[total_read] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }

    if (total_read <= 0) {
        closesocket(sock);
        return 0;
    }

    char method[16] = {0};
    char path[256] = {0};
    sscanf(buf, "%15s %255s", method, path);

    // Normalize path (strip query params)
    char *q = strchr(path, '?');
    if (q) *q = '\0';

    // Handle CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(sock, 200, "OK", "text/plain", "");
        closesocket(sock);
        return 0;
    }

    // Route: GET /
    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
        const char *resp = "{\"status\":\"UP\",\"service\":\"ZemoServer\",\"version\":\"1.0.0\"}\n";
        send_response(sock, 200, "OK", "application/json", resp);
        closesocket(sock);
        return 0;
    }

    // Route: GET /hello
    if (strcmp(method, "GET") == 0 && strcmp(path, "/hello") == 0) {
        send_response(sock, 200, "OK", "text/plain", "hello");
        closesocket(sock);
        return 0;
    }

    // Route: GET /auth/status or GET /api/auth/status
    if (strcmp(method, "GET") == 0 &&
        (strcmp(path, "/auth/status") == 0 || strcmp(path, "/api/auth/status") == 0)) {
        const char *resp = "{\"status\":\"UP\",\"service\":\"zemoLibrary-auth\"}\n";
        send_response(sock, 200, "OK", "application/json", resp);
        closesocket(sock);
        return 0;
    }

    // Route: GET /users or GET /users?name=...
    if (strcmp(method, "GET") == 0 &&
        (strcmp(path, "/users") == 0 || strncmp(path, "/users?", 7) == 0)) {
        char search_name[128] = {0};
        char *q = strstr(path, "name=");
        if (q) {
            strncpy(search_name, q + 5, sizeof(search_name) - 1);
            for (int i = 0; search_name[i]; i++) {
                if (search_name[i] == '+') search_name[i] = ' ';
            }
        }

        EnterCriticalSection(&db_cs);
        sqlite3_stmt *stmt = NULL;
        if (strlen(search_name) > 0) {
            sqlite3_prepare_v2(db, "SELECT id, name, email FROM users WHERE name LIKE ? COLLATE NOCASE;", -1, &stmt, NULL);
            char like_pattern[140];
            snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", search_name);
            sqlite3_bind_text(stmt, 1, like_pattern, -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_prepare_v2(db, "SELECT id, name, email FROM users;", -1, &stmt, NULL);
        }

        char resp[8192] = "[\n";
        int first = 1;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            long long id = sqlite3_column_int64(stmt, 0);
            const char *n = (const char*)sqlite3_column_text(stmt, 1);
            const char *e = (const char*)sqlite3_column_text(stmt, 2);
            char item[512];
            snprintf(item, sizeof(item), "%s  {\"id\":%lld,\"name\":\"%s\",\"email\":\"%s\"}",
                first ? "" : ",\n", id, n ? n : "", e ? e : "");
            first = 0;
            strncat(resp, item, sizeof(resp) - strlen(resp) - 1);
        }
        strncat(resp, "\n]\n", sizeof(resp) - strlen(resp) - 1);
        sqlite3_finalize(stmt);
        LeaveCriticalSection(&db_cs);

        send_response(sock, 200, "OK", "application/json", resp);
        closesocket(sock);
        return 0;
    }

    // Extract body for POST requests
    char *body = strstr(buf, "\r\n\r\n");
    if (body) body += 4;
    else body = "";

    // Check Content-Length to read remainder of body if needed
    int content_length = 0;
    char *cl_hdr = strstr(buf, "Content-Length:");
    if (!cl_hdr) cl_hdr = strstr(buf, "content-length:");
    if (cl_hdr) {
        content_length = atoi(cl_hdr + 15);
    }
    int body_read = total_read - (int)(body - buf);
    while (body_read < content_length && total_read < sizeof(buf) - 1) {
        int r = recv(sock, buf + total_read, min(content_length - body_read, (int)sizeof(buf) - 1 - total_read), 0);
        if (r <= 0) break;
        total_read += r;
        body_read += r;
        buf[total_read] = '\0';
    }

    // Route: POST /auth/signup or POST /api/auth/signup
    if (strcmp(method, "POST") == 0 &&
        (strcmp(path, "/auth/signup") == 0 || strcmp(path, "/api/auth/signup") == 0)) {
        char name[128] = {0};
        char email[128] = {0};
        char password[128] = {0};

        json_extract_string(body, "name", name, sizeof(name));
        json_extract_string(body, "email", email, sizeof(email));
        json_extract_string(body, "password", password, sizeof(password));

        // Normalize email to lowercase
        for (int i = 0; email[i]; i++) email[i] = (char)tolower((unsigned char)email[i]);

        // Validation
        if (strlen(email) == 0) {
            send_response(sock, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Email is required\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }
        if (strlen(password) < 6) {
            send_response(sock, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Password must be at least 6 characters\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }

        EnterCriticalSection(&db_cs);

        // Check duplicate email
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "SELECT id FROM users WHERE email = ?;", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
        int exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);

        if (exists) {
            LeaveCriticalSection(&db_cs);
            send_response(sock, 400, "Bad Request", "application/json",
                "{\"success\":false,\"message\":\"Email is already registered\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }

        char salt_b64[64] = {0};
        generate_salt_base64(salt_b64, sizeof(salt_b64));

        char hash_b64[128] = {0};
        hash_password_pbkdf2(password, salt_b64, hash_b64, sizeof(hash_b64));

        sqlite3_prepare_v2(db, "INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?);", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, email, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, hash_b64, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, salt_b64, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        long long user_id = sqlite3_last_insert_rowid(db);
        LeaveCriticalSection(&db_cs);

        char resp[512];
        snprintf(resp, sizeof(resp),
            "{\"success\":true,\"message\":\"User registered successfully\",\"user\":{\"id\":%lld,\"name\":\"%s\",\"email\":\"%s\"}}\n",
            user_id, name, email);
        send_response(sock, 201, "Created", "application/json", resp);
        closesocket(sock);
        return 0;
    }

    // Route: POST /auth/login or POST /api/auth/login
    if (strcmp(method, "POST") == 0 &&
        (strcmp(path, "/auth/login") == 0 || strcmp(path, "/api/auth/login") == 0)) {
        char email[128] = {0};
        char password[128] = {0};

        json_extract_string(body, "email", email, sizeof(email));
        json_extract_string(body, "password", password, sizeof(password));

        for (int i = 0; email[i]; i++) email[i] = (char)tolower((unsigned char)email[i]);

        if (strlen(email) == 0) {
            send_response(sock, 401, "Unauthorized", "application/json",
                "{\"success\":false,\"message\":\"Email is required\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }
        if (strlen(password) == 0) {
            send_response(sock, 401, "Unauthorized", "application/json",
                "{\"success\":false,\"message\":\"Password is required\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }

        EnterCriticalSection(&db_cs);
        sqlite3_stmt *stmt = NULL;
        sqlite3_prepare_v2(db, "SELECT id, name, password_hash, salt FROM users WHERE email = ?;", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);

        int found = 0;
        long long user_id = 0;
        char name[128] = {0};
        char stored_hash[128] = {0};
        char stored_salt[64] = {0};

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            found = 1;
            user_id = sqlite3_column_int64(stmt, 0);
            const char *n = (const char*)sqlite3_column_text(stmt, 1);
            if (n) strncpy(name, n, sizeof(name) - 1);
            const char *h = (const char*)sqlite3_column_text(stmt, 2);
            if (h) strncpy(stored_hash, h, sizeof(stored_hash) - 1);
            const char *s = (const char*)sqlite3_column_text(stmt, 3);
            if (s) strncpy(stored_salt, s, sizeof(stored_salt) - 1);
        }
        sqlite3_finalize(stmt);
        LeaveCriticalSection(&db_cs);

        if (!found) {
            send_response(sock, 401, "Unauthorized", "application/json",
                "{\"success\":false,\"message\":\"Invalid email or password\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }

        char computed_hash[128] = {0};
        hash_password_pbkdf2(password, stored_salt, computed_hash, sizeof(computed_hash));

        if (strcmp(computed_hash, stored_hash) != 0) {
            send_response(sock, 401, "Unauthorized", "application/json",
                "{\"success\":false,\"message\":\"Invalid email or password\",\"user\":null}\n");
            closesocket(sock);
            return 0;
        }

        char resp[512];
        snprintf(resp, sizeof(resp),
            "{\"success\":true,\"message\":\"Login successful\",\"user\":{\"id\":%lld,\"name\":\"%s\",\"email\":\"%s\"}}\n",
            user_id, name, email);
        send_response(sock, 200, "OK", "application/json", resp);
        closesocket(sock);
        return 0;
    }

    // 404 Not Found
    send_response(sock, 404, "Not Found", "application/json",
        "{\"error\":\"Not Found\",\"status\":404}\n");
    closesocket(sock);
    return 0;
}

int main() {
    InitializeCriticalSection(&db_cs);

    // Initialize SQLite database
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char *lastSlash = strrchr(exePath, '\\');
    char dbPath[MAX_PATH];
    if (lastSlash) {
        *lastSlash = '\0';
        snprintf(dbPath, sizeof(dbPath), "%s\\users.db", exePath);
    } else {
        strcpy(dbPath, "users.db");
    }

    if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database %s: %s\n", dbPath, sqlite3_errmsg(db));
        return 1;
    }

    char *err_msg = NULL;
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT,"
        "  email TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL"
        ");";
    if (sqlite3_exec(db, create_sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }

    // Start Winsock server on port 8080
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(listenSock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed on port 8080\n");
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed\n");
        return 1;
    }

    printf("ZemoServer running on 0.0.0.0:8080 (DB: %s)\n", dbPath);

    while (1) {
        SOCKET clientSock = accept(listenSock, NULL, NULL);
        if (clientSock == INVALID_SOCKET) break;
        _beginthreadex(NULL, 0, handle_client, (void *)clientSock, 0, NULL);
    }

    closesocket(listenSock);
    sqlite3_close(db);
    DeleteCriticalSection(&db_cs);
    WSACleanup();
    return 0;
}
