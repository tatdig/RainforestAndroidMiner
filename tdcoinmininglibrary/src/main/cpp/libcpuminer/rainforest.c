#include <miner.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "rfv2.h"
#include "portable_endian.h"

#include <stdbool.h>
// struct work_restart {
// 	volatile unsigned long	restart;
// 	char			padding[128 - sizeof(unsigned long)];
// };

extern struct work_restart *work_restart;
extern bool fulltest(const uint32_t *hash, const uint32_t *target);

static int pretest(const uint32_t *hash, const uint32_t *target)
{
	return hash[7] < target[7];
}

int scanhash_rainforest(int thr_id, uint32_t *pdata, const uint32_t *ptarget,
					uint32_t max_nonce, unsigned long *hashes_done)
{
	uint32_t data[20] __attribute__((aligned(64)));
	uint32_t hash[8] __attribute__((aligned(64)));
	uint32_t n = pdata[19];
	const uint32_t first_nonce = pdata[19];

	volatile uint8_t *restart = &(work_restart[thr_id].restart);
	uint32_t Htarg = ptarget[7];
	static void *rambox;
	int ret = 0;

	for (int i = 0; i < 19; i++) {
		be32enc(&data[i], pdata[i]);
	}

	if (!rambox) {
		//printf("Rambox not yet initialized\n");
		if (!thr_id) {
			/* only thread 0 is responsible for allocating the shared rambox */
			void *r = malloc(RFV2_RAMBOX_SIZE * 8);
			if (r == NULL) {
				//printf("[%d] rambox allocation failed\n", thr_id);
				*(volatile void **)&rambox = (void*)0x1;
				goto out;
			}
			//printf("Thread %d initializing the rambox\n", thr_id);
			rfv2_raminit(r);
			*(volatile void **)&rambox = r;
		} else {
			/* wait for thread 0 to finish alloc+init of rambox */
			while (!*(volatile void **)&rambox)
				usleep(100000);
		}
	}

	if (*(volatile void **)&rambox == (void*)0x1) {
		//printf("[%d] rambox allocation failed\n", thr_id);
		goto out; // the rambox wasn't properly initialized
	}

	do
	{
		ret = rfv2_scan_hdr((char *)data, rambox, hash, Htarg, n, max_nonce, restart);
		n = be32toh(data[19]);
		if (ret <= 0)
			break;

		if (fulltest(hash, ptarget)) {
			pdata[19] = n;
			*hashes_done = ret;
			ret = 1;
			goto out;
		} else {
			printf("Warning: rfv2_scan_hdr() returned invalid solution %u\n", n);
		}

		n++;
	} while (n < max_nonce && !work_restart[thr_id].restart);

	pdata[19] = n;
	*hashes_done = -ret;
	ret = 0;

out:
	return ret;
}
