/****************************************************************************
Generated from D:\source\S310\QCC3026_earbud_6284\apps\applications\earbud\qcc512x_qcc302x\CF376_CF440\dfu\dfu-public.key
by C:\qtil\ADK_QCC512x_QCC302x_WIN_6.2.84\tools\gen_rsa_pss_constants.py
at 14:24:00 30/07/2018

FILE NAME
    rsa_pss_constants.c

DESCRIPTION
    Constant data needed for the rsa_decrypt and the ce_pkcs1_pss_padding_verify
    function.

NOTES
    The constant data for the rsa_decrypt is generated by the host from the
    RSA public key and compiled into the CONFIG_HYDRACORE VM upgrade library.
    It is directly related to the RSA private and public key pair, and if they
    change then this file must be regenerated from the public key.

****************************************************************************/

#include "rsa_decrypt.h"

/*
 * The const rsa_mod_t *mod parameter for the rsa_decrypt function.
 */
const rsa_mod_t rsa_decrypt_constant_mod = {
    /* uint16 M[RSA_SIGNATURE_SIZE] where RSA_SIGNATURE_SIZE is 128 */
    {
        0xe566,     /* 00 */
        0x5dea,     /* 01 */
        0xd6d2,     /* 02 */
        0x63d2,     /* 03 */
        0x28a2,     /* 04 */
        0x6a5f,     /* 05 */
        0x6a4c,     /* 06 */
        0x80a6,     /* 07 */
        0x70c3,     /* 08 */
        0xecb5,     /* 09 */
        0x1d88,     /* 10 */
        0x1374,     /* 11 */
        0xd732,     /* 12 */
        0x3682,     /* 13 */
        0xe3c5,     /* 14 */
        0x96e3,     /* 15 */
        0xb5b2,     /* 16 */
        0x7228,     /* 17 */
        0x89d7,     /* 18 */
        0x3dbb,     /* 19 */
        0xe683,     /* 20 */
        0x9c28,     /* 21 */
        0xbd51,     /* 22 */
        0x0716,     /* 23 */
        0xb5e9,     /* 24 */
        0x5029,     /* 25 */
        0xdb1a,     /* 26 */
        0xa4b1,     /* 27 */
        0x5d83,     /* 28 */
        0xb02e,     /* 29 */
        0xfedb,     /* 30 */
        0xd04a,     /* 31 */
        0xfe88,     /* 32 */
        0x6f40,     /* 33 */
        0x31dc,     /* 34 */
        0x73a8,     /* 35 */
        0x9a21,     /* 36 */
        0x87f3,     /* 37 */
        0x22c2,     /* 38 */
        0xa13f,     /* 39 */
        0x6506,     /* 40 */
        0xefce,     /* 41 */
        0xb94d,     /* 42 */
        0xf5d6,     /* 43 */
        0xcaec,     /* 44 */
        0x8d6e,     /* 45 */
        0x0c49,     /* 46 */
        0x3e48,     /* 47 */
        0x762d,     /* 48 */
        0xe297,     /* 49 */
        0xf3d7,     /* 50 */
        0x1d7a,     /* 51 */
        0x45d4,     /* 52 */
        0x8794,     /* 53 */
        0x01a5,     /* 54 */
        0x83eb,     /* 55 */
        0xa2b9,     /* 56 */
        0xc6cb,     /* 57 */
        0x6a85,     /* 58 */
        0xe914,     /* 59 */
        0x328b,     /* 60 */
        0x0feb,     /* 61 */
        0xbd8d,     /* 62 */
        0x6bf6,     /* 63 */
        0x9cbc,     /* 64 */
        0x8934,     /* 65 */
        0x5c1b,     /* 66 */
        0x9c2e,     /* 67 */
        0x2cec,     /* 68 */
        0xda6d,     /* 69 */
        0x436d,     /* 70 */
        0x0a8d,     /* 71 */
        0x72cd,     /* 72 */
        0xe929,     /* 73 */
        0xb84b,     /* 74 */
        0xc546,     /* 75 */
        0xef17,     /* 76 */
        0x4843,     /* 77 */
        0x3cde,     /* 78 */
        0xf31b,     /* 79 */
        0x567c,     /* 80 */
        0x040a,     /* 81 */
        0x1e5f,     /* 82 */
        0x506a,     /* 83 */
        0x111d,     /* 84 */
        0x3c1f,     /* 85 */
        0xaad8,     /* 86 */
        0x7571,     /* 87 */
        0x4379,     /* 88 */
        0xc489,     /* 89 */
        0x8d70,     /* 90 */
        0x284d,     /* 91 */
        0xd2e7,     /* 92 */
        0x8629,     /* 93 */
        0xd57f,     /* 94 */
        0x3c6d,     /* 95 */
        0x2836,     /* 96 */
        0x6228,     /* 97 */
        0x3f70,     /* 98 */
        0x6676,     /* 99 */
        0xdbab,     /* 100 */
        0x5981,     /* 101 */
        0x5068,     /* 102 */
        0xf88e,     /* 103 */
        0x0e59,     /* 104 */
        0x631b,     /* 105 */
        0xa703,     /* 106 */
        0xa34f,     /* 107 */
        0xc384,     /* 108 */
        0xab48,     /* 109 */
        0x48e9,     /* 110 */
        0xed53,     /* 111 */
        0xcbe5,     /* 112 */
        0xbe14,     /* 113 */
        0xf860,     /* 114 */
        0x2eca,     /* 115 */
        0x50f6,     /* 116 */
        0xfb8c,     /* 117 */
        0x42a2,     /* 118 */
        0xfe0f,     /* 119 */
        0x740b,     /* 120 */
        0xb345,     /* 121 */
        0x60fb,     /* 122 */
        0xf857,     /* 123 */
        0x16a5,     /* 124 */
        0x5773,     /* 125 */
        0x072a,     /* 126 */
        0x744f      /* 127 */
    },
    /* uint16 M_dash */
    0xdd51
};

