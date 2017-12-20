#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BYTE unsigned char
#define BLOCKSIZE 16

int pic_len;

void printBytes(BYTE b[], int len) {
    int i;
    for (i=0; i<len; i++)
    printf("%d ", b[i]);
    printf("\n");
}

/******************************************************************************/

// The following lookup tables and functions are for internal use only!
BYTE AES_Sbox[] = {99,124,119,123,242,107,111,197,48,1,103,43,254,215,171,
    118,202,130,201,125,250,89,71,240,173,212,162,175,156,164,114,192,183,253,
    147,38,54,63,247,204,52,165,229,241,113,216,49,21,4,199,35,195,24,150,5,154,
    7,18,128,226,235,39,178,117,9,131,44,26,27,110,90,160,82,59,214,179,41,227,
    47,132,83,209,0,237,32,252,177,91,106,203,190,57,74,76,88,207,208,239,170,
    251,67,77,51,133,69,249,2,127,80,60,159,168,81,163,64,143,146,157,56,245,
    188,182,218,33,16,255,243,210,205,12,19,236,95,151,68,23,196,167,126,61,
    100,93,25,115,96,129,79,220,34,42,144,136,70,238,184,20,222,94,11,219,224,
    50,58,10,73,6,36,92,194,211,172,98,145,149,228,121,231,200,55,109,141,213,
    78,169,108,86,244,234,101,122,174,8,186,120,37,46,28,166,180,198,232,221,
    116,31,75,189,139,138,112,62,181,102,72,3,246,14,97,53,87,185,134,193,29,
    158,225,248,152,17,105,217,142,148,155,30,135,233,206,85,40,223,140,161,
    137,13,191,230,66,104,65,153,45,15,176,84,187,22};

BYTE AES_ShiftRowTab[] = {0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11};

BYTE AES_Sbox_Inv[256];
BYTE AES_ShiftRowTab_Inv[16];
BYTE AES_xtime[256];

__device__ void AES_SubBytes(BYTE state[], BYTE sbox[]) {
    int i;
    for(i = 0; i < 16; i++)
        state[i] = sbox[state[i]];
}

__device__ void AES_AddRoundKey(BYTE state[], BYTE rkey[]) {
    int i;
    for(i = 0; i < 16; i++)
        state[i] ^= rkey[i];
}

__device__ void AES_ShiftRows(BYTE state[], BYTE shifttab[]) {
    BYTE h[16];
    memcpy(h, state, 16);
    int i;
    for(i = 0; i < 16; i++)
        state[i] = h[shifttab[i]];
}

__device__ void AES_MixColumns(BYTE state[], BYTE aes_xtime[]) {
    int i;
    for(i = 0; i < 16; i += 4) {
        BYTE s0 = state[i + 0], s1 = state[i + 1];
        BYTE s2 = state[i + 2], s3 = state[i + 3];
        BYTE h = s0 ^ s1 ^ s2 ^ s3;
        state[i + 0] ^= h ^ aes_xtime[s0 ^ s1];
        state[i + 1] ^= h ^ aes_xtime[s1 ^ s2];
        state[i + 2] ^= h ^ aes_xtime[s2 ^ s3];
        state[i + 3] ^= h ^ aes_xtime[s3 ^ s0];
    }
}

__device__ void AES_MixColumns_Inv(BYTE state[], BYTE aes_xtime[]) {
    int i;
    for(i = 0; i < 16; i += 4) {
        BYTE s0 = state[i + 0], s1 = state[i + 1];
        BYTE s2 = state[i + 2], s3 = state[i + 3];
        BYTE h = s0 ^ s1 ^ s2 ^ s3;
        BYTE xh = aes_xtime[h];
        BYTE h1 = aes_xtime[aes_xtime[xh ^ s0 ^ s2]] ^ h;
        BYTE h2 = aes_xtime[aes_xtime[xh ^ s1 ^ s3]] ^ h;
        state[i + 0] ^= h1 ^ aes_xtime[s0 ^ s1];
        state[i + 1] ^= h2 ^ aes_xtime[s1 ^ s2];
        state[i + 2] ^= h1 ^ aes_xtime[s2 ^ s3];
        state[i + 3] ^= h2 ^ aes_xtime[s3 ^ s0];
    }
}

// AES_Init: initialize the tables needed at runtime.
// Call this function before the (first) key expansion.
void AES_Init() {
    int i;
    for(i = 0; i < 256; i++)
        AES_Sbox_Inv[AES_Sbox[i]] = i;

    for(i = 0; i < 16; i++)
        AES_ShiftRowTab_Inv[AES_ShiftRowTab[i]] = i;

    for(i = 0; i < 128; i++) {
        AES_xtime[i] = i << 1;
        AES_xtime[128 + i] = (i << 1) ^ 0x1b;
    }
}

// AES_Done: release memory reserved by AES_Init.
// Call this function after the last encryption/decryption operation.
void AES_Done() {}

