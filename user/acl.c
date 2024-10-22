#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/stat.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define MAX_INFO_LEN 256
#define MAX_DIM_NUM	24
#define MAX_TABLE_NUM	4

#define ys_err(f, arg...) \
        fprintf(stderr, "ERR:[%s:%d]: " f "\n", __func__, __LINE__, ##arg)

#define ys_info(f, arg...) \
	printf("INFO:[%s:%d]:" f "\n", __func__, __LINE__, ##arg)

#define ys_warn(f, arg...) \
        printf("WARN:[%s:%d]:" f "\n", __func__, __LINE__, ##arg)

#define ys_debug(f, arg...) \
//	printf("DBG:[%s:%d]:" f "\n", __func__, __LINE__, ##arg)

#define DEFAULT_USER_TABLE_PATH		"/tmp/user"
#define DEFAULT_RELAY_TABLE_PATH	"/tmp/relay"

int trans_ipv6_prefix(const u32 prefix, u32 * mask);
int ipv6To16BitSegments(const char *ipv6str, u32 * segments);

struct user_rule {
	char sip[MAX_INFO_LEN];
	char dip[MAX_INFO_LEN];
	char sport[20];
	char dport[20];
	u32 sip_mask;
	u32 dip_mask;
};

struct relay_rule {
	u32 sip[4];
	u32 dip[4];
	u32 sip_mask[4];
	u32 dip_mask[4];
	u16 sport_start;
	u16 sport_end;
	u16 dport_start;
	u16 dport_end;
};

struct rule {
	u8 protocol;
	u32 protocol_mask;
	u16 action;
	struct user_rule user;
	struct relay_rule relay;
};

struct user_table {
	int dim;
	int table_index;
	struct rule rule[3000];
	int rule_num;
};

struct user_command {
	char *exe_path;
	char *device;
	int opt_type;
	int table_num;
	int ip_type;
	char *chain;
	char *jump;
	int table_index;
	char *rule_index_str;
	int rule_index;
	char *protocol_str;
	struct rule rule;
};

static struct option ys_fme_long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "init", no_argument, 0, 0 },
	{ "save", no_argument, 0, 0 },
	{ "restore", no_argument, 0, 0 },
	{ "device", required_argument, 0, 'i' },
	{ "protocol", required_argument, 0, 'p' },
	{ "ipv4", no_argument, 0, '4' },
	{ "ipv6", no_argument, 0, '6' },
	{ "src", required_argument, 0, 's' },
	{ "destination", required_argument, 0, 'd' },
	{ "dst", required_argument, 0, 'd' },
	{ "sport", required_argument, 0, 0 },
	{ "dport", required_argument, 0, 0 },
	{ "table", required_argument, 0, 't' },
	{ "append", required_argument, 0, 'A' },
	{ "delete", required_argument, 0, 'D' },
	{ "rule-number", required_argument, 0, 0 },
//      { "check", required_argument, 0, 'C' },
	{ "insert", required_argument, 0, 'I' },
	{ "replace", required_argument, 0, 'R' },
	{ "list", optional_argument, 0, 'L' },
	{ "list-rules", optional_argument, 0, 'S' },
	{ "flush", optional_argument, 0, 'F' },
	{ "zero", optional_argument, 0, 'Z' },
	{ "replace", required_argument, 0, 'R' },
	{ "log-level", required_argument, 0, 0 },
	{ "help", optional_argument, 0, 'h' },

	{ 0, 0, 0, 0 }
};

enum ys_fme_opt_type {
	YS_FME_INIT,
	YS_FME_UNINIT,
	YS_FME_RUNTME_INSERT_RULE,
	YS_FME_RUNTME_APPEND_RULE,
	YS_FME_RUNTME_DELETE_RULE,
	YS_FME_RUNTME_REPLACE_RULE,
	YS_FME_RUNTME_READ_TABLE,
	YS_FME_SAVE,
//      YS_FME_RUNTME_ADD_TABLE,
//      YS_FME_RUNTME_DELETE_TABLE,
//      YS_FME_RUNTME_REPLACE_TABLE,
//      YS_FME_USER_ACTION,
//      YS_FME_SELFTEST,
//      YS_FME_OTHER,
	NUMBER_OF_CMD
};

#define NUMBER_OF_OPT		30
/* Table of legal combinations of commands and options.  If any of the
 * given commands make an option legal, that option is legal (applies to
 * CMD_LIST and CMD_ZERO only).
 * Key:
 *  +  compulsory
 *  x  illegal
 *     optional
 */
static const char commands_v_options[NUMBER_OF_CMD][NUMBER_OF_OPT] = {
//            4    6    A:   C:   d:   D:   h    i:   I:   j:   L::  R:   p:   s:  sport dport init num
/*INIT*/   { 'x', 'x', 'x', 'x', 'x', 'x', ' ', '+', 'x', 'x', 'x', 'x', 'x', 'x' , 'x', 'x', '+', 'x' },
/*UNINIT*/ { 'x', 'x', 'x', 'x', 'x', 'x', ' ', '+', 'x', 'x', 'x', 'x', 'x', 'x' , 'x', 'x', '+', 'x' },
/*INSERT*/ { ' ', ' ', 'x', 'x', ' ', 'x', ' ', '+', '+', '+', 'x', 'x', ' ', ' ' , ' ', ' ', 'x', 'x' },
/*APPEND*/ { ' ', ' ', '+', 'x', ' ', 'x', ' ', '+', 'x', '+', 'x', 'x', ' ', ' ' , ' ', ' ', 'x', 'x' },
/*DELETE*/ { ' ', ' ', 'x', 'x', 'x', '+', ' ', '+', 'x', 'x', 'x', 'x', 'x', 'x' , 'x', 'x', 'x', 'x' },
/*REPLACE*/{ ' ', ' ', 'x', 'x', ' ', 'x', ' ', '+', 'x', '+', 'x', '+', ' ', ' ' , ' ', ' ', 'x', 'x' },
/*READ*/   { ' ', ' ', 'x', 'x', 'x', 'x', ' ', '+', 'x', 'x', '+', 'x', 'x', 'x' , 'x', 'x', 'x', 'x' },
};

//46A:C:d:D:hi:I:j:L::R:p:s:sport:dport:init rule_number
enum opt_str {
	OPT_4,
	OPT_6,
	OPT_A,
	OPT_C,
	OPT_d,
	OPT_D,
	OPT_h,
	OPT_i,
	OPT_I,
	OPT_j,
	OPT_L,
	OPT_R,
	OPT_p,
	OPT_s,
	OPT_sport,
	OPT_dport,
	OPT_init,
	OPT_rule_number,
};

bool commands_options[NUMBER_OF_OPT] = { 0 };

static int generic_command_check(int opt_type)
{
	for (int i = 0; i < NUMBER_OF_CMD; i++) {
		if (i != opt_type)
			continue;
		for (int j = 0; j < NUMBER_OF_OPT; j++) {
			if (commands_v_options[i][j] == '+') {
				if (commands_options[j] == false) {
					ys_err("cmd %d option %d not set", i,
					       j);
					return -1;
				}
			} else if (commands_v_options[i][j] == 'x') {
				if (commands_options[j] == true) {
					ys_err("Invalid cmd %d option %d", i,
					       j);
					return -1;
				}
			}
		}
	}
	return 0;
}

char **allocate_string_array(int rows, int cols)
{
	char **array = calloc(rows, sizeof(char *));
	if (array != NULL) {
		for (int i = 0; i < rows; i++) {
			array[i] = calloc(cols, sizeof(char));
			if (array[i] == NULL) {
				// 处理内存分配失败的情况，释放之前分配的内存  
				while (i > 0) {
					free(array[i - 1]);
					i--;
				}
				free(array);
				return NULL;
			}
		}
	}
	return array;
}

void free_string_array(char **array, int rows)
{
	if (array != NULL) {
		for (int i = 0; i < rows; i++) {
			free(array[i]);
		}
		free(array);
	}
}

static void ys_str_copy(char *dst, const char *src)
{
	memcpy(dst, src, strlen(src));
	dst[strlen(src)] = '\0';
}

static int ys_strtoken_dim(char *line, char **str)
{
	int dim = 0;
	char *token;
	const char delimiters[] = "[,]\n\r";	// 定义分隔符，包括逗号和右方括号

	// 获取第一个token
	token = strtok(line, delimiters);
	while (token != NULL) {
		//ys_debug("token %d:%s", dim, token);
		ys_str_copy(str[dim], token);
		dim++;
		// 获取下一个token
		token = strtok(NULL, delimiters);
	}
	return dim;
}

static int ys_strtoken_user_rule(char *line, char **str)
{
	int dim = 0;
	char *token;
	const char delimiters[] = "/ \n\r\t";	// 定义分隔符，包括逗号和右方括号

	// 获取第一个token
	token = strtok(line, delimiters);
	while (token != NULL) {
		//      ys_debug("token %d:%s", dim, token);
		ys_str_copy(str[dim], token);
		dim++;
		// 获取下一个token
		token = strtok(NULL, delimiters);
	}
	return dim;
}

static int ys_strtoken_relay_rule(char *line, char **str)
{
	int dim = 0;
	char *token;
	const char delimiters[] = "=/ \n\r";	// 定义分隔符，包括逗号和右方括号

	// 获取第一个token
	token = strtok(line, delimiters);
	while (token != NULL) {
		if (strncmp(token, "rule", strlen("rule")) == 0) {
			token = strtok(NULL, delimiters);
			continue;
		}
		//ys_debug("token %d:%s", dim, token);
		ys_str_copy(str[dim], token);
		dim++;
		// 获取下一个token
		token = strtok(NULL, delimiters);
	}
	return dim;
}

int get_info_of_user_table(const char *filename, int *table_index, int *dim)
{
	char line[MAX_INFO_LEN];
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		ys_err("Error opening file %s", filename);
		return -1;
	}

	char **str = allocate_string_array(MAX_DIM_NUM, MAX_INFO_LEN);
	if (str == NULL) {
		ys_err("Failed to alloc str");
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "table", strlen("table")) == 0) {
			ys_debug("Read line: %s", line);      // 调试输出
			int result = sscanf(line,
					    "table:%d", table_index);
			if (result != 1) {
				ys_err
				    ("Error reading table index, result %d",
				     result);
				fclose(fp);
				return -1;
			}
		} else if (strncmp(line, "[protocol", strlen("[protocol"))
			   == 0) {
			ys_debug("Read line: %s", line);      // 调试输出
			*dim = ys_strtoken_dim(line, str);
		}
	}

	for (int i = 0; i < *dim; i++) {
		if (strlen(str[i])) {
			ys_debug("dim %d: %s", i, str[i]);
		}
	}

	free_string_array(str, MAX_DIM_NUM);
	fclose(fp);
	return 0;
}

