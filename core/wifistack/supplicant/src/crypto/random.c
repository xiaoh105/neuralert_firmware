/*
 * Random number generator
 * Copyright (c) 2010-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This random number generator is used to provide additional entropy to the
 * one provided by the operating system (os_get_random()) for session key
 * generation. The os_get_random() output is expected to be secure and the
 * implementation here is expected to provide only limited protection against
 * cases where os_get_random() cannot provide strong randomness. This
 * implementation shall not be assumed to be secure as the sole source of
 * randomness. The random_get_bytes() function mixes in randomness from
 * os_get_random() and as such, calls to os_get_random() can be replaced with
 * calls to random_get_bytes() without reducing security.
 *
 * The design here follows partially the design used in the Linux
 * drivers/char/random.c, but the implementation here is simpler and not as
 * strong. This is a compromise to reduce duplicated CPU effort and to avoid
 * extra code/memory size. As pointed out above, os_get_random() needs to be
 * guaranteed to be secure for any of the security assumptions to hold.
 */

#include "includes.h"

#if 0	// Not used in DA16200
#include "utils/supp_eloop.h"
#include "crypto.h"
#include "sha1.h"
#include "random.h"

#define POOL_WORDS		32
#define POOL_WORDS_MASK		(POOL_WORDS - 1)
#define POOL_TAP1		26
#define POOL_TAP2		20
#define POOL_TAP3		14
#define POOL_TAP4		7
#define POOL_TAP5		1
#define EXTRACT_LEN		16
#define MIN_READY_MARK		2

static u32 pool[POOL_WORDS];
static unsigned int input_rotate = 0;
static unsigned int pool_pos = 0;
static u8 dummy_key[20];
static unsigned int own_pool_ready = 0;
#define RANDOM_ENTROPY_SIZE 20

static char *random_entropy_file = NULL;
static int random_entropy_file_read = 0;

#define MIN_COLLECT_ENTROPY 1000
static unsigned int entropy = 0;
static unsigned int total_collected = 0;


static void random_write_entropy(void);


static u32 __ROL32(u32 x, u32 y)
{
	return (x << (y & 31)) | (x >> (32 - (y & 31)));
}


static void random_mix_pool(const void *buf, size_t len)
{
	static const u32 twist[8] = {
		0x00000000, 0x3b6e20c8, 0x76dc4190, 0x4db26158,
		0xedb88320, 0xd6d6a3e8, 0x9b64c2b0, 0xa00ae278
	};
	const u8 *pos = buf;
	u32 w;

	wpa_hexdump_key(MSG_EXCESSIVE, "random_mix_pool", buf, len);

	while (len--) {
		w = __ROL32(*pos++, input_rotate & 31);
		input_rotate += pool_pos ? 7 : 14;
		pool_pos = (pool_pos - 1) & POOL_WORDS_MASK;
		w ^= pool[pool_pos];
		w ^= pool[(pool_pos + POOL_TAP1) & POOL_WORDS_MASK];
		w ^= pool[(pool_pos + POOL_TAP2) & POOL_WORDS_MASK];
		w ^= pool[(pool_pos + POOL_TAP3) & POOL_WORDS_MASK];
		w ^= pool[(pool_pos + POOL_TAP4) & POOL_WORDS_MASK];
		w ^= pool[(pool_pos + POOL_TAP5) & POOL_WORDS_MASK];
		pool[pool_pos] = (w >> 3) ^ twist[w & 7];
	}
}


static void random_extract(u8 *out)
{
	unsigned int i;
	u8 hash[SHA1_MAC_LEN];
	u32 *hash_ptr;
	u32 buf[POOL_WORDS / 2];

	/* First, add hash back to pool to make backtracking more difficult. */
	hmac_sha1(dummy_key, sizeof(dummy_key), (const u8 *) pool,
		  sizeof(pool), hash);
	random_mix_pool(hash, sizeof(hash));
	/* Hash half the pool to extra data */
	for (i = 0; i < POOL_WORDS / 2; i++)
		buf[i] = pool[(pool_pos - i) & POOL_WORDS_MASK];
	hmac_sha1(dummy_key, sizeof(dummy_key), (const u8 *) buf,
		  sizeof(buf), hash);
	/*
	 * Fold the hash to further reduce any potential output pattern.
	 * Though, compromise this to reduce CPU use for the most common output
	 * length (32) and return 16 bytes from instead of only half.
	 */
	hash_ptr = (u32 *) hash;
	hash_ptr[0] ^= hash_ptr[4];
	os_memcpy(out, hash, EXTRACT_LEN);
}


void random_add_randomness(const void *buf, size_t len)
{
	struct os_time t;
	static unsigned int count = 0;

	count++;
	if (entropy > MIN_COLLECT_ENTROPY && (count & 0x3ff) != 0) {
		/*
		 * No need to add more entropy at this point, so save CPU and
		 * skip the update.
		 */
		return;
	}
	da16x_prt("[%s] Add randomness: count=%u entropy=%u\n",
		   __func__, count, entropy);

	os_get_time(&t);
	wpa_hexdump_key(MSG_EXCESSIVE, "random pool",
			(const u8 *) pool, sizeof(pool));
	random_mix_pool(&t, sizeof(t));
	random_mix_pool(buf, len);
	wpa_hexdump_key(MSG_EXCESSIVE, "random pool",
			(const u8 *) pool, sizeof(pool));
	entropy++;
	total_collected++;
}


