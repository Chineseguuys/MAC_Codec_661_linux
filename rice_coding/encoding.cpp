/* Rice [en]coder
 * Demonstration code by Emil Mikulic, 2002.
 * http://purl.org/net/overload
 *
 * $Id: encode.c,v 1.2 2002/12/10 14:56:55 emikulic Exp $
 */

/**
 * https://www.firstpr.com.au/audiocomp/lossless/#rice
*/

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

FILE *fp;
unsigned char buff = 0;
int filled = 0;

/**
 * 每次满了 8 个bits(即一个字节)才写入文件。
 * 主要是因为 fwrite() 函数写入文件至少是一个字节
*/
void put_bit(unsigned char b)
{
    /**
     * 每次只将 b 的最右侧的比特放入到 buff 当中去
    */
	buff = buff | ((b & 1) << filled);
	if (filled == 7)
	{
		if (!fwrite(&buff, 1, 1, fp))
		{
			printf("\nerror writing to file!\n");
			exit(1);
		}

		buff = 0;
		filled = 0;
	}
	else
		filled++;
}



void rice_code(unsigned char x, int k)
{
	int m = 1 << k;
	int q = x / m;
	int i;

    /**
     * 可以选择在多个 0 的后面跟上终止1
     * 或者也可以选择多个 1 的会面跟上终止 0
    */
	for (i=0; i<q; i++) put_bit(1);
	put_bit(0);

	for (i=k-1; i>=0; i--) put_bit( (x >> i) & 1 );
}



int rice_len(unsigned char x, int k)
{
	int m = 1 << k;
	int q = x / m;
	return q + 1 + k;
}



int main(int argv, char **argc)
{
	unsigned int fs, i, k, outsize, best_size = 0, best_k;
	unsigned char *buf;
	struct stat stat;
	
	if (argv != 3)
	{
		printf("usage: %s <infile> <outfile>\n", argc[0]);
		return 1;
	}

	fp = fopen(argc[1], "rb");
	if (fp)
		printf("reading from %s ", argc[1]);
	else {
		printf("error opening %s!\n", argc[1]);
		return 1;
	}

	fstat(fileno(fp), &stat);
	fs = (unsigned int)stat.st_size;
	printf("(%d bits)\n", fs*8);

	buf = (unsigned char*) malloc(fs);
	if (!buf)
	{
		printf("can't allocate memory!\n");
		return 1;
	}

	if (!fread(buf, fs, 1, fp))
	{
		printf("error reading from file!\n");
		return 1;
	}
	fclose(fp);

	/* find length */
	for (k=0; k<8; k++)
	{
		printf("k = %d, ", k);
		fflush(stdout);

		outsize = 0;
		for (i=0; i<fs; i++)
			outsize += rice_len(buf[i], k);

		printf("size = %d bits\n", outsize);

		if (!best_size || outsize < best_size)
		{
			best_size = outsize;
			best_k = k;
		}
	}

	/* output */
	fp = fopen(argc[2], "wb");
	if (fp)
	{
		printf("writing to %s...", argc[2]);
		fflush(stdout);
	} else {
		printf("error opening %s!\n", argc[2]);
		return 1;
	}

	printf("(k = %d)...", best_k);
    /**
     * 把 best_k 的最右侧三个比特放入到 buff 当中去，这里存储的 k 值在 
     * decode 的时候有用
    */
	put_bit((best_k >> 2) & 1);
	put_bit((best_k >> 1) & 1);
	put_bit((best_k     ) & 1);

	for (i=0; i<fs; i++)
		rice_code(buf[i], best_k);

	/* flush */
	i = 8 - filled;
	while (i--) put_bit(1);

	fclose(fp);	
	free(buf);
	printf("done!\n");
	return 0;
}