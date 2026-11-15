// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Known-answer + differential test for the ICP AES-GCM/AES-CCM paths.
 * Built against libicp sources (scripts/zfs-crypto-kat.py), driven through
 * the same crypto_encrypt()/crypto_decrypt() calls zio_crypt.c makes.
 *
 * usage: kat <aes_impl> <gcm_impl> <vectors> [uio]
 *   uio: input and output as multi-iovec uios split at odd sizes (as zio_crypt's
 *   page-sized ABD iovecs), so mode update calls get split mid-block
 * vector line: MODE id key iv aad pt ct tag valid   (hex, "-" = empty)
 *   MODE = GCM | CCM, valid = 1 (tag good) | 0 (decrypt must fail)
 * output per vector: "<id> PASS|FAIL|UNSUP <detail>", then a summary.
 * UNSUP = ICP rejected the parameter shape (valid vector, encrypt failed).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/zfs_context.h>
#include <sys/simd.h>
#include <sys/crypto/icp.h>
#include <sys/crypto/api.h>
#include <sys/crypto/common.h>

static uint8_t *
unhex(const char *s, size_t *len)
{
	size_t n = strcmp(s, "-") == 0 ? 0 : strlen(s) / 2;
	uint8_t *b = malloc(n + 1);

	for (size_t i = 0; i < n; i++) {
		unsigned v;

		sscanf(s + 2 * i, "%2x", &v);
		b[i] = (uint8_t)v;
	}
	*len = n;
	return (b);
}

static int uio_mode;
static int uio_seed;

/* split [base, base+len) into iovecs of assorted odd sizes */
static void
split_uio(zfs_uio_t *uio, struct iovec *iov, int max, uint8_t *base,
    size_t len)
{
	/* ICP assumes a block spans at most two iovecs: keep every one >= 16 B */
	static const size_t sizes[] = { 4096, 100, 16, 4097, 33, 8192, 31, 255,
	    17, 512, 48, 4000 };
	int n = 0;
	size_t off = 0;
	unsigned k = (unsigned)uio_seed++;

	while (off < len && n < max - 1) {
		size_t c = sizes[(k++) % (sizeof (sizes) / sizeof (sizes[0]))];

		if (c > len - off)
			c = len - off;
		iov[n].iov_base = base + off;
		iov[n].iov_len = c;
		off += c;
		n++;
	}
	if (off < len) {
		iov[n].iov_base = base + off;
		iov[n].iov_len = len - off;
		n++;
	}
	if (n == 0) {
		iov[0].iov_base = base;
		iov[0].iov_len = 0;
		n = 1;
	}
	memset(uio, 0, sizeof (*uio));
	uio->uio_iov = iov;
	uio->uio_iovcnt = n;
	uio->uio_resid = len;
}

static int
run(int gcm, const uint8_t *key, size_t keylen, const uint8_t *iv,
    size_t ivlen, const uint8_t *aad, size_t aadlen, uint8_t *in,
    size_t inlen, uint8_t *out, size_t outlen, size_t taglen, int enc)
{
	CK_AES_GCM_PARAMS g;
	CK_AES_CCM_PARAMS c;
	crypto_mechanism_t mech;
	crypto_key_t k;
	crypto_data_t din, dout;
	/* ICP wants the data length incl. MAC on the plaintext side when
	 * decrypting (see zio_do_crypt_uio) */
	size_t plain_full = enc ? inlen : outlen + taglen;

	mech.cm_type = crypto_mech2id(gcm ? SUN_CKM_AES_GCM : SUN_CKM_AES_CCM);
	if (gcm) {
		g.ulIvLen = ivlen;
		g.ulIvBits = ivlen * 8;
		g.ulAADLen = aadlen;
		g.pAAD = (uchar_t *)aad;
		g.ulTagBits = taglen * 8;
		g.pIv = (uchar_t *)iv;
		mech.cm_param = (char *)&g;
		mech.cm_param_len = sizeof (g);
	} else {
		c.ulNonceSize = ivlen;
		c.ulAuthDataSize = aadlen;
		c.authData = (uchar_t *)aad;
		c.ulMACSize = taglen;
		c.nonce = (uchar_t *)iv;
		c.ulDataSize = plain_full;
		mech.cm_param = (char *)&c;
		mech.cm_param_len = sizeof (c);
	}
	k.ck_data = (void *)key;
	k.ck_length = keylen * 8;
	zfs_uio_t uin, uout;
	struct iovec iin[4096], iout[4096];

	din.cd_offset = dout.cd_offset = 0;
	din.cd_length = inlen;
	dout.cd_length = outlen;
	if (uio_mode) {
		din.cd_format = dout.cd_format = CRYPTO_DATA_UIO;
		split_uio(&uin, iin, 4096, in, inlen);
		split_uio(&uout, iout, 4096, out, outlen);
		din.cd_uio = &uin;
		dout.cd_uio = &uout;
	} else {
		din.cd_format = dout.cd_format = CRYPTO_DATA_RAW;
		din.cd_raw.iov_base = (char *)in;
		din.cd_raw.iov_len = inlen;
		dout.cd_raw.iov_base = (char *)out;
		dout.cd_raw.iov_len = outlen;
	}
	return (enc ? crypto_encrypt(&mech, &din, &k, NULL, &dout) :
	    crypto_decrypt(&mech, &din, &k, NULL, &dout));
}

