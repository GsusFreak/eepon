/* client-server: test out mailbox and messages */

#include "csim.h"
#include <stdio.h>

#define SIMTIME 10.0
#define NS 2l
#define NC 4l

#define SERV_REQ 1l
#define ACK 2l
#define QUIT 3l
#define QUIT_ACK 4l

typedef struct msg *msg_t;

struct msg {
	MBOX reply;
	long type;
}; 

MBOX serv_mb[NS];

char str[24];
long msg_ct, msg_cur;

void server();
void client();

msg_t new_reply();
msg_t new_send();
void stop_servers();
void my_error();
void del_msg();
void msg_rep();

void sim()
{
	long i;

	create("sim");
	for(i = 0; i < NS; i++)
		serv_mb[i] = mailbox("serv");
	msg_ct = msg_cur = 0;
	for(i = 0; i < NS; i++)
		server(i);
	for(i = 0; i < NC; i++)
		client(i);
	wait(event_list_empty);
	stop_servers();
	msg_rep();
}

void server(n)
long n;
{
	msg_t r;
	msg_t s;
	long type;

	create("server");
	do {
		receive(serv_mb[n], &r);
		hold(1.0);
  		printf("server.%ld sends back to mb %s\n",
			n, mailbox_name(r->reply));
		switch(r->type) {
		case SERV_REQ:
			type = ACK;
			break;
		case QUIT:
			type = QUIT_ACK;
			break;
		default:
			my_error("server %ld: unexpected msg type", n);
		}
		s = new_reply(n, type);
		send(r->reply, s);
		del_msg(r);
	} while(type != QUIT);
}

void client(n)
long n;
{
	MBOX recv;
	msg_t s;
	msg_t r;
	long i;

	create("client");
	sprintf(str, "cli.%ld", n);
	recv = mailbox(str);
	while(clock < SIMTIME) {
		s = new_send(n, SERV_REQ, recv);
		i = random(0l, NS-1);
		printf("client.%ld sends to server.%ld\n",
			n, i);
		send(serv_mb[i], s);
		receive(recv, &r);
		del_msg(r);
		}
	delete_mailbox(recv);
}

void stop_servers()
{
	long i;

	msg_t s;
	msg_t r;
	MBOX recv;

	recv = mailbox("stop");
	for(i = 0; i < NS; i++) {
		s = new_send(i, QUIT, recv);
		send(serv_mb[i], s);
		}
	for(i = 0; i < NS; i++) {
		receive(recv, &r);
		del_msg(r);
		}
	delete_mailbox(recv);
}

void my_error(f, n)
char *f; long n;
{
	printf(f,n);
	printf("\n");
	exit(1);
}

msg_t new_send(n, t, m)
long n, t; MBOX m;
{
	msg_t msg;

	msg = (msg_t) do_malloc(sizeof(struct msg));
	msg->reply = m;
	msg->type = t;
	msg_ct++;
	msg_cur++;
	return(msg);
}

msg_t new_reply(n, t) long n, t;
{
	msg_t msg;

	msg = (msg_t) do_malloc(sizeof(struct msg));
	msg->type = t;
	msg_ct++;
	msg_cur++;
	return(msg);
}

void del_msg(msg)
msg_t msg;
{
	msg_cur--;
	free((char*) msg);
}

void msg_rep()
{
	printf("messages created %4d\n", msg_ct);
	printf("current messages %4d\n", msg_cur);
}