/* 
 * The "lump of memory passed in initialised to R^2N mod M" needed for the
 * uint16 *A parameter for the rsa_decrypt function. This must be copied into
 * a writable array of RSA_SIGNATURE_SIZE uint16 from here before use.
 */
const  uint16 rsa_decrypt_constant_sign_r2n[RSA_SIGNATURE_SIZE] = {
    0xb2b9,     /* 00 */
    0xf192,     /* 01 */
    0x0862,     /* 02 */
    0x68a8,     /* 03 */
    0x04e4,     /* 04 */
    0x2b9f,     /* 05 */
    0x82d0,     /* 06 */
    0xec3b,     /* 07 */
    0xbc4c,     /* 08 */
    0x2e27,     /* 09 */
    0xb282,     /* 10 */
    0x857c,     /* 11 */
    0xf8d9,     /* 12 */
    0x4294,     /* 13 */
    0x15a4,     /* 14 */
    0xb2d1,     /* 15 */
    0xb8fe,     /* 16 */
    0xef43,     /* 17 */
    0xa0e6,     /* 18 */
    0xbd0b,     /* 19 */
    0xbd89,     /* 20 */
    0xbc7b,     /* 21 */
    0xa4de,     /* 22 */
    0x6ef9,     /* 23 */
    0xc44c,     /* 24 */
    0x62d8,     /* 25 */
    0x070c,     /* 26 */
    0x90ea,     /* 27 */
    0x4d5f,     /* 28 */
    0x00a5,     /* 29 */
    0x99e2,     /* 30 */
    0x9616,     /* 31 */
    0x1355,     /* 32 */
    0x9dd1,     /* 33 */
    0x70b6,     /* 34 */
    0xf486,     /* 35 */
    0x5345,     /* 36 */
    0x5611,     /* 37 */
    0x1019,     /* 38 */
    0xead3,     /* 39 */
    0xaeca,     /* 40 */
    0xf92c,     /* 41 */
    0xd46f,     /* 42 */
    0xf511,     /* 43 */
    0x2020,     /* 44 */
    0xc09f,     /* 45 */
    0x69a2,     /* 46 */
    0x9934,     /* 47 */
    0xb51e,     /* 48 */
    0x7770,     /* 49 */
    0xd5b8,     /* 50 */
    0xc546,     /* 51 */
    0xf220,     /* 52 */
    0x9b71,     /* 53 */
    0x6cb3,     /* 54 */
    0xad24,     /* 55 */
    0xb66e,     /* 56 */
    0x7f57,     /* 57 */
    0xa5c6,     /* 58 */
    0xf547,     /* 59 */
    0x6b7b,     /* 60 */
    0xfd64,     /* 61 */
    0x108a,     /* 62 */
    0x1c5b,     /* 63 */
    0xeb42,     /* 64 */
    0xcc24,     /* 65 */
    0xadee,     /* 66 */
    0x5b4d,     /* 67 */
    0xf8c4,     /* 68 */
    0x6261,     /* 69 */
    0xc9b3,     /* 70 */
    0x444a,     /* 71 */
    0x1fde,     /* 72 */
    0x4eb7,     /* 73 */
    0x9d7f,     /* 74 */
    0x86d6,     /* 75 */
    0x6340,     /* 76 */
    0xad08,     /* 77 */
    0xa0ee,     /* 78 */
    0xef8e,     /* 79 */
    0x95e1,     /* 80 */
    0xe0a1,     /* 81 */
    0xe2cf,     /* 82 */
    0x9d56,     /* 83 */
    0xbac5,     /* 84 */
    0x2cb0,     /* 85 */
    0xe0cf,     /* 86 */
    0x0019,     /* 87 */
    0x85d2,     /* 88 */
    0x90e9,     /* 89 */
    0xe2aa,     /* 90 */
    0x104f,     /* 91 */
    0x6968,     /* 92 */
    0xd23c,     /* 93 */
    0x6ec7,     /* 94 */
    0xc550,     /* 95 */
    0xc925,     /* 96 */
    0x56c7,     /* 97 */
    0xcf9d,     /* 98 */
    0x1af9,     /* 99 */
    0x538c,     /* 100 */
    0xa50e,     /* 101 */
    0x33fb,     /* 102 */
    0x4d4b,     /* 103 */
    0x3937,     /* 104 */
    0x9764,     /* 105 */
    0x1a28,     /* 106 */
    0x86ea,     /* 107 */
    0xd8ee,     /* 108 */
    0xaebb,     /* 109 */
    0x35be,     /* 110 */
    0xbdb1,     /* 111 */
    0xced8,     /* 112 */
    0x3396,     /* 113 */
    0x3cfc,     /* 114 */
    0x075d,     /* 115 */
    0x59dd,     /* 116 */
    0xba8a,     /* 117 */
    0x39c5,     /* 118 */
    0xaa9f,     /* 119 */
    0x3eaf,     /* 120 */
    0x0403,     /* 121 */
    0x7ffd,     /* 122 */
    0x287e,     /* 123 */
    0x0e32,     /* 124 */
    0x8d9b,     /* 125 */
    0xa4bb,     /* 126 */
    0xb52f      /* 127 */
};

/*
 * The value to be provided for the saltlen parameter to the 
 * ce_pkcs1_pss_padding_verify function used to decode the PSS padded SHA-256
 * hash. It has to match the value that was used in the encoding process.
 */
const unsigned long ce_pkcs1_pss_padding_verify_constant_saltlen = 222;