int
main(int argc, char **argv)
{
	char *line = NULL;
	size_t cap = 0;
	int pass = 0, fail = 0, unsup = 0;
	FILE *f;

	if (argc != 4 && argc != 5) {
		fprintf(stderr, "usage: kat aes_impl gcm_impl vectors [uio]\n");
		return (2);
	}
	setvbuf(stdout, NULL, _IOLBF, 0);
	icp_init();
	if (aes_impl_set(argv[1]) != 0 || gcm_impl_set(argv[2]) != 0) {
		fprintf(stderr, "FATAL: impl %s/%s not available\n", argv[1],
		    argv[2]);
		return (2);
	}
	uio_mode = argc == 5 && strcmp(argv[4], "uio") == 0;
	printf("INFO aes_hwcap=%d pmull_hwcap=%d uio=%d\n", zfs_aes_available(),
	    zfs_pmull_available(), uio_mode);
	f = fopen(argv[3], "r");
	if (f == NULL) {
		perror(argv[3]);
		return (2);
	}
	while (getline(&line, &cap, f) > 0) {
		char *tok[9], *sp = NULL, *mode;
		const char *id;
		int valid;
		size_t kl, il, al, pl, cl, tl;
		uint8_t *key, *iv, *aad, *pt, *ct, *tag, *buf, *buf2;
		int gcm, rc, ok = 1, n;
		const char *why = "";

		for (n = 0; n < 9; n++) {
			tok[n] = strtok_r(n == 0 ? line : NULL, " \t\r\n", &sp);
			if (tok[n] == NULL)
				break;
		}
		if (n != 9)
			continue;
		mode = tok[0];
		id = tok[1];
		valid = atoi(tok[8]);
		const char *hk = tok[2], *hiv = tok[3], *haad = tok[4],
		    *hpt = tok[5], *hct = tok[6], *htag = tok[7];

		gcm = strcmp(mode, "GCM") == 0;
		key = unhex(hk, &kl);
		iv = unhex(hiv, &il);
		aad = unhex(haad, &al);
		pt = unhex(hpt, &pl);
		ct = unhex(hct, &cl);
		tag = unhex(htag, &tl);
		buf = malloc(pl + tl + 16);
		buf2 = malloc(pl + tl + 16);

		if (valid) {
			rc = run(gcm, key, kl, iv, il, aad, al, pt, pl, buf,
			    pl + tl, tl, 1);
			if (rc != CRYPTO_SUCCESS) {
				printf("%s UNSUP encrypt rc=0x%x\n", id, rc);
				unsup++;
				goto next;
			}
			if (memcmp(buf, ct, cl) != 0) {
				ok = 0;
				why = "ciphertext mismatch";
			} else if (memcmp(buf + pl, tag, tl) != 0) {
				ok = 0;
				why = "tag mismatch";
			}
		}
		/* decrypt ct||tag */
		memcpy(buf, ct, cl);
		memcpy(buf + cl, tag, tl);
		rc = run(gcm, key, kl, iv, il, aad, al, buf, cl + tl, buf2,
		    cl, tl, 0);
		if (valid) {
			if (rc != CRYPTO_SUCCESS) {
				ok = 0;
				why = "decrypt rejected valid vector";
			} else if (memcmp(buf2, pt, pl) != 0) {
				ok = 0;
				why = "plaintext mismatch";
			}
		} else if (rc == CRYPTO_SUCCESS) {
			ok = 0;
			why = "decrypt accepted invalid tag";
		}
		if (ok) {
			printf("%s PASS\n", id);
			pass++;
		} else {
			printf("%s FAIL %s\n", id, why);
			fail++;
		}
next:
		free(key); free(iv); free(aad); free(pt); free(ct);
		free(tag); free(buf); free(buf2);
	}
	printf("SUMMARY aes=%s gcm=%s uio=%d pass=%d fail=%d unsup=%d\n", argv[1],
	    argv[2], uio_mode, pass, fail, unsup);
	return (fail != 0);
}