int get_rule_of_user_table(const char *filename, struct rule *rule,
			   int *rule_num)
{
	char line[256];
	int dim = 0;
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		ys_err("Error opening file %s", filename);
		return -1;
	}
	char **str = allocate_string_array(MAX_DIM_NUM, MAX_INFO_LEN);
	if (str == NULL) {
		ys_err("Failed to alloc str");
		return -1;
	}

	int id = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp(line, "[", 1) == 0)
			continue;
                ys_debug("Read line: %s", line);        // 调试输出
		dim = ys_strtoken_user_rule(line, str);
		switch (dim) {
		case 7:	//ipv4 ipv6
			rule[id].protocol = strtoul(str[0], NULL, 16);
			rule[id].protocol_mask = atoi(str[1]);
			ys_str_copy(rule[id].user.sip, str[2]);
			rule[id].user.sip_mask = atoi(str[3]);
			ys_str_copy(rule[id].user.dip, str[4]);
			rule[id].user.dip_mask = atoi(str[5]);
			rule[id].action = atoi(str[6]);
			id++;
			break;
		case 9:	//tcp-dup/ipv4 ipv6
			rule[id].protocol = strtoul(str[0], NULL, 16);
			rule[id].protocol_mask = atoi(str[1]);
			ys_str_copy(rule[id].user.sip, str[2]);
			rule[id].user.sip_mask = atoi(str[3]);
			ys_str_copy(rule[id].user.dip, str[4]);
			rule[id].user.dip_mask = atoi(str[5]);
			ys_str_copy(rule[id].user.sport, str[6]);
			ys_str_copy(rule[id].user.dport, str[7]);
			rule[id].action = atoi(str[8]);
			id++;
			break;
		default:
			//ys_debug("Unsupport dim %d", dim);
			break;
		}
	}

	*rule_num = id;
#if 0
	ys_debug("rule num %d", id);
	for (int i = 0; i < id; i++) {
		ys_debug
		    ("0x%02x/%d\t%15s/%-2d\t%15s/%-2d\t%s\t%s\t\t%d",
		     rule[i].protocol, rule[i].protocol_mask,
		     rule[i].user.sip, rule[i].user.sip_mask,
		     rule[i].user.dip, rule[i].user.dip_mask,
		     rule[i].user.sport, rule[i].user.dport, rule[i].action);

	}
#endif
	free_string_array(str, MAX_DIM_NUM);
	fclose(fp);
	return 0;
}

int parse_user_table(const char *filename, struct user_table *user)
{
	int ret;

	ret = get_info_of_user_table(filename, &user->table_index, &user->dim);
	if (ret)
		return ret;
	ret = get_rule_of_user_table(filename, user->rule, &user->rule_num);
	if (ret)
		return ret;

	return ret;
}

static void trans_user_protocol(struct rule *rule, const char *protocol_str)
{
	char protocol[MAX_INFO_LEN];

	if (!protocol_str)
		rule->protocol_mask = 32;
	else {
		sscanf(protocol_str, "%[^/]/%u", protocol,
		       &rule->protocol_mask);
		if (strcasecmp(protocol, "TCP") == 0) {
			rule->protocol = IPPROTO_TCP;
		} else if (strcasecmp(protocol, "UDP") == 0) {
			rule->protocol = IPPROTO_UDP;
		} else if (strcasecmp(protocol, "ICMP") == 0) {
			rule->protocol = IPPROTO_ICMP;
		} else {
			ys_err("Unsupport protocol %s", protocol);
		}
	}

	//ys_debug("Protocol 0x%02x/%u", rule->protocol, rule->protocol_mask);

}

int trans_user_ip(struct rule *rule, int ip_type)
{
	char ip[MAX_INFO_LEN];
	int ip_mask = 0;
	int result;

	if (ip_type != AF_INET6) {
		if (!strlen(rule->user.sip)) {
			rule->user.sip_mask = 32;
			rule->relay.sip_mask[0] = 32;
		} else {
			//ys_debug("input sip = %s", rule->user.sip);
			// 使用 sscanf 解析 IP 地址和子网掩码
			result =
			    sscanf(rule->user.sip, "%[^/]/%d", ip, &ip_mask);
			if (result == 2 || result == 1) {
				// 解析成功，cmd.ip 包含 IP 地址，cmd.ip_mask 包含子网掩码
			} else {
				// 解析失败
				ys_err("Failed to parse IP address and mask.");
			}
			ys_str_copy(rule->user.sip, ip);
			if (ip_mask) {
				rule->relay.sip_mask[0] = rule->user.sip_mask =
				    ip_mask;
			}
			struct in_addr sip;
			if (inet_pton(AF_INET, rule->user.sip, &sip) != 1) {
				ys_err("inet_pton");
				return -1;
			}
			rule->relay.sip[0] = sip.s_addr;
		}
		if (!strlen(rule->user.dip)) {
			rule->user.dip_mask = 32;
			rule->relay.dip_mask[0] = 32;
		} else {
			//ys_debug("input dip = %s", rule->user.dip);
			// 使用 sscanf 解析 IP 地址和子网掩码
			result =
			    sscanf(rule->user.dip, "%[^/]/%d", ip, &ip_mask);
			if (result == 2 || result == 1) {
				// 解析成功，cmd.ip 包含 IP 地址，cmd.ip_mask 包含子网掩码
			} else {
				// 解析失败
				ys_err("Failed to parse IP address and mask.");
			}
			ys_str_copy(rule->user.dip, ip);
			if (ip_mask) {
				rule->relay.dip_mask[0] = rule->user.dip_mask =
				    ip_mask;
			}
			ys_debug("dst IP: %s, Mask: %d", rule->user.dip,
				 rule->user.dip_mask);
			struct in_addr dip;
			if (inet_pton(AF_INET, rule->user.dip, &dip) != 1) {
				ys_err("inet_pton");
				return -1;
			}
			rule->relay.dip[0] = dip.s_addr;
		}
	} else {
		if (!strlen(rule->user.sip)) {
			rule->user.sip_mask = 128;
			trans_ipv6_prefix(rule->user.sip_mask,
					  rule->relay.sip_mask);
		} else {
			//ys_debug("input sip = %s", rule->user.sip);
			// 使用 sscanf 解析 IP 地址和子网掩码
			result =
			    sscanf(rule->user.sip, "%[^/]/%d", ip, &ip_mask);
			if (result == 2 || result == 1) {
				// 解析成功，cmd.ip 包含 IP 地址，cmd.ip_mask 包含子网掩码
			} else {
				// 解析失败
				ys_err("Failed to parse IP address and mask.");
			}
			ys_str_copy(rule->user.sip, ip);
			if (ip_mask) {
				rule->user.sip_mask = ip_mask;
				trans_ipv6_prefix(ip_mask,
						  rule->relay.sip_mask);
			}
			ipv6To16BitSegments(rule->user.sip, rule->relay.sip);
		}
		if (!strlen(rule->user.dip)) {
			rule->user.dip_mask = 128;
			trans_ipv6_prefix(rule->user.dip_mask,
					  rule->relay.dip_mask);
		} else {
			//ys_debug("input dip = %s", rule->user.dip);
			// 使用 sscanf 解析 IP 地址和子网掩码
			result =
			    sscanf(rule->user.dip, "%[^/]/%d", ip, &ip_mask);
			if (result == 2 || result == 1) {
				// 解析成功，cmd.ip 包含 IP 地址，cmd.ip_mask 包含子网掩码
			} else {
				// 解析失败
				ys_err("Failed to parse IP address and mask.");
			}
			ys_str_copy(rule->user.dip, ip);
			if (ip_mask) {
				rule->user.dip_mask = ip_mask;
				trans_ipv6_prefix(ip_mask,
						  rule->relay.dip_mask);
			}
			ipv6To16BitSegments(rule->user.dip, rule->relay.dip);
		}
		ys_debug("relay src maks %d %d %d %d", rule->relay.sip_mask[0],
			rule->relay.sip_mask[1], rule->relay.sip_mask[2],
			rule->relay.sip_mask[3]);
		ys_debug("relay dst maks %d %d %d %d", rule->relay.dip_mask[0],
			rule->relay.dip_mask[1], rule->relay.dip_mask[2],
			rule->relay.dip_mask[3]);
	}
	return 0;
}

