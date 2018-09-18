#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ydb.h"

char *example_yaml =
	"system:\n"
	" monitor:\n"
	" stats:\n"
	"  rx-cnt: !!int 2000\n"
	"  tx-cnt: 2010\n"
	"  rmon:\n"
	"    rx-frame: 1343\n"
	"    rx-frame-64: 2343\n"
	"    rx-frame-65-127: 233\n"
	"    rx-frame-etc: 2\n"
	"    tx-frame: 2343\n"
	" mgmt:\n"
	"interfaces:\n"
	"  - eth0\n"
	"  - eth1\n"
	"  - eth3\n";

char *example_yaml2 =
	"monitor:\n"
	" mem: 10\n"
	" cpu: amd64\n";

int test_ydb_open_close()
{
	printf("\n\n=== %s ===\n", __func__);
	ydb *block1, *block2, *block3;
	block1 = ydb_open("/path/to/datablock1");
	block2 = ydb_open("/path/to/datablock2");
	block3 = ydb_open("/path/to/datablock3");

	ydb_close(block3);
	ydb_close(block2);
	ydb_close(block1);
	printf("\n");
	return 0;
}

int test_ydb_read_write()
{
	int num;
	ydb_res res = YDB_OK;
	printf("\n\n=== %s ===\n", __func__);
	ydb *block1, *block2, *block3;
	block1 = ydb_open("/path/to/datablock1");
	block2 = ydb_open("/path/to/datablock2");
	block3 = ydb_open("/path/to/datablock3");

	ydb_write(block1, "system: {hostname: 100c}");
	res = ydb_write(block1, "system: {fan-speed: 20}");
	if (res)
		goto _done;
	char hostname[129] = {0,};
	int speed = 0;

	// ydb_read(block1, "system: {hostname: %s} \n", temp);
	// ydb_read(block1, "system: {fan-speed: %d} \n", &speed);
	num = ydb_read(block1,
				   "system:\n"
				   " hostname: %s\n"
				   " fan-speed: %d\n",
				   hostname, &speed);
	printf("read num=%d hostname=%s, fan-speed=%d\n", num, hostname, speed);
	if (num < 0)
		goto _done;

	ydb_path_write(block1, "system/temporature=%d", 60);
	ydb_path_write(block1, "system/running=%s", "2 hours");


	char *temp = ydb_path_read(block1, "system/temporature");
	printf("temporature=%d", atoi(temp));
	ynode_dump(ydb_top(block1), 0, YDB_LEVEL_MAX);
_done:
	ydb_close(block1);
	ydb_close(block2);
	ydb_close(block3);
	printf("\n");
	return res;
}

#define TEST_FUNC(func)                    \
	do                                     \
	{                                      \
		if (func())                        \
		{                                  \
			printf("%s failed.\n", #func); \
			return -1;                     \
		}                                  \
	} while (0)

int main(int argc, char *argv[])
{
	ydb_log_severity = YDB_LOG_DBG;
	TEST_FUNC(test_ydb_open_close);
	TEST_FUNC(test_ydb_read_write);
	return 0;
}