/* AES_ExpandKey: expand a cipher key. Depending on the desired encryption
strength of 128, 192 or 256 bits 'key' has to be a byte array of length
16, 24 or 32, respectively. The key expansion is done "in place", meaning
that the array 'key' is modified.
*/
int AES_ExpandKey(BYTE key[], int keyLen) {
    int kl = keyLen, ks, Rcon = 1, i, j;
    BYTE temp[4], temp2[4];
    switch (kl) {
        case 16: ks = 16 * (10 + 1); break;
        case 24: ks = 16 * (12 + 1); break;
        case 32: ks = 16 * (14 + 1); break;
        default:
            printf("AES_ExpandKey: Only key lengths of 16, 24 or 32 bytes allowed!");
    }
    for(i = kl; i < ks; i += 4) {
        memcpy(temp, &key[i-4], 4);
        if (i % kl == 0) {
            temp2[0] = AES_Sbox[temp[1]] ^ Rcon;
            temp2[1] = AES_Sbox[temp[2]];
            temp2[2] = AES_Sbox[temp[3]];
            temp2[3] = AES_Sbox[temp[0]];
            memcpy(temp, temp2, 4);
            if ((Rcon <<= 1) >= 256)
                Rcon ^= 0x11b;
        }
        else if ((kl > 24) && (i % kl == 16)) {
            temp2[0] = AES_Sbox[temp[0]];
            temp2[1] = AES_Sbox[temp[1]];
            temp2[2] = AES_Sbox[temp[2]];
            temp2[3] = AES_Sbox[temp[3]];
            memcpy(temp, temp2, 4);
        }
        for(j = 0; j < 4; j++)
            key[i + j] = key[i + j - kl] ^ temp[j];
    }
    return ks;
}

// AES_Encrypt: encrypt the 16 byte array 'block' with the previously expanded key 'key'.
__global__ void AES_Encrypt(BYTE block[], BYTE key[], int keyLen, BYTE aes_sbox[], BYTE aes_shiftrowtab[], BYTE aes_xtime[]) {
    int l = keyLen, i;
    int j = threadIdx.x;
    int k = j + blockIdx.x * 16;

    BYTE device_block[16];
    for( int idx = k*16; idx < k*16+16; idx++){
        device_block[idx - k*16] = block[idx];
    }

    // printBytes(block, 16);
    AES_AddRoundKey(device_block, &key[0]);
    for(i = 16; i < l - 16; i += 16) {
        AES_SubBytes(device_block, aes_sbox);
        AES_ShiftRows(device_block, aes_shiftrowtab);
        AES_MixColumns(device_block, aes_xtime);
        AES_AddRoundKey(device_block, &key[i]);
    }
    AES_SubBytes(device_block, aes_sbox);
    AES_ShiftRows(device_block, aes_shiftrowtab);
    AES_AddRoundKey(device_block, &key[i]);

    for( int idx = k*16; idx < k*16+16; idx++ ){
        block[idx] = device_block[idx - k*16];
    }
}

// AES_Decrypt: decrypt the 16 byte array 'block' with the previously expanded key 'key'.
__global__ void AES_Decrypt(BYTE block[], BYTE key[], int keyLen, BYTE aes_xtime[],BYTE aes_shiftrowtab_inv[], BYTE aes_sbox_inv[]) {
    int l = keyLen, i;
    AES_AddRoundKey(block, &key[l - 16]);
    AES_ShiftRows(block, aes_shiftrowtab_inv);
    AES_SubBytes(block, aes_sbox_inv);
    for(i = l - 32; i >= 16; i -= 16) {
        AES_AddRoundKey(block, &key[i]);
        AES_MixColumns_Inv(block, aes_xtime);
        AES_ShiftRows(block, aes_shiftrowtab_inv);
        AES_SubBytes(block, aes_sbox_inv);
    }
    AES_AddRoundKey(block, &key[0]);
}

BYTE* readFile(char *filename){
    FILE *file;
    BYTE *buffer;
    unsigned long fileLen;

    // Open file
    file = fopen(filename, "rb");
    if(!file){
        fprintf(stderr, "Unable to open file %s", filename);
        return 0;
    }

    // Get file lengths
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (BYTE*)malloc(fileLen + 1);
    if(!buffer){
        fprintf(stderr, "Memory error!");
        fclose(file);
        return 0;
    }

    fread(buffer, fileLen, sizeof(BYTE), file);
    fclose(file);

    pic_len = fileLen;

    return buffer;
}