int trans_user_table_ip(struct rule *rule)
{
	if (!strlen(rule->user.sip))
		rule->user.sip_mask = 32;
	else {
		struct in_addr sip;
		if (inet_pton(AF_INET, rule->user.sip, &sip) != 1) {
			ys_err("inet_pton");
			return -1;
		}
		rule->relay.sip[0] = sip.s_addr;
	}
	if (!strlen(rule->user.dip))
		rule->user.dip_mask = 32;
	else {
		struct in_addr dip;
		if (inet_pton(AF_INET, rule->user.dip, &dip) != 1) {
			ys_err("inet_pton");
			return -1;
		}
		rule->relay.dip[0] = dip.s_addr;
	}
	return 0;
}

static void trans_user_port(struct rule *rule)
{
		if (strlen(rule->user.sport)) {
			sscanf(rule->user.sport, "%hu:%hu",
			       &rule->relay.sport_start,
			       &rule->relay.sport_end);
			if (rule->relay.sport_end == 0)
				rule->relay.sport_end = rule->relay.sport_start;
		} else {
			rule->relay.sport_start = 0;
			rule->relay.sport_end = 65535;
		}
		if (strlen(rule->user.dport)) {
			sscanf(rule->user.dport, "%hu:%hu",
			       &rule->relay.dport_start,
			       &rule->relay.dport_end);
			if (rule->relay.dport_end == 0)
				rule->relay.dport_end = rule->relay.dport_start;
		} else {
			rule->relay.dport_start = 0;
			rule->relay.dport_end = 65535;
		}
}

// 将整数转换为二进制字符串
void decimalToBinary(unsigned int value, char *binaryStr)
{
	for (int i = 0; i < 5; i++) {
		binaryStr[i] = (value & 1) ? '1' : '0';
		value >>= 1;
	}
}

void decimalToBinary_ipv6(unsigned int value, char *binaryStr)
{
	for (int i = 0; i < 11; i++) {
		binaryStr[i] = (value & 1) ? '1' : '0';
		value >>= 1;
	}
}

// 将 IPv6 string 转换为 8 个 16 位的无符号整数数组
int ipv6To16BitSegments(const char *ipv6str, u32 *segments)
{
	struct in6_addr addr;
	if (inet_pton(AF_INET6, ipv6str, &addr) != 1) {
		ys_err("Invalid IPv6 string:%s", ipv6str);
		return -1;	// 转换失败
	}
	// 将 IPv6 地址复制到 segments 数组
	memcpy(segments, addr.s6_addr, 16);

	// 将每个 8 字节段转换为 16 位整数
	for (int i = 0; i < 4; i++) {
		segments[i] =
		    (addr.s6_addr[4 * i + 3] << 24) | (addr.s6_addr[4 * i +
								    2] << 16)
		    | (addr.s6_addr[4 * i + 1] << 8) | addr.s6_addr[4 * i];
	}
	return 0;		// 转换成功
}

int trans_ipv6(const char *ipv6str, u32 *segments)
{
	if (ipv6To16BitSegments(ipv6str, segments) == 0) {
#if 0
		// 打印转换后的 16 位段和掩码
		ys_debug("ipv6str [%s], segment[%08x:%08x:%08x:%08x]",
			 ipv6str, segments[0], segments[1], segments[2],
			 segments[3]);
#endif
	} else {
		ys_err("Invalid IPv6 address.");
	}

	return 0;
}

int trans_ipv6_prefix(const u32 prefix, u32 *mask)
{
	int i;
	for (i = 0; i < prefix / 32; i++) {
		mask[i] = 32;
	}
	mask[i] = prefix % 32;
	//ys_debug("mask %u %u %u %u", mask[0], mask[1], mask[2], mask[3]);

	return 0;
}

static void compute_mask_type(FILE *fp, int dim)
{
	char binaryStr[24] = { 0 };	// 32位二进制数 + null终止符
	u32 mask_type;

	switch (dim) {
	case 3:
		fprintf(fp, "mask_type=000\n");
		break;
	case 5:
		mask_type = 0x18;
		decimalToBinary(mask_type, binaryStr);
		fprintf(fp, "mask_type=%c%c%c%c%c\n", binaryStr[0],
			binaryStr[1], binaryStr[2], binaryStr[3], binaryStr[4]);
		break;
	case 9:
		fprintf(fp, "mask_type=000000000\n");
		break;
	case 11:
		mask_type = 0x600;
		decimalToBinary_ipv6(mask_type, binaryStr);
		fprintf(fp, "mask_type=%c%c%c%c%c%c%c%c%c%c%c\n",
			binaryStr[0], binaryStr[1], binaryStr[2], binaryStr[3],
			binaryStr[4], binaryStr[5], binaryStr[6], binaryStr[7],
			binaryStr[8], binaryStr[9], binaryStr[10]);
		break;
	}
}

