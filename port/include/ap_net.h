#ifndef AP_NET_H
#define AP_NET_H

/*
 * One WebSocket to an Archipelago server.
 * Windows speaks wss:// through WinHTTP. Other builds speak ws:// only.
 * The game thread starts, stops, sends, and polls. A worker thread owns the socket.
 */

/* 1 = thread started, 0 = try again later, -1 = do not retry (bad host, or tls on a build without it). */
int apNetStart(const char *host, int port, int tls);
void apNetStop(void);
int apNetIsOpen(void);
int apNetBusy(void);

/* Queue or send a text frame. 1 if the server will get it. */
int apNetSend(const char *text, int len);

/*
 * Copy one complete inbound text message into buf. Returns its length,
 * 0 when the queue is empty, -1 if the message did not fit (it is dropped).
 */
int apNetPoll(char *buf, int buflen);

/* Copy the next status line from the network thread. 1 if one was waiting. */
int apNetTakeStatus(char *buf, int buflen);

/* 1 if a ReceivedItems message was discarded. The client should Sync. */
int apNetTakeLoss(void);

#endif
