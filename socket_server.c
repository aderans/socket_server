/*
 *  TCP server
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAXPENDING 5

/*
 *  サーバ情報を格納する
 */
struct server_info {
    unsigned short sv_port;
};
typedef struct server_info sv_info_t;

/*!
 * @brief      TCPメッセージを受信する
 * @param[in]  sd  ソケットディスクリプタ
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
tcp_receiver(int serv_sock, char *errmsg)
{
    int exit_flag = 1;
    int recv_msglen = 0;
    char buff[BUFSIZ];
    struct sockaddr_in cl_addr = {0};
    unsigned int cl_addr_len = 0;
    int clnt_sock = 0;

    while(exit_flag){
        /* 入出力パラメータのサイズをセットする */
        cl_addr_len = sizeof(cl_addr);

        /* クライアントが接続するまでブロックして待ち合わせる */
        clnt_sock = accept(serv_sock, (struct sockaddr *)&cl_addr, 
                           &cl_addr_len);
        if(clnt_sock < 0){
            sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
            return(-1);
        }

        /* クライアントが接続してきたら、ここに到達する */
        fprintf(stdout, "[client: %s]\n", inet_ntoa(cl_addr.sin_addr));

        /* クライアントからメッセージを受信する */
        recv_msglen = recv(clnt_sock, buff, BUFSIZ, 0);
        if(recv_msglen < 0){
            sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
            return(-1);
        }

        /* 受信メッセージを標準出力する */
        fprintf(stdout, "message: %s\n", buff);

        /* クライアントからのサーバ終了命令を確認する */
        if(strcmp(buff, "terminate") == 0){
            exit_flag = 0;
        }
    }

    return( 0 );
}

/*!
 * @brief      TCPサーバ実行
 * @param[in]  info   クライアント接続情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
tcp_server(sv_info_t *info, char *errmsg)
{
    struct sockaddr_in sv_addr = {0};
    int sd = 0;
    int rc = 0;

    /* ソケットの生成する */
    sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sd < 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        return(-1);
    }

    /* ローカルのアドレス構造体を作成する */
    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* ワイルドカード指定 */
    sv_addr.sin_port = htons(info->sv_port);

    /* ローカルアドレスへバインドする */
    rc = bind(sd, (struct sockaddr *)&sv_addr, sizeof(sv_addr));
    if(rc != 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        goto close_exit;
    }

    /* TCP接続要求を許可する */
    rc = listen(sd, MAXPENDING);
    if(rc < 0){
        sprintf(errmsg, "(line:%d) %s", __LINE__, strerror(errno));
        goto close_exit;
    }

    /* 文字列を受信する */
    rc = tcp_receiver(sd, errmsg);
    if(rc != 0) goto close_exit;

  close_exit:
    /* ソケットを破棄する */
    if(sd != 0) close(sd);
    return(rc);
}

/*!
 * @brief      初期化処理。待受ポート番号を設定する。
 * @param[in]  argc   コマンドライン引数の数
 * @param[in]  argv   コマンドライン引数
 * @param[out] info   サーバ情報
 * @param[out] errmsg エラーメッセージ格納先
 * @return     成功ならば0、失敗ならば-1を返す。
 */
static int
initialize(int argc, char *argv[], sv_info_t *info, char *errmsg)
{
    if(argc != 2){
        sprintf(errmsg, "Usage: %s <port>\n", argv[0]);
        return(-1);
    }

    memset(info, 0, sizeof(sv_info_t));
    info->sv_port = atoi(argv[1]);

    return(0);
}

/*!
 * @brief   main routine
 * @return  成功ならば0、失敗ならば-1を返す。
 */
int
main(int argc, char *argv[])
{
    int rc = 0;
    sv_info_t info = {0};
    char errmsg[BUFSIZ];

    rc = initialize(argc, argv, &info, errmsg);
    if(rc != 0){
        fprintf(stderr, "Error: %s\n", errmsg);
        return(-1);
    }

    rc = tcp_server(&info, errmsg);
    if(rc != 0){
        fprintf(stderr, "Error: %s\n", errmsg);
        return(-1);
    }

    return(0);
}