int trans_user2ys_ipv6(struct user_table *user, struct user_command *cmd)
{
	char relay_path[MAX_INFO_LEN];

	mkdir("/tmp/relay", 0755);
	sprintf(relay_path, "/tmp/relay/table%d", user->table_index);

	FILE *fp = fopen(relay_path, "w");
	if (fp == NULL) {
		ys_err("Error opening file %s", relay_path);
		return -1;
	}

	fprintf(fp, "[table_info]\n");
	fprintf(fp, "table_index=%d\n", user->table_index);
	fprintf(fp, "dimension_select=%d\n", user->dim);	//TODO:

	compute_mask_type(fp, user->dim);
	fprintf(fp, "rule_num=%d\n\n", user->rule_num);
	ys_debug("rule_num %d", user->rule_num);

	for (int i = 0; i < user->rule_num; i++) {
		fprintf(fp, "[rule%d]\n", i);
		fprintf(fp, "action_index=%u\n", user->rule[i].action);

		char sip_str[MAX_INFO_LEN];
		char dip_str[MAX_INFO_LEN];
		sscanf(user->rule[i].user.sip, "%[^/]/%u", sip_str,
		       &user->rule[i].user.sip_mask);
		trans_ipv6(sip_str, user->rule[i].relay.sip);
		trans_ipv6_prefix(user->rule[i].user.sip_mask,
				  user->rule[i].relay.sip_mask);
		sscanf(user->rule[i].user.dip, "%[^/]/%u", dip_str,
		       &user->rule[i].user.dip_mask);
		trans_ipv6(dip_str, user->rule[i].relay.dip);
		trans_ipv6_prefix(user->rule[i].user.dip_mask,
				  user->rule[i].relay.dip_mask);

		trans_user_port(&user->rule[i]);
		if (strlen(user->rule[i].user.sport)
		    || strlen(user->rule[i].user.dport)) {
			fprintf(fp,
				"rule=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%04x%04x/0 0x%04x%04x/0\n",
				user->rule[i].protocol,
				user->rule[i].protocol_mask,
				user->rule[i].relay.sip[0],
				user->rule[i].relay.sip_mask[0],
				user->rule[i].relay.sip[1],
				user->rule[i].relay.sip_mask[1],
				user->rule[i].relay.sip[2],
				user->rule[i].relay.sip_mask[2],
				user->rule[i].relay.sip[3],
				user->rule[i].relay.sip_mask[3],
				user->rule[i].relay.dip[0],
				user->rule[i].relay.dip_mask[0],
				user->rule[i].relay.dip[1],
				user->rule[i].relay.dip_mask[1],
				user->rule[i].relay.dip[2],
				user->rule[i].relay.dip_mask[2],
				user->rule[i].relay.dip[3],
				user->rule[i].relay.dip_mask[3],
				user->rule[i].relay.sport_start,
				user->rule[i].relay.sport_end,
				user->rule[i].relay.dport_start,
				user->rule[i].relay.dport_end);
			ys_debug
			    ("rule[%d]=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%04x%04x/0 0x%04x%04x/0",
			     i, user->rule[i].protocol,
			     user->rule[i].protocol_mask,
			     user->rule[i].relay.sip[0],
			     user->rule[i].relay.sip_mask[0],
			     user->rule[i].relay.sip[1],
			     user->rule[i].relay.sip_mask[1],
			     user->rule[i].relay.sip[2],
			     user->rule[i].relay.sip_mask[2],
			     user->rule[i].relay.sip[3],
			     user->rule[i].relay.sip_mask[3],
			     user->rule[i].relay.dip[0],
			     user->rule[i].relay.dip_mask[0],
			     user->rule[i].relay.dip[1],
			     user->rule[i].relay.dip_mask[1],
			     user->rule[i].relay.dip[2],
			     user->rule[i].relay.dip_mask[2],
			     user->rule[i].relay.dip[3],
			     user->rule[i].relay.dip_mask[3],
			     user->rule[i].relay.sport_start,
			     user->rule[i].relay.sport_end,
			     user->rule[i].relay.dport_start,
			     user->rule[i].relay.dport_end);
		} else {
			fprintf(fp,
				"rule=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u\n",
				user->rule[i].protocol,
				user->rule[i].protocol_mask,
				user->rule[i].relay.sip[0],
				user->rule[i].relay.sip_mask[0],
				user->rule[i].relay.sip[1],
				user->rule[i].relay.sip_mask[1],
				user->rule[i].relay.sip[2],
				user->rule[i].relay.sip_mask[2],
				user->rule[i].relay.sip[3],
				user->rule[i].relay.sip_mask[3],
				user->rule[i].relay.dip[0],
				user->rule[i].relay.dip_mask[0],
				user->rule[i].relay.dip[1],
				user->rule[i].relay.dip_mask[1],
				user->rule[i].relay.dip[2],
				user->rule[i].relay.dip_mask[2],
				user->rule[i].relay.dip[3],
				user->rule[i].relay.dip_mask[3]);
			ys_debug
			    ("rule[%d]=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%08x/%02u",
			     i, user->rule[i].protocol,
			     user->rule[i].protocol_mask,
			     user->rule[i].relay.sip[0],
			     user->rule[i].relay.sip_mask[0],
			     user->rule[i].relay.sip[1],
			     user->rule[i].relay.sip_mask[1],
			     user->rule[i].relay.sip[2],
			     user->rule[i].relay.sip_mask[2],
			     user->rule[i].relay.sip[3],
			     user->rule[i].relay.sip_mask[3],
			     user->rule[i].relay.dip[0],
			     user->rule[i].relay.dip_mask[0],
			     user->rule[i].relay.dip[1],
			     user->rule[i].relay.dip_mask[1],
			     user->rule[i].relay.dip[2],
			     user->rule[i].relay.dip_mask[2],
			     user->rule[i].relay.dip[3],
			     user->rule[i].relay.dip_mask[3]);
		}
	}

	fclose(fp);
	return 0;
}

int trans_user2ys(struct user_table *user, struct user_command *cmd)
{
	char relay_path[MAX_INFO_LEN];
	int ret = 0;

	mkdir("/tmp/relay", 0755);
	sprintf(relay_path, "/tmp/relay/table%d", user->table_index);
	ys_debug("relay path:%s", relay_path);

	FILE *fp = fopen(relay_path, "w");
	if (fp == NULL) {
		ys_err("Error opening file %s", relay_path);
		return -1;
	}

	fprintf(fp, "[table_info]\n");
	fprintf(fp, "table_index=%d\n", user->table_index);
	fprintf(fp, "dimension_select=%d\n", user->dim);

	compute_mask_type(fp, user->dim);

	fprintf(fp, "rule_num=%d\n\n", user->rule_num);
	ys_debug("rule_num %d", user->rule_num);

	for (int i = 0; i < user->rule_num; i++) {
		fprintf(fp, "[rule%d]\n", i);
		fprintf(fp, "action_index=%u\n", user->rule[i].action);

		ret = trans_user_table_ip(&user->rule[i]);
		if (ret)
			goto err;
		user->rule[i].relay.sip_mask[0] = user->rule[i].user.sip_mask;
		user->rule[i].relay.dip_mask[0] = user->rule[i].user.dip_mask;
		ys_debug("sip_mask %d, dip_mask %d",
			 user->rule[i].user.sip_mask,
			 user->rule[i].user.dip_mask);
		trans_user_port(&user->rule[i]);

		if (strlen(user->rule[i].user.sport)
		    || strlen(user->rule[i].user.dport)) {
			fprintf(fp,
				"rule=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%04x%04x/0 0x%04x%04x/0\n",
				user->rule[i].protocol,
				user->rule[i].protocol_mask,
				user->rule[i].relay.sip[0],
				user->rule[i].relay.sip_mask[0],
				user->rule[i].relay.dip[0],
				user->rule[i].relay.dip_mask[0],
				user->rule[i].relay.sport_start,
				user->rule[i].relay.sport_end,
				user->rule[i].relay.dport_start,
				user->rule[i].relay.dport_end);

			ys_debug(
				"rule=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u 0x%04x%04x/0 0x%04x%04x/0",
				user->rule[i].protocol,
				user->rule[i].protocol_mask,
				user->rule[i].relay.sip[0],
				user->rule[i].relay.sip_mask[0],
				user->rule[i].relay.dip[0],
				user->rule[i].relay.dip_mask[0],
				user->rule[i].relay.sport_start,
				user->rule[i].relay.sport_end,
				user->rule[i].relay.dport_start,
				user->rule[i].relay.dport_end);
		} else
			fprintf(fp,
				"rule=0x%08x/%02u 0x%08x/%02u 0x%08x/%02u\n",
				user->rule[i].protocol,
				user->rule[i].protocol_mask,
				user->rule[i].relay.sip[0],
				user->rule[i].relay.sip_mask[0],
				user->rule[i].relay.dip[0],
				user->rule[i].relay.dip_mask[0]);
	}

err:
	fclose(fp);
	return ret;
}

