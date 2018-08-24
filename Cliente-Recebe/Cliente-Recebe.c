#include<stdio.h>
#include<winsock2.h>
#include <sys/types.h>
#include<string.h>
#include<locale.h>
#define tam_buffer 1024
#define porta_servidor 10222

int checksum(char *buffer, int tam_msg,char *check){
    unsigned int soma=0,i;
    char ch[3];
    for(i=4;i<tam_msg;i++){
        soma+=buffer[i];
    }
    soma=soma%16;
    if(soma<10){
        soma=soma*10;
    }
    printf("Checksum = %d\n",soma);
    itoa(soma,ch,10);
    check[0]=ch[0];
    check[1]=ch[1];
    return 0;
}

int main(){
    setlocale(LC_ALL,"Portuguese");
    char nome_arquivo[50],porta_cliente_fonte[6];
    FILE *arquivo;
    WSADATA data;
    SOCKET sock_cliente;
    struct sockaddr_in addr_servidor,addr_cliente;
    int addr_tam=sizeof(SOCKADDR),rec_msg_tam,status,pkt_cont,porta_cliente;
    char *buffer,ch[3];
    buffer=(char*)malloc(tam_buffer);
    ///Iniciando sokets em windows
    if(WSAStartup(MAKEWORD(2, 2), &data)!=0){
        printf("Falha WSAStartup\n");
        return -1;
    }
    ///criando socket local
    sock_cliente=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_cliente<0){
        printf("Erro ao iniciar socket\n");
        return -1;
    }
    ///Dados do socket do cliente
    memset(&addr_servidor, 0, sizeof(addr_servidor));
    addr_servidor.sin_family = AF_INET;
    addr_servidor.sin_port = htons(porta_servidor);
    addr_servidor.sin_addr.s_addr = inet_addr("127.0.0.1");
    while(1){
        printf("Insira o nome do arquivo\n");
        scanf("%s",nome_arquivo);
        strcpy(buffer,nome_arquivo);

        if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_servidor, sizeof(addr_servidor)) == SOCKET_ERROR){
            printf("sendto() failed with error code : %d\n" , WSAGetLastError());
            exit(EXIT_FAILURE);

        }
        printf("Aguardando resposta do servidor\n");
        status=0;
        while(1){
            memset(buffer,'\0', tam_buffer);
            if ((rec_msg_tam=recvfrom(sock_cliente, buffer, tam_buffer, 0, (struct sockaddr *) &addr_servidor, &addr_tam)) == SOCKET_ERROR){
                printf("recvfrom() failed with error code : %d\n" , WSAGetLastError());
                exit(EXIT_FAILURE);

            }
            if(buffer[0]!='\0'){
                if(buffer[0]=='1'){
                    buffer[1]='1';
                    strcpy(porta_cliente_fonte,&buffer[1]);
                    status=1;
                    printf("Dados recebidos\n");
                    printf("porta cliente %s\n",porta_cliente_fonte);
                    break;
                }
                else if (buffer[0]=='0'){
                    printf("Arquivo não localizado.\n%s\n",&buffer[1]);
                    break;
                }
            }

        }
        if(status==1){
                printf("Solicitando arquivo no cliente fonte");
                break;
            }
    }
    ///Solicitando arquivo
    ///definindo addr_cliente
    memset(&addr_cliente, 0, sizeof(addr_servidor));
    addr_cliente.sin_family = AF_INET;
    porta_cliente=atoi(porta_cliente_fonte);
    addr_cliente.sin_port = htons(porta_cliente);
    addr_cliente.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(buffer,'\0', tam_buffer);
    strcpy(buffer,nome_arquivo);
    ///nome enviado
    if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, sizeof(addr_cliente)) == SOCKET_ERROR){
        printf("sendto() failed with error code : %d\n" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    while(1){
        memset(buffer,'\0', tam_buffer);
        if ((rec_msg_tam=recvfrom(sock_cliente, buffer, tam_buffer, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) == SOCKET_ERROR){
            printf("recvfrom() failed with error code : %d\n" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }
        if(buffer[0]!='\0'){
            if(strcmp(buffer,"5")==0){
                printf("Arquivo localizado\nIniciando transferencia do arquivo %s\n",nome_arquivo);
                arquivo=fopen(nome_arquivo,"wb");
                break;
            }
            else if (strcmp(buffer,"0Erro-0002")==0){
                printf("Aquivo não localizado\n%s\n",&buffer[1]);
            }
        }
    }
    ///transferindo arquivo
    pkt_cont=0;
    while(1){
        memset(buffer,'\0', tam_buffer);
        if ((rec_msg_tam=recvfrom(sock_cliente, buffer, tam_buffer, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) == SOCKET_ERROR){
            printf("recvfrom() failed with error code : %d\n" , WSAGetLastError());
            exit(EXIT_FAILURE);
        }
        if(buffer[0]!='\0'){
            if(buffer[0]=='1'){
                checksum(buffer,rec_msg_tam,ch);
                if(ch[0]==buffer[2] && ch[1]==buffer[3]){
                    fwrite(&buffer[4],1,rec_msg_tam-4,arquivo);
                    pkt_cont++;
                    strcpy(buffer,"ack1");
                    printf("buffer = %s\n",buffer);
                    if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, sizeof(addr_cliente)) == SOCKET_ERROR){
                        printf("sendto() failed with error code : %d\n" , WSAGetLastError());
                        exit(EXIT_FAILURE);
                    }
                    printf("pacote %d recebido\n",pkt_cont);
                }
                else{
                    strcpy(buffer,"ack0");
                    if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, sizeof(addr_cliente)) == SOCKET_ERROR){
                        printf("sendto() failed with error code : %d\n" , WSAGetLastError());
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if(buffer[0]=='0'){
                checksum(buffer,rec_msg_tam,ch);
                if(ch[0]==buffer[2] && ch[1]==buffer[3]){
                    fwrite(&buffer[4],1,rec_msg_tam-4,arquivo);
                    pkt_cont++;
                    strcpy(buffer,"ack1");
                    if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, sizeof(addr_cliente)) == SOCKET_ERROR){
                        printf("sendto() failed with error code : %d\n" , WSAGetLastError());
                        exit(EXIT_FAILURE);
                    }
                    printf("pacote %d recebido\n",pkt_cont);
                    printf("Transferencia concluida\n");
                    break;
                }
                else{
                    strcpy(buffer,"ack0");
                    if (sendto(sock_cliente, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, sizeof(addr_cliente)) == SOCKET_ERROR){
                        printf("sendto() failed with error code : %d\n" , WSAGetLastError());
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    fclose(arquivo);
system("pause");
}