// ===================== test ============================================
int main() {
    // int i;
    AES_Init();

    // BYTE block[16];
    // for(i = 0; i < 16; i++)
    //     block[i] = 0x11 * i;

    BYTE *pic;
    pic = readFile("minions.jpg");

    if( !pic ){
        fprintf(stderr, "Memory creation error");
    }
    else{
        // delete jpg header (11 bytes)
        for( int i = 11; i < pic_len; i++ ){
            pic[i-11] = pic[i];
        }
        printf("%d\n", pic_len);
        // delete the last header (2 bytes)
        pic_len -= 11;
        pic[pic_len-1] = 0;
        pic[pic_len-2] = 0;
        pic_len -= 2;
        pic = (BYTE*)realloc(pic, pic_len);
        printf("%d\n", pic_len);

        // align the last block
        // if( pic_len % 16 ){
        //     int align_num = 16 - pic_len % 16;
        //     pic_len += align_num;
        //     pic = (BYTE*)realloc(pic, pic_len);
        //     for( int j = 0; j < align_num; j++ ){
        //         pic[pic_len - j - 1] = 1;
        //     }
        // }



        // allocate the cuda device space
        BYTE *pic_d, *key_d, *AES_Sbox_d, *AES_ShiftRowTab_d, *AES_Sbox_Inv_d, *AES_ShiftRowTab_Inv_d, *AES_xtime_d, *encrypt_result;
        BYTE key[16 * (14 + 1)];

        int keyLen = 32, maxKeyLen=16 * (14 + 1);
        for( int j = 0; j < keyLen; j++ ){
            key[j] = j;
        }
        int expandKeyLen = AES_ExpandKey(key, keyLen);

        cudaMalloc( &pic_d, pic_len*sizeof(BYTE) );
        cudaMemcpy( pic_d, pic, pic_len*sizeof(BYTE), cudaMemcpyHostToDevice);
        cudaMalloc( &key_d, expandKeyLen);
        cudaMemcpy( key_d, key, expandKeyLen*sizeof(BYTE), cudaMemcpyHostToDevice);

        cudaMalloc( &AES_Sbox_d, sizeof(AES_Sbox)/sizeof(BYTE));
        cudaMemcpy( AES_Sbox_d, AES_Sbox, sizeof(AES_Sbox)/sizeof(BYTE), cudaMemcpyHostToDevice);
        cudaMalloc( &AES_ShiftRowTab_d, sizeof(AES_ShiftRowTab)/sizeof(BYTE));
        cudaMemcpy( AES_ShiftRowTab_d, AES_ShiftRowTab, sizeof(AES_ShiftRowTab)/sizeof(BYTE), cudaMemcpyHostToDevice);
        cudaMalloc( &AES_Sbox_Inv_d, sizeof(AES_Sbox_Inv)/sizeof(BYTE));
        cudaMemcpy( AES_Sbox_Inv_d, AES_Sbox_Inv, sizeof(AES_Sbox_Inv)/sizeof(BYTE), cudaMemcpyHostToDevice);
        cudaMalloc( &AES_ShiftRowTab_Inv_d, sizeof(AES_ShiftRowTab_Inv)/sizeof(BYTE));
        cudaMemcpy( AES_ShiftRowTab_Inv_d, AES_ShiftRowTab_Inv, sizeof(AES_ShiftRowTab_Inv)/sizeof(BYTE), cudaMemcpyHostToDevice);
        cudaMalloc( &AES_xtime_d, sizeof(AES_xtime)/sizeof(BYTE));
        cudaMemcpy( AES_xtime_d, AES_xtime, sizeof(AES_xtime)/sizeof(BYTE), cudaMemcpyHostToDevice);


        /**
         * Block Size: 16 thread
         * Grid Size: BlockNum, 1
         **/
        int blockNum;
        if( pic_len % 16 ){
            blockNum = 1 + pic_len / 16;
        }
        else{
            blockNum = pic_len / 16;
        }

        dim3 dimGrid(blockNum, 1);
        dim3 dimBlock(16, 1);

        AES_Encrypt<<<dimGrid, dimBlock>>>(pic_d, key_d, expandKeyLen, AES_Sbox_d, AES_ShiftRowTab_d, AES_xtime_d);

        /* get the encrypt result */
        encrypt_result = (BYTE*)malloc(sizeof(BYTE)*pic_len);
        cudaMemcpy( encrypt_result, pic_d, pic_len*sizeof(BYTE), cudaMemcpyDeviceToHost);
        printf("%d\n", pic_len);
        int j = 0;
        while( j < pic_len){
            printf("%02x ", (BYTE)encrypt_result[j]);
            j++;
            if( !(j%16) ) printf("\n");
        }
        cudaFree(pic_d);
        cudaFree(key_d);
        cudaFree(AES_Sbox_d);
        cudaFree(AES_ShiftRowTab_d);
        cudaFree(AES_Sbox_Inv_d);
        cudaFree(AES_ShiftRowTab_Inv_d);
        cudaFree(AES_xtime_d);
    }

    // printf("原始訊息："); printBytes(block, 16);

    // BYTE key[16 * (14 + 1)];
    // int keyLen = 32, maxKeyLen=16 * (14 + 1), blockLen = 16;
    // for(i = 0; i < keyLen; i++)
    //     key[i] = i;

    // printf("原始金鑰："); printBytes(key, keyLen);

    // int expandKeyLen = AES_ExpandKey(key, keyLen);

    // printf("展開金鑰："); printBytes(key, expandKeyLen);

    // AES_Encrypt(block, key, expandKeyLen);

    // printf("加密完後："); printBytes(block, blockLen);

    // AES_Decrypt(block, key, expandKeyLen, aes_xtime_d);

    // printf("解密完後："); printBytes(block, blockLen);

    // AES_Done();
}