static void usage()
{
	printf("Init table:\n");
	printf("./acl -i <device bdf> --init\n");
	printf("Save table:\n");
	printf("./acl -i <device bdf> --save\n");
	printf("====================================\n");
	printf("Insert ipv4 rule:\n");
	printf("./acl [-4] -i <device bdf> -I <chain> <rule_index> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -i 0000:01:00.2 -I INPUT 1 -p TCP -s 192.168.1.100/24 --sport 1234:5678 --dport 1111:2222 -j deny\n");
	printf("./acl -i 0000:01:00.2 -I OUTPUT 1 -s 192.168.1.100 --dport 1234 -j deny\n");
	printf("------------------------------------\n");

	printf("Append ipv4 rule:\n");
	printf("./acl [-4] -i <device bdf> -A <chain> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -i 0000:01:00.2 -A INPUT -p TCP -s 192.168.1.100/24 --sport 1234:5678 --dport 1111:2222 -j deny\n");
	printf("./acl -i 0000:01:00.2 -A OUTPUT -s 192.168.1.100 --dport 1234 -j deny\n");
	printf("------------------------------------\n");

	printf("Replace ipv4 rule:\n");
	printf("./acl [-4] -i <device bdf> -R <chain> <rule_index> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -i 0000:01:00.2 -R INPUT 1 -p TCP -s 192.168.1.100/24 --sport 1234:5678 --dport 1111:2222 -j deny\n");
	printf("./acl -i 0000:01:00.2 -R OUTPUT 2 -s 192.168.1.100 --dport 1234 -j deny\n");
	printf("------------------------------------\n");

	printf("Delete ipv4 rule:\n");
	printf("./acl [-4] -i <device bdf> -D <chain> <rule_index>\n");
	printf("./acl -i 0000:01:00.2 -D INPUT 1\n");
	printf("./acl -i 0000:01:00.2 -D OUTPUT 1\n");

	printf("====================================\n");
	printf("Insert ipv6 rule:\n");
	printf("./acl <-6> -i <device bdf> -I <chain> <rule_index> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -6 -i 0000:01:00.2 -I INPUT 1 -p TCP -s 2001:db8::1 --sport 1234:5678 --dport 1111:2222 -j deny\n");
	printf("./acl -6 -i 0000:01:00.2 -I OUTPUT 1 -s 2001:db8::1/16 --dport 1234 -j deny\n");
	printf("------------------------------------\n");

	printf("Append ipv6 rule:\n");
	printf("./acl <-6> -i <device bdf> -A <chain> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -6 -i 0000:01:00.2 -A INPUT -p TCP -s fe80::67c:16ff:fe16:a343 --sport 1234:5678 --dport 1111:2222 -j deny\n");
	printf("./acl -6 -i 0000:01:00.2 -A OUTPUT -s fe80::67c:16ff:fe16:a343/16 --dport 1234 -j deny\n");

	printf("Replace ipv6 rule:\n");
	printf("./acl <-6> -i <device bdf> -R <chain> <rule_index> -p <protocol_str> -s <sip>[/sip_mask] --sport start[:end] --dport start[:end] -j <action>\n");
	printf("./acl -6 -i 0000:01:00.2 -R OUTPUT 2 -p UDP -s fe80::67c:16ff:fe16:a343 --dport 1234 -j deny\n");
	printf("./acl -6 -i 0000:01:00.2 -R INPUT 1 -s fe80::67c:16ff:fe16:a343/16 --sport 4444:5555 --dport 6666:7777 -j deny\n");

	printf("Delete ipv6 rule:\n");
	printf("./acl <-6> -i <device bdf> -D <chain> <rule_index>\n");
	printf("./acl -6 -i 0000:01:00.2 -D INPUT 1\n");
	printf("./acl -6 -i 0000:01:00.2 -D OUTPUT 1\n");
	printf("====================================\n");
	printf("List rule:\n");
	printf("./acl -i <device bdf> -L\n");
	printf("./acl -i 0000:01:00.2 -L\n");
}

#if 0
static int read_yusur_table(struct user_command *cmd)
{
	//read
	char command[256];
	sprintf(command, "sudo %s --read-table -t %d > %s",
		cmd->exe_path, cmd->table_index, cmd->relay_file);
	ys_info("command=%s", command);

	// 打开管道以读取命令输出
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) {
		perror("Failed to open pipe for command");
		return -1;
	}
	// 打开文件以写入命令输出
	FILE *file = fopen("cmd->relay_file", "w");
	if (file == NULL) {
		perror("Failed to open file for writing");
		pclose(pipe);
		return -1;
	}
	// 读取管道并写入文件
	char buffer[1024];
	while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		fputs(buffer, file);
	}

	// 关闭管道和文件
	pclose(pipe);
	fclose(file);

	ys_info("Command output has been written to '%s'\n", cmd->relay_file);

	return 0;
}
#endif

static int get_info_of_relay_table(const char *relay_file,
				   struct user_table *user)
{
	if (!relay_file) {
		ys_err("Error file %s\n", relay_file);
		return -1;
	}

	FILE *fp = fopen(relay_file, "r");
	if (fp == NULL) {
		ys_err("Error opening file %s\n", relay_file);
		return -1;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL) {
		//ys_debug("Read line: %s", line);      // 调试输出
		if (strncmp(line, "table_index=", strlen("table_index=")) == 0) {
//                      ys_debug("Read line: %s", line);        // 调试输出
			int result = sscanf(line, "table_index=%d",
					    &user->table_index);
			if (result != 1) {
				ys_err("Error reading table info %d\n", result);
				fclose(fp);
				return -1;
			}
			break;
		} else if (strncmp(line, "rule_num=", strlen("rule_num="))
			   == 0) {
			//ys_debug("Read line: %s", line);      // 调试输出
			int result = sscanf(line, "rule_num=%d",
					    &user->rule_num);
			if (result != 1) {
				ys_err("Error reading table info %d\n", result);
				fclose(fp);
				return -1;
			}
			break;
		}
	}

	fclose(fp);
	return 0;

}

// 将 IPv4 地址转换为 8 个 16 位的无符号整数数组
int bitsegments_to_ipv4(const u32 *segments, char *ipv4str)
{
	if (inet_ntop(AF_INET, segments, ipv4str, INET_ADDRSTRLEN) == NULL) {
		perror("inet_ntop");
		exit(EXIT_FAILURE);
	}
	return 0;
}

// 将 IPv6 地址转换为 8 个 16 位的无符号整数数组
int bitsegments_to_ipv6(const u32 *segments, char *ipv6str)
{
	u8 buf[sizeof(struct in6_addr)] = { 0 };
	for (int i = 0; i < 4; i++) {
		buf[4 * i] = segments[i] & 0xFF;
		buf[4 * i + 1] = (segments[i] >> 8) & 0xFF;
		buf[4 * i + 2] = (segments[i] >> 16) & 0xFF;
		buf[4 * i + 3] = (segments[i] >> 24) & 0xFF;
//      ys_debug("seg %d:%08x, buf[] 0x%02x%02x%02x%02x", i, segments[i], buf[4 * i+3] , buf[4 * i + 2], buf[4 * i + 1], buf[4 * i]);
	}

	if (inet_ntop(AF_INET6, buf, ipv6str, INET6_ADDRSTRLEN) == NULL) {
		ys_err("inet_ntop");
		return -1;	// 转换失败
	}
//    ys_debug("ip_str[%s]", ipv6str);

	return 0;		// 转换成功
}

int get_ipv6_prefix(const u32 *mask, u32 *prefix)
{
	for (int i = 0; i < 4; i++)
		*prefix += mask[i];

	return 0;
}

static void dump_user_table(struct user_command *cmd, struct user_table *user)
{
	switch (user->table_index) {
	case 0 ... 1:
		printf("V4 %s info:\n", user->table_index % 2 == 0 ? "INPUT": "OUTPUT");
		break;
	case 2 ... 3:
		printf("V6 %s info:\n", user->table_index % 2 == 0 ? "INPUT": "OUTPUT");
		break;
	}

	printf
	    ("ID%*sPROT/MASK%*sSIP/PREFIX%*sDIP/PREFIX%*sSPORT_REGION%*sDPORT_REGION%*sACTION\n",
	     6, " ", 29, " ", 38, " ", 9, " ", 4, " ", 3, " ");
	for (int i = 0; i < user->rule_num; i++) {
		if (cmd->rule_index == -1)
			printf
			    ("[%d]\t0x%02x/%u\t%39s/%u\t%39s/%u\t%11s\t%11s\t%s\n",
			     i, user->rule[i].protocol,
			     user->rule[i].protocol_mask,
			     user->rule[i].user.sip,
			     user->rule[i].user.sip_mask,
			     user->rule[i].user.dip,
			     user->rule[i].user.dip_mask,
			     user->rule[i].user.sport,
			     user->rule[i].user.dport,
			     user->rule[i].action ==
			     0x11 ? " deny " : "permit");
		else if (cmd->rule_index == i)
			printf
			    ("[%d]\t0x%02x/%u\t%39s/%u\t%39s/%u\t%11s\t%11s\t%s\n",
			     i, user->rule[i].protocol,
			     user->rule[i].protocol_mask,
			     user->rule[i].user.sip,
			     user->rule[i].user.sip_mask,
			     user->rule[i].user.dip,
			     user->rule[i].user.dip_mask,
			     user->rule[i].user.sport,
			     user->rule[i].user.dport,
			     user->rule[i].action ==
			     0x11 ? " deny " : "permit");
		else
			continue;
	}
	printf("\n");
}

