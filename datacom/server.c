#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <curl/curl.h>

#pragma comment(lib, "ws2_32.lib") // Winsock kutuphanesini bagla

#define PORT 8080
#define API_KEY "1af23f30dc14f9f7592a2a66b2990915" // Weatherstack API Anahtariniz

// CURL'den gelen verileri islemek icin bir callback fonksiyonu
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    strncat(data, ptr, size * nmemb);
    return size * nmemb;
}

// Sehir adini temizlemek icin fonksiyon
void trim_city_name(char *city) {
    char *end;

    // Sonundaki bosluklari ve newline karakterlerini kaldir
    end = city + strlen(city) - 1;
    while (end > city && (*end == '\n' || *end == '\r' || *end == ' ')) {
        *end = '\0';
        end--;
    }

    // Basindaki bosluklari kaldir
    while (*city == ' ') {
        memmove(city, city + 1, strlen(city));
    }
}

// JSON yanitini islemeye yarayan fonksiyon
void parse_weather_data(const char *json, char *parsed_data) {
    char temp[32] = {0}, description[128] = {0};

    // JSON'daki verileri ayiklayin
    char *temp_ptr = strstr(json, "\"temperature\":");
    char *desc_ptr = strstr(json, "\"weather_descriptions\":");

    if (temp_ptr && desc_ptr) {
        sscanf(temp_ptr + 14, "%[^,]", temp);
        char *desc_start = strchr(desc_ptr + 23, '\"');
        char *desc_end = strchr(desc_start + 1, '\"');
        strncpy(description, desc_start + 1, desc_end - desc_start - 1);

        snprintf(parsed_data, 256, "Sicaklik: %s\u00B0C, Durum: %s", temp, description);
    } else {
        snprintf(parsed_data, 256, "Hava durumu bilgisi islenemedi. Yanit: %s", json);
    }
}

// Weatherstack API'den hava durumu bilgisi al
void get_weather_info(const char *city, char *weather_data) {
    CURL *curl;
    CURLcode res;
    char url[256];
    char buffer[1024] = {0};

    // URL'yi olustur
    snprintf(url, sizeof(url),
             "http://api.weatherstack.com/current?access_key=%s&query=%s", API_KEY, city);

    printf("API URL: %s\n", url); // Debug: URL'yi yazdir

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            printf("API Yanit: %s\n", buffer); // Debug: Yaniti yazdir
            parse_weather_data(buffer, weather_data);
        } else {
            snprintf(weather_data, 1024, "Hava durumu alinmadi. Hata: %s", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    } else {
        snprintf(weather_data, 1024, "CURL baslatilamadi.");
    }
}

// Istemci isleme fonksiyonu
DWORD WINAPI handle_client(void *socket_desc) {
    SOCKET client_socket = *(SOCKET *)socket_desc;
    char buffer[1024] = {0};
    char response[1024] = {0};
    int read_size;

    // Baglanti acikken surekli mesajlari dinle
    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0'; // Gelen mesaji null-terminate yap
        printf("Istemciden gelen mesaj: %s\n", buffer);

        // Komut kontrolu
        if (strncmp(buffer, "WEATHER:", 8) == 0) {
            // Sehir adini al
            char city[64] = {0};
            sscanf(buffer + 8, "%63[^\n]", city); // Sehir adini cikar
            trim_city_name(city);                // Sehir adini temizle
            printf("Islenen sehir: '%s'\n", city); // Debug: Sehir adini yazdir

            // Hava durumu bilgisini al
            get_weather_info(city, response);
        } else {
            snprintf(response, sizeof(response), "Bilinmeyen komut: %s", buffer);
        }

        // Yanit gonder
        send(client_socket, response, strlen(response), 0);
        printf("Yanit gonderildi: %s\n", response);

        memset(buffer, 0, sizeof(buffer));  // Buffer'i sifirla
        memset(response, 0, sizeof(response));  // Yaniti sifirla
    }

    if (read_size == 0) {
        printf("Istemci baglantiyi sonlandirdi.\n");
    } else if (read_size == SOCKET_ERROR) {
        printf("Istemci ile iletisimde hata olustu. Hata Kodu: %d\n", WSAGetLastError());
    }

    // Istemci soketini kapat
    closesocket(client_socket);
    free(socket_desc);

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, client_socket;
    struct sockaddr_in server, client;
    int c;
    SOCKET *new_sock;

    // Winsock baslat
    printf("Winsock baslatiliyor...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock baslatildi.\n");

    // Soket olustur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket olusturulamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Soket olusturuldu.\n");

    // Sunucu adresi ve portu tanimla
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Soketi bagla
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind basarisiz. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Baglanti noktasi olusturuldu.\n");

    // Dinlemeye basla
    listen(server_fd, 5);
    printf("Dinleme baslatildi. Port: %d\n", PORT);

    // Gelen baglantilari kabul et
    c = sizeof(struct sockaddr_in);
    while ((client_socket = accept(server_fd, (struct sockaddr *)&client, &c)) != INVALID_SOCKET) {
        printf("Yeni bir istemci baglandi.\n");

        // Yeni soket icin bellek ayir
        new_sock = malloc(sizeof(SOCKET));
        *new_sock = client_socket;

        // Yeni bir is parcacigi olustur ve istemciyi isle
        if (CreateThread(NULL, 0, handle_client, (void *)new_sock, 0, NULL) == NULL) {
            printf("Is parcacigi olusturulamadi.\n");
            closesocket(client_socket);
            free(new_sock);
        }
    }

    if (client_socket == INVALID_SOCKET) {
        printf("Baglanti kabul edilemedi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }

    // Winsock'u kapat
    closesocket(server_fd);
    WSACleanup();

    return 0;
}
