#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket,
                       const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[]) {
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval,
               sizeof(optval));

    if (bind(server_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket =
            accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }

        printf("Accepted connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app) {
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
            case 'b':
                app->server_port = atoi(optarg);
                break;
            case 'r':
                app->remote_host = strdup(optarg);
                break;
            case 'p':
                app->remote_port = atoi(optarg);
                break;
            default: /* Unrecognized parameter or "-?" */
                fprintf(stderr,
                        "Usage: server [-b local_port] [-r remote_host] [-p "
                        "remote_port]\n");
                exit(-1);
                break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = (char *)malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    const char *get_token = "GET /";
    char *file_name_start = strstr(request, get_token) + strlen(get_token);
    size_t file_name_len = strchr(file_name_start, ' ') - file_name_start;
    std::string file_name(file_name_start, file_name_len);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    // char file_name[] = "index.html";
    if (file_name == "") {
        file_name = "index.html";
    } else {
        std::string space_encoding = "%20";
        std::string percent_encoding = "%25";

        size_t found = file_name.find(space_encoding);
        while (found != std::string::npos) {
            file_name.replace(found, space_encoding.length(), " ");
            found = file_name.find(space_encoding);
        }

        found = file_name.find(percent_encoding);
        while (found != std::string::npos) {
            file_name.replace(found, percent_encoding.length(), "%");
            found = file_name.find(percent_encoding);
        }
    }

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {

    std::string remoteEnd(".ts");

    if (!(file_name.size() >= 3 &&
          std::equal(remoteEnd.rbegin(), remoteEnd.rend(),
                     file_name.rbegin()))) {
        serve_local_file(client_socket, file_name.c_str());
    } else {
        proxy_remote_file(app, client_socket, request);
    }

    //}
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks
    // like What you need to do (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    std::ifstream file(path);
    if (!file.is_open()) {
        char response[] =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    const char *mime_type_start = nullptr;
    for (int i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '.') {
            mime_type_start = &path[i];
        }
    }

    std::string content_type = "application/octet-stream";
    if (mime_type_start != nullptr) {
        std::string mime_type(mime_type_start + 1);
        if (mime_type == "txt") {
            content_type = "text/plain; charset=UTF-8";
        } else if (mime_type == "html") {
            content_type = "text/html";
        } else if (mime_type == "jpg") {
            content_type = "image/jpeg";
        }
    }
    std::stringstream response;
    response << "HTTP/1.0 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";

    file.seekg(0, std::ios::end);
    int file_length = file.tellg();
    file.seekg(0, std::ios::beg);
    response << "Content-Length: " << file_length << "\r\n\r\n";

    send(client_socket, response.str().c_str(), strlen(response.str().c_str()),
         0);

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        send(client_socket, buffer, file.gcount(), 0);
    }
    file.close();
}

void proxy_remote_file(struct server_app *app, int client_socket,
                       const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    // char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    // send(client_socket, response, strlen(response), 0);

    int remote_socket;
    struct sockaddr_in remote_addr;

    remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(app->remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(app->remote_host);

    int status = connect(remote_socket, (struct sockaddr *)&remote_addr,
                         sizeof(remote_addr));
    if (status == -1) {
        char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    send(remote_socket, request, strlen(request), 0);
    printf("Forwarding Request To Remote Server\n");

    char buff[BUFFER_SIZE];
    int bytesRead = 1024, bytesSent = 1024;
    while (bytesRead > 0 or bytesSent > 0) {
        bytesRead = read(remote_socket, buff, BUFFER_SIZE);
        if (bytesRead < 1) break;

        bytesSent = send(client_socket, buff, bytesRead, 0);
    }
    close(remote_socket);
}