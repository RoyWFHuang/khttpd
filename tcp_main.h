#ifndef KTCP_MAIN_H
#define KTCP_MAIN_H

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define DEFAULT_PORT 12345
#define DEFAULT_BACKLOG 1024

int ktcp_init(void);
void ktcp_exit(void);

#endif
