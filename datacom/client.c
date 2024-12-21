#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // Winsock kutuphanesini bagla

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char message[1024];
    char buffer[1024] = {0};
    int read_size;

    // 1. Winsock baslat
    printf("Winsock baslatiliyor...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Winsock baslatildi.\n");

    // 2. Soket olustur
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket olusturulamadi. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Soket olusturuldu.\n");

    // 3. Sunucu adresini ayarla
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 4. Sunucuya baglan
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Baglanti basarisiz. Hata Kodu: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Sunucuya baglanildi.\n");

    // 5. Surekli mesaj gonder ve yanit al
    while (1) {
        printf("Hava durumu icin sehir adi girin ('cikis' yazip enter ile cikabilirsiniz): ");
        fgets(message, sizeof(message), stdin);

        // Kullanici cikis yapmak isterse
        if (strncmp(message, "cikis", 5) == 0) {
            printf("Baglanti sonlandiriliyor...\n");
            break;
        }

        // Komut formatini ekle
        char formatted_message[1024];
        snprintf(formatted_message, sizeof(formatted_message), "WEATHER:%s", message);

        // Mesaji sunucuya gonder
        send(sock, formatted_message, strlen(formatted_message), 0);

        // Sunucudan yanit al
        read_size = recv(sock, buffer, sizeof(buffer), 0);
        if (read_size > 0) {
            buffer[read_size] = '\0'; // Null-terminate yanit
            printf("Sunucudan gelen yanit: %s\n", buffer);
        } else if (read_size == 0) {
            printf("Sunucu baglantiyi kapatti.\n");
            break;
        } else {
            printf("Sunucudan veri alinirken hata olustu. Hata Kodu: %d\n", WSAGetLastError());
            break;
        }

        // Yanit alindiktan sonra buffer'i temizle
        memset(buffer, 0, sizeof(buffer));
    }

    // 6. Soketi kapat
    closesocket(sock);
    WSACleanup();

    return 0;
}