static int save_user_table(struct user_command *cmd, struct user_table *user)
{
	char user_file[MAX_INFO_LEN];
	sprintf(user_file, "/tmp/user/table%d", user->table_index);
	FILE *fp = fopen(user_file, "w");
	if (fp == NULL) {
		ys_err("Error opening file %s", user_file);
		return -1;
	}

	fprintf(fp, "table:%d\n", user->table_index);
	switch (user->table_index) {
	case 0 ... 1:
		fprintf(fp, "[protocol, sip, dip, sport, dport]\n\n");
		break;
	case 2 ... 3:
		fprintf(fp, "[protocol, src_1, src_2, src_3, src_4, dst_1, dst_2, dst_3, dst_4, s_port, d_port]\n\n");
		break;
	}
	for (int i = 0; i < user->rule_num; i++) {
		fprintf(fp,
		    "0x%02x/%u\t%s/%u\t%s/%u\t%s\t%s\t%d\n",
		     user->rule[i].protocol,
		     user->rule[i].protocol_mask,
		     user->rule[i].user.sip,
		     user->rule[i].user.sip_mask,
		     user->rule[i].user.dip,
		     user->rule[i].user.dip_mask,
		     user->rule[i].user.sport,
		     user->rule[i].user.dport,
		     user->rule[i].action);
	}
	fclose(fp);
	return 0;
}

static int get_rule_of_relay_table(const char *relay_file,
				   struct user_table *user)
{
	char line[256];
	u32 sport, dport;
	struct rule *rule;
	int dim = 0;
	int id = 0;

	if (!relay_file) {
		ys_err("Error file %s", relay_file);
		return -1;
	}
	char **str = allocate_string_array(MAX_DIM_NUM, MAX_INFO_LEN);
	if (str == NULL) {
		ys_err("Failed to alloc str");
		return -1;
	}
	FILE *fp = fopen(relay_file, "r");
	if (fp == NULL) {
		ys_err("Error opening file %s", relay_file);
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (strncmp
		    (line, "action_index=", strlen("action_index=")) == 0) {
			//ys_debug("Read line: %s", line);      // 调试输出
			rule = &user->rule[id];
			int result = sscanf(line, "action_index=%hx",
					    &rule->action);
			if (result != 1) {
				ys_err("Error reading table info %d", result);
				fclose(fp);
				return -1;
			}
		} else if (strncmp(line, "rule=", strlen("rule=")) == 0) {
			//ys_debug("Read line: %s", line);      // 调试输出
			dim = ys_strtoken_relay_rule(line, str);
			switch (dim) {
			case 6:	//ipv4
				user->dim = 3;
				rule->protocol = strtoul(str[0], NULL, 16);
				rule->protocol_mask = atoi(str[1]);
				rule->relay.sip[0] = strtoul(str[2], NULL, 16);
				rule->user.sip_mask = rule->relay.sip_mask[0] =
				    atoi(str[3]);
				rule->relay.dip[0] = strtoul(str[4], NULL, 16);
				rule->user.dip_mask = rule->relay.dip_mask[0] =
				    atoi(str[5]);

				bitsegments_to_ipv4(rule->relay.sip,
						    rule->user.sip);
				bitsegments_to_ipv4(rule->relay.dip,
						    rule->user.dip);
				id++;
				break;
			case 10:	//tcp-udp/ipv4
				user->dim = 5;
				rule->protocol = strtoul(str[0], NULL, 16);
				rule->protocol_mask = atoi(str[1]);
				rule->relay.sip[0] = strtoul(str[2], NULL, 16);
				rule->user.sip_mask = rule->relay.sip_mask[0] =
				    atoi(str[3]);
				rule->relay.dip[0] = strtoul(str[4], NULL, 16);
				rule->user.dip_mask = rule->relay.dip_mask[0] =
				    atoi(str[5]);
				sport = strtoul(str[6], NULL, 16);
				rule->relay.sport_start = sport >> 16;
				rule->relay.sport_end = sport & 0xFFFF;
				dport = strtoul(str[8], NULL, 16);
				rule->relay.dport_start = dport >> 16;
				rule->relay.dport_end = dport & 0xFFFF;
				bitsegments_to_ipv4(rule->relay.sip,
						    rule->user.sip);
				bitsegments_to_ipv4(rule->relay.dip,
						    rule->user.dip);
				sprintf(rule->user.sport, "%u:%u",
					rule->relay.sport_start,
					rule->relay.sport_end & 0xFFFF);
				sprintf(rule->user.dport, "%u:%u",
					rule->relay.dport_start,
					rule->relay.dport_end & 0xFFFF);
				id++;
				break;
			case 18:	//ipv6
				user->dim = 9;
				rule->protocol = strtoul(str[0], NULL, 16);
				rule->protocol_mask = atoi(str[1]);
				rule->relay.sip[0] = strtoul(str[2], NULL, 16);
				rule->relay.sip_mask[0] = atoi(str[3]);
				rule->relay.sip[1] = strtoul(str[4], NULL, 16);
				rule->relay.sip_mask[1] = atoi(str[5]);
				rule->relay.sip[2] = strtoul(str[6], NULL, 16);
				rule->relay.sip_mask[2] = atoi(str[7]);
				rule->relay.sip[3] = strtoul(str[8], NULL, 16);
				rule->relay.sip_mask[3] = atoi(str[9]);
				rule->relay.dip[0] = strtoul(str[10], NULL, 16);
				rule->relay.dip_mask[0] = atoi(str[11]);
				rule->relay.dip[1] = strtoul(str[12], NULL, 16);
				rule->relay.dip_mask[1] = atoi(str[13]);
				rule->relay.dip[2] = strtoul(str[14], NULL, 16);
				rule->relay.dip_mask[2] = atoi(str[15]);
				rule->relay.dip[3] = strtoul(str[16], NULL, 16);
				rule->relay.dip_mask[3] = atoi(str[17]);
				bitsegments_to_ipv6(rule->relay.sip,
						    rule->user.sip);
				bitsegments_to_ipv6(rule->relay.dip,
						    rule->user.dip);
				get_ipv6_prefix(rule->relay.sip_mask,
						&rule->user.sip_mask);
				get_ipv6_prefix(rule->relay.dip_mask,
						&rule->user.dip_mask);
				id++;
				break;
			case 22:	//tcp-udp ipv6
				user->dim = 11;
				rule->protocol = strtoul(str[0], NULL, 16);
				rule->protocol_mask = atoi(str[1]);
				rule->relay.sip[0] = strtoul(str[2], NULL, 16);
				rule->relay.sip_mask[0] = atoi(str[3]);
				rule->relay.sip[1] = strtoul(str[4], NULL, 16);
				rule->relay.sip_mask[1] = atoi(str[5]);
				rule->relay.sip[2] = strtoul(str[6], NULL, 16);
				rule->relay.sip_mask[2] = atoi(str[7]);
				rule->relay.sip[3] = strtoul(str[8], NULL, 16);
				rule->relay.sip_mask[3] = atoi(str[9]);
				rule->relay.dip[0] = strtoul(str[10], NULL, 16);
				rule->relay.dip_mask[0] = atoi(str[11]);
				rule->relay.dip[1] = strtoul(str[12], NULL, 16);
				rule->relay.dip_mask[1] = atoi(str[13]);
				rule->relay.dip[2] = strtoul(str[14], NULL, 16);
				rule->relay.dip_mask[2] = atoi(str[15]);
				rule->relay.dip[3] = strtoul(str[16], NULL, 16);
				rule->relay.dip_mask[3] = atoi(str[17]);
				sport = strtoul(str[18], NULL, 16);
				rule->relay.sport_start = sport >> 16;
				rule->relay.sport_end = sport & 0xFFFF;
				dport = strtoul(str[20], NULL, 16);
				rule->relay.dport_start = dport >> 16;
				rule->relay.dport_end = dport & 0xFFFF;
				bitsegments_to_ipv6(rule->relay.sip,
						    rule->user.sip);
				bitsegments_to_ipv6(rule->relay.dip,
						    rule->user.dip);
				get_ipv6_prefix(rule->relay.sip_mask,
						&rule->user.sip_mask);
				get_ipv6_prefix(rule->relay.dip_mask,
						&rule->user.dip_mask);
				sprintf(rule->user.sport, "%u:%u",
					rule->relay.sport_start,
					rule->relay.sport_end & 0xFFFF);
				sprintf(rule->user.dport, "%u:%u",
					rule->relay.dport_start,
					rule->relay.dport_end & 0xFFFF);
				id++;
				break;
			default:
				if (dim > 6 && dim != 10 && dim != 18
				    && dim != 22)
					ys_err("Unsupport dim %d", dim);
				break;;
			}
		}
	}