int random_get_bytes(void *buf, size_t len)
{
	int ret;
	u8 *bytes = buf;
	size_t left;

#ifdef CHECK_SUPPLICANT_ERR
	da16x_tls_prt("[%s] Get randomness: len=%u entropy=%u\n",
		   __func__, (unsigned int) len, entropy);
#endif /*CHECK_SUPPLICANT_ERR*/

	/* Start with assumed strong randomness from OS */
	ret = os_get_random(buf, len);

#ifdef CHECK_SUPPLICANT_ERR
	da16x_tls_dump("random from os_get_random", buf, len);
#endif	/*CHECK_SUPPLICANT_ERR*/

	/* Mix in additional entropy extracted from the internal pool */
	left = len;
	while (left) {
		size_t siz, i;
		u8 tmp[EXTRACT_LEN];

		random_extract(tmp);

#ifdef CHECK_SUPPLICANT_ERR
		da16x_tls_dump("random from internal pool", tmp, sizeof(tmp));
#endif /*CHECK_SUPPLICANT_ERR*/

		siz = left > EXTRACT_LEN ? EXTRACT_LEN : left;
		for (i = 0; i < siz; i++)
			*bytes++ ^= tmp[i];
		left -= siz;
	}

#ifdef CONFIG_FIPS
	/* Mix in additional entropy from the crypto module */
	left = len;
	while (left) {
		size_t siz, i;
		u8 tmp[EXTRACT_LEN];
		if (crypto_get_random(tmp, sizeof(tmp)) < 0) {
			da16x_prt("[%s] random: No entropy available "
				   "for generating strong random bytes\n",
				__func__);

			return -1;
		}

#ifdef CHECK_SUPPLICANT_ERR
		da16x_tls_dump("[%s] random from crypto module", tmp, sizeof(tmp));
#endif /*CHECK_SUPPLICANT_ERR*/

		siz = left > EXTRACT_LEN ? EXTRACT_LEN : left;
		for (i = 0; i < siz; i++)
			*bytes++ ^= tmp[i];
		left -= siz;
	}
#endif /* CONFIG_FIPS */

	da16x_prt("mixed random\n");

	if (entropy < len)
		entropy = 0;
	else
		entropy -= len;

	return ret;
}


void random_mark_pool_ready(void)
{
	own_pool_ready++;
	wpa_printf_dbg(MSG_DEBUG, "random: Mark internal entropy pool to be "
		   "ready (count=%u/%u)", own_pool_ready, MIN_READY_MARK);
	random_write_entropy();
}

static void random_read_entropy(void)
{
	char *buf;
	size_t len;

	if (!random_entropy_file)
		return;

	buf = os_readfile(random_entropy_file, &len);
	if (buf == NULL)
		return; /* entropy file not yet available */

	if (len != 1 + RANDOM_ENTROPY_SIZE) {
		wpa_printf_dbg(MSG_DEBUG, "random: Invalid entropy file %s",
			   random_entropy_file);
		os_free(buf);
		return;
	}

	own_pool_ready = (u8) buf[0];
	random_add_randomness(buf + 1, RANDOM_ENTROPY_SIZE);
	random_entropy_file_read = 1;
	os_free(buf);
	wpa_printf_dbg(MSG_DEBUG, "random: Added entropy from %s "
		   "(own_pool_ready=%u)",
		   random_entropy_file, own_pool_ready);
}

static void random_write_entropy(void)
{
	char buf[RANDOM_ENTROPY_SIZE];
	FILE *f;
	u8 opr;
	int fail = 0;

	if (!random_entropy_file)
		return;

	if (random_get_bytes(buf, RANDOM_ENTROPY_SIZE) < 0)
		return;

	f = fopen(random_entropy_file, "wb");
	if (f == NULL) {
		wpa_printf(MSG_ERROR, "random: Could not open entropy file %s "
			   "for writing", random_entropy_file);
		return;
	}

	opr = own_pool_ready > 0xff ? 0xff : own_pool_ready;
	if (fwrite(&opr, 1, 1, f) != 1 ||
	    fwrite(buf, RANDOM_ENTROPY_SIZE, 1, f) != 1)
		fail = 1;
	fclose(f);
	if (fail) {
		wpa_printf(MSG_ERROR, "random: Could not write entropy data "
			   "to %s", random_entropy_file);
		return;
	}

	wpa_printf_dbg(MSG_DEBUG, "random: Updated entropy file %s "
		   "(own_pool_ready=%u)",
		   random_entropy_file, own_pool_ready);
}


void random_init(const char *entropy_file)
{
	os_free(random_entropy_file);
	if (entropy_file)
		random_entropy_file = os_strdup(entropy_file);
	else
		random_entropy_file = NULL;

	random_read_entropy();
	random_write_entropy();
}


void random_deinit(void)
{
	random_write_entropy();
	os_free(random_entropy_file);
	random_entropy_file = NULL;
}
#endif	// 0

/* EOF */
