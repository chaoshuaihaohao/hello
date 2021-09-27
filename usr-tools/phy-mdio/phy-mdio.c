#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/mii.h>
#include <linux/sockios.h>
#include <linux/types.h>

#define reteck(ret) \
if (ret < 0) { \
	printf("%m! \"%s\" : line: %d\n", __func__, __LINE__); \
	goto lab; \
}

#define help() do {\
printf("phy-mdio:\n"); \
printf("	read operation:	phy-mdio -i <if_name> -n <reg_num>\n"); \
printf("	write operation:phy-mdio -i <if_name> -n <reg_num> -w value\n"); \
printf("For example:\n"); \
printf("	phy-mdio -i eth0 -n 1\n"); \
printf("	phy-mdio -i eth0 -n 0 -w 0x12\n\n"); \
exit(0); \
} while(0);

int sockfd;

char *const short_options = "hi:n:w:";	//有参数的需要跟':'。
struct option long_options[] = {
	{ "help", 0, NULL, 'h' },
	{ "if_name", 1, NULL, 'i' },
	{ "reg_num", 1, NULL, 'n' },
	{ "write", 1, NULL, 'w' },
	{ 0, 0, 0, 0 },
};

int main(int argc, char *argv[])
{
	struct mii_ioctl_data *mii = NULL;
	struct ifreq ifr;
	char if_name[20] = { 0 };
	int reg_num;
	char *value = NULL;
	int ret;
	int c;

	if (argc == 1) {
		help();
	}
	while ((c =
		getopt_long(argc, argv, short_options, long_options,
			    NULL)) != -1) {
		switch (c) {
		case 'h':
			help();
		case 'i':
			//      printf("Interface name is %s\n", optarg);
			strncpy(if_name, optarg, IFNAMSIZ - 1);
			break;
		case 'n':
			//      printf("Register number is %s\n", optarg);
			reg_num = (uint16_t) strtoul(optarg, NULL, 0);
			break;
		case 'w':
			value = optarg;
			printf("value = %s\n", value);
			break;
		}
	}

	memset(&ifr, 0, sizeof(ifr));
	//strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
	sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
	reteck(sockfd);

//get phy address in smi bus
	ret = ioctl(sockfd, SIOCGMIIPHY, &ifr);
	reteck(ret);

	mii = (struct mii_ioctl_data *)&ifr.ifr_data;
	if (!value) {
		mii->reg_num = reg_num;
		ret = ioctl(sockfd, SIOCGMIIREG, &ifr);
		reteck(ret);
		printf("read phy addr: 0x%x; reg: 0x%x ;value : 0x%04x\n\n",
		       mii->phy_id, mii->reg_num, mii->val_out);
	} else {
		printf("value = %s\n", value);
		mii->reg_num = reg_num;
		mii->val_in = (uint16_t) strtoul(value, NULL, 0);
		ret = ioctl(sockfd, SIOCSMIIREG, &ifr);
		reteck(ret);
		printf("write phy addr: 0x%xreg: 0x%xvalue : 0x%x\n\n",
		       mii->phy_id, mii->reg_num, mii->val_in);
	}
lab:
	close(sockfd);
	return 0;
}