	user->rule_num = id;

	free_string_array(str, MAX_DIM_NUM);
	fclose(fp);
	return 0;
}

int trans_ys2user(const char *relay_file, struct user_table *user)
{
	get_info_of_relay_table(relay_file, user);
	get_rule_of_relay_table(relay_file, user);

	return 0;
}

static void generate_relay_command(struct user_command *cmd, char *command, int opt_type)
{
	char *opt_str;
	switch (cmd->opt_type) {
	case YS_FME_RUNTME_INSERT_RULE:
	case YS_FME_RUNTME_APPEND_RULE:
		opt_str = "--add-rule";
	break;
	case YS_FME_RUNTME_REPLACE_RULE:
		opt_str = "--change-rule";
	break;
	}

	if (cmd->table_index == 0 || cmd->table_index == 1) {
		sprintf(command,
			"sudo ../ysfmemgr -p %s %s -t %d -l %d -a %d --rule '0x%08x/%u 0x%08x/%u 0x%08x/%u  0x%04x%04x/0 0x%04x%04x/0'",
			cmd->device,
			opt_str,
			cmd->table_index,
			cmd->rule_index,
			cmd->rule.action, cmd->rule.protocol,
			cmd->rule.protocol_mask,
			cmd->rule.relay.sip[0],
			cmd->rule.relay.sip_mask[0],
			cmd->rule.relay.dip[0],
			cmd->rule.relay.dip_mask[0],
			cmd->rule.relay.sport_start,
			cmd->rule.relay.sport_end,
			cmd->rule.relay.dport_start,
			cmd->rule.relay.dport_end);
	} else if (cmd->table_index == 2 || cmd->table_index == 3) {
		sprintf(command,
			"sudo ../ysfmemgr -p %s %s -t %d -l %d -a %d --rule '0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%08x/%u 0x%04x%04x/0 0x%04x%04x/0'",
			cmd->device,
			opt_str,
			cmd->table_index,
			cmd->rule_index,
			cmd->rule.action, cmd->rule.protocol,
			cmd->rule.protocol_mask,
			cmd->rule.relay.sip[0],
			cmd->rule.relay.sip_mask[0],
			cmd->rule.relay.sip[1],
			cmd->rule.relay.sip_mask[1],
			cmd->rule.relay.sip[2],
			cmd->rule.relay.sip_mask[2],
			cmd->rule.relay.sip[3],
			cmd->rule.relay.sip_mask[3],
			cmd->rule.relay.dip[0],
			cmd->rule.relay.dip_mask[0],
			cmd->rule.relay.dip[1],
			cmd->rule.relay.dip_mask[1],
			cmd->rule.relay.dip[2],
			cmd->rule.relay.dip_mask[2],
			cmd->rule.relay.dip[3],
			cmd->rule.relay.dip_mask[3],
			cmd->rule.relay.sport_start,
			cmd->rule.relay.sport_end,
			cmd->rule.relay.dport_start,
			cmd->rule.relay.dport_end);
	}
}

static int do_cmd_operation(struct user_command *cmd)
{
	char user_path[MAX_INFO_LEN] = { 0 };
	char relay_file[MAX_INFO_LEN];
	char command[MAX_INFO_LEN];
	struct user_table *user;
	int ret = 0;

	switch (cmd->opt_type) {
	case YS_FME_RUNTME_INSERT_RULE:
		generate_relay_command(cmd, command, cmd->opt_type);
		ys_info("command=%s", command);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		break;
	case YS_FME_RUNTME_APPEND_RULE:
		sprintf(relay_file, "/tmp/relay/table%d", cmd->table_index);
		sprintf(command,
			"sudo ../ysfmemgr -p %s --read-table -t %d > /tmp/relay/table%d",
			cmd->device, cmd->table_index, cmd->table_index);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		user = calloc(1, sizeof(*user));
		user->table_index = cmd->table_index;
		trans_ys2user(relay_file, user);
		cmd->rule_index = user->rule_num, free(user);
		generate_relay_command(cmd, command, cmd->opt_type);
		ys_info("command=%s", command);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		break;
	case YS_FME_RUNTME_DELETE_RULE:
		sprintf(command,
			"sudo ../ysfmemgr -p %s --delete-rule -t %d -l %d",
			cmd->device, cmd->table_index, cmd->rule_index);
		ys_info("command=%s", command);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		break;
	case YS_FME_RUNTME_REPLACE_RULE:
		generate_relay_command(cmd, command, cmd->opt_type);
		ys_info("command=%s", command);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		break;
	case YS_FME_RUNTME_READ_TABLE:
		for (int i = 0; i < MAX_TABLE_NUM; i++) {
			sprintf(relay_file, "/tmp/relay/table%d", i);
			sprintf(command,
				"sudo ../ysfmemgr -p %s --read-table -t %d > /tmp/relay/table%d",
				cmd->device, i, i);
			ret = system(command);
			if (ret) {
				ys_err
				    ("command exec failed, command:%s, ret: %d",
				     command, ret);
				return -1;
			}
			user = calloc(1, sizeof(*user));
			user->table_index = i;
			trans_ys2user(relay_file, user);
			dump_user_table(cmd, user);
			free(user);
		}
		break;
	case YS_FME_SAVE:
		for (int i = 0; i < MAX_TABLE_NUM; i++) {
			sprintf(relay_file, "/tmp/relay/table%d", i);
			sprintf(command,
				"sudo ../ysfmemgr -p %s --read-table -t %d > /tmp/relay/table%d",
				cmd->device, i, i);
			ret = system(command);
			if (ret) {
				ys_err
				    ("command exec failed, command:%s, ret: %d",
				     command, ret);
				return -1;
			}
			user = calloc(1, sizeof(*user));
			user->table_index = i;
			trans_ys2user(relay_file, user);
			save_user_table(cmd, user);
			free(user);
		}
		ys_info("Save rule table to /tmp/user/ success.");
	break;
	case YS_FME_INIT:
		sprintf(command,
			"sudo ../ysfmemgr -p %s --uninit", cmd->device);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}

		for (int i = 0; i < MAX_TABLE_NUM; i++) {
			mkdir(DEFAULT_USER_TABLE_PATH, 0755);
			sprintf(user_path, DEFAULT_USER_TABLE_PATH "/table%d",
				i);
			ys_debug("user path:%s", user_path);

			user = calloc(1, sizeof(*user));
			user->table_index = i;
			ret = parse_user_table(user_path, user);
			if (ret)
				return ret;

			if (i < 2) {
				ret = trans_user2ys(user, cmd);
				if (ret)
					return ret;
			} else {
				ret = trans_user2ys_ipv6(user, cmd);
				if (ret)
					return ret;
			}

			free(user);
		}
		sprintf(command,
			"sudo ../ysfmemgr %s --init -f '/tmp/relay/table0;/tmp/relay/table1;/tmp/relay/table2;/tmp/relay/table3'",
			cmd->device);
		ret = system(command);
		if (ret) {
			ys_err("command exec failed, command:%s, ret: %d",
			       command, ret);
			return -1;
		}
		break;
	default:
		ys_err("Unsupport operation %d", cmd->opt_type);
		return -1;
	}
	return 0;
}

enum {
	CMD_NONE = 0,
	CMD_INSERT = 1 << 0,
	CMD_DELETE = 1 << 1,
	CMD_DELETE_NUM = 1 << 2,
	CMD_REPLACE = 1 << 3,
	CMD_APPEND = 1 << 4,
	CMD_LIST = 1 << 5,
	CMD_FLUSH = 1 << 6,
	CMD_ZERO = 1 << 7,
	CMD_NEW_CHAIN = 1 << 8,
	CMD_DELETE_CHAIN = 1 << 9,
	CMD_SET_POLICY = 1 << 10,
	CMD_RENAME_CHAIN = 1 << 11,
	CMD_LIST_RULES = 1 << 12,
	CMD_ZERO_NUM = 1 << 13,
	CMD_CHECK = 1 << 14,
};

static int check_input_rule(struct user_command *cmd)
{
	switch (cmd->opt_type) {
	case YS_FME_RUNTME_INSERT_RULE:
		if (!cmd->rule_index_str) {
			ys_err("Invalid rule index argument");
			return -1;
		}
		/* fallthrough */
	case YS_FME_RUNTME_APPEND_RULE:
		if ((!cmd->protocol_str) && (!strlen(cmd->rule.user.sip))
		    && (!strlen(cmd->rule.user.dip))
		    && (!strlen(cmd->rule.user.sport))
		    && (!strlen(cmd->rule.user.dport))) {
			ys_err("Invalid rule argument");
			return -1;
		}
		break;
	}

	if (!cmd->device) {
		ys_err("Invalid device argument");
		return -1;
	}

	return 0;
}

static int init_cmd(struct user_command *cmd)
{
	int ret = 0;

	cmd->exe_path = "../ysfmemgr";

	ret = check_input_rule(cmd);
	if (ret)
		return ret;

	trans_user_protocol(&cmd->rule, cmd->protocol_str);
	trans_user_ip(&cmd->rule, cmd->ip_type);
	trans_user_port(&cmd->rule);

	if (cmd->jump) {
		if (strcmp(cmd->jump, "permit") == 0)
			cmd->rule.action = 0x1;
		else if (strcmp(cmd->jump, "deny") == 0)
			cmd->rule.action = 0x11;
		else {
			ys_err("Invalid rule action (%s)", cmd->jump);
			return -1;
		}
	}

	int chain;
	if (cmd->chain && strcasecmp(cmd->chain, "OUTPUT") == 0) {
		chain = 1;
	} else if (!cmd->chain || (strcasecmp(cmd->chain, "INPUT") == 0)) {	//default INPUT
		chain = 0;
	} else {
		ys_err("Invalid chain %s", cmd->chain);
		return -1;
	}

	if (cmd->ip_type != AF_INET6) {
		cmd->table_index = 0 + chain;
	} else {
		cmd->table_index = 2 + chain;
	}

	if (cmd->rule_index_str)
		cmd->rule_index = atoi(cmd->rule_index_str);
	else
		cmd->rule_index = -1;

	ys_debug("device = %s", cmd->device);
	ys_debug("ip_type = %d", cmd->ip_type);
	ys_debug("protocol = 0x%x", cmd->rule.protocol);
	ys_debug("protocol_mask = %d", cmd->rule.protocol_mask);
	ys_debug("Src IP: %s, sMask: %d", cmd->rule.user.sip,
		cmd->rule.user.sip_mask);
	ys_debug("Dst IP: %s, dMask: %d", cmd->rule.user.dip,
		cmd->rule.user.dip_mask);
	ys_debug("sport: %s", cmd->rule.user.sport);
	ys_debug("dport: %s", cmd->rule.user.dport);

	ys_debug("chain: %s", cmd->chain);
	ys_debug("Rule Index: %d", cmd->rule_index);
	ys_debug("Table Index: %d", cmd->table_index);
	return 0;
}

static inline bool xs_has_arg(int argc, char *argv[])
{
	return optind < argc &&
	    argv[optind][0] != '-' && argv[optind][0] != '!';
}

int main(int argc, char **argv)
{
	int option_index = 0;
	int opt = 0;
	int ret = 0;
	struct user_command *cmd;

	if (!argv[1]) {
		usage();
		return -1;
	}

	cmd = calloc(1, sizeof(*cmd));
	while ((opt =
		getopt_long(argc, argv,
			    "46A:C:d:D:hi:I:j:L::R:p:s:",
			    ys_fme_long_options, &option_index)) != -1) {
		switch (opt) {
		case '4':
			commands_options[OPT_4] = true;
			cmd->ip_type = AF_INET;
			break;
		case '6':
			commands_options[OPT_6] = true;
			cmd->ip_type = AF_INET6;
			break;
		case 'A':
			commands_options[OPT_A] = true;
			cmd->opt_type = YS_FME_RUNTME_APPEND_RULE;
			if (optarg)
				cmd->chain = optarg;
			else if (xs_has_arg(argc, argv))
				cmd->chain = argv[optind++];

			if (xs_has_arg(argc, argv))
				cmd->rule_index_str = argv[optind++];
			break;
		case 'd':
			commands_options[OPT_d] = true;
			ys_str_copy(cmd->rule.user.dip, optarg);
			break;
		case 'D':	/* Delete rule */
			commands_options[OPT_D] = true;
			cmd->opt_type = YS_FME_RUNTME_DELETE_RULE;
			if (optarg)
				cmd->chain = optarg;
			else if (xs_has_arg(argc, argv))
				cmd->chain = argv[optind++];

			if (xs_has_arg(argc, argv))
				cmd->rule_index_str = argv[optind++];
			break;
		case 'h':
			commands_options[OPT_h] = true;
			usage();
			return 0;
		case 'i':
			commands_options[OPT_i] = true;
			cmd->device = optarg;
			break;
		case 'I':
			commands_options[OPT_I] = true;
			cmd->opt_type = YS_FME_RUNTME_INSERT_RULE;
			if (optarg)
				cmd->chain = optarg;
			else if (xs_has_arg(argc, argv))
				cmd->chain = argv[optind++];

			if (xs_has_arg(argc, argv))
				cmd->rule_index_str = argv[optind++];
			break;
		case 'j':
			commands_options[OPT_j] = true;
			cmd->jump = optarg;
			break;
		case 'L':	/* Read Table */
			commands_options[OPT_L] = true;
			cmd->opt_type = YS_FME_RUNTME_READ_TABLE;
			if (optarg)
				cmd->chain = optarg;
			else if (xs_has_arg(argc, argv))
				cmd->chain = argv[optind++];

			if (xs_has_arg(argc, argv))
				cmd->rule_index_str = argv[optind++];

			break;
		case 'R':	/* Delete rule */
			commands_options[OPT_R] = true;
			cmd->opt_type = YS_FME_RUNTME_REPLACE_RULE;
			if (optarg)
				cmd->chain = optarg;
			else if (xs_has_arg(argc, argv))
				cmd->chain = argv[optind++];

			if (xs_has_arg(argc, argv))
				cmd->rule_index_str = argv[optind++];
			break;
		case 'p':
			commands_options[OPT_p] = true;
			cmd->protocol_str = optarg;
			break;
		case 's':
			commands_options[OPT_s] = true;
			ys_str_copy(cmd->rule.user.sip, optarg);
			break;
		case 0:
			/* for only long options */
			if (strcmp
			    (ys_fme_long_options[option_index].name,
			     "sport") == 0) {
				commands_options[OPT_sport] = true;
				ys_str_copy(cmd->rule.user.sport, optarg);
			} else
			    if (strcmp
				(ys_fme_long_options[option_index].name,
				 "dport") == 0) {
				commands_options[OPT_dport] = true;
				ys_str_copy(cmd->rule.user.dport, optarg);
			} else
			    if (strcmp
				(ys_fme_long_options[option_index].name,
				 "init") == 0) {
				commands_options[OPT_init] = true;
				cmd->opt_type = YS_FME_INIT;
			} else
			    if (strcmp
				(ys_fme_long_options[option_index].name,
				 "rule-number") == 0) {
				commands_options[OPT_rule_number] = true;
				cmd->rule_index_str = optarg;
			} else
			    if (strcmp
				(ys_fme_long_options[option_index].name,
				 "save") == 0) {
				//commands_options[OPT_save] = true;
				//cmd->rule_index_str = optarg;
				cmd->opt_type = YS_FME_SAVE;
			}
			break;
		default:
			ys_err("Unknown option: %c", opt);
			usage();
			ret = -1;
			goto err;
		}
	}

	ret = generic_command_check(cmd->opt_type);
	if (ret)
		goto err;

	ret = init_cmd(cmd);
	if (ret)
		goto err;

	ret = do_cmd_operation(cmd);
	if (ret)
		goto err;

err:
	free(cmd);
	return ret;
}
