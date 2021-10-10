/* Rice [de]coder
 * Demonstration code by Emil Mikulic, 2002.
 * http://purl.org/net/overload
 *
 * $Id: decode.c,v 1.2 2002/12/10 14:56:22 emikulic Exp $
 */

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

FILE *fin, *fout;
unsigned char buff;
int done = 0, filled = 0;

unsigned char get_bit(void)
{
	unsigned char tmp;

	if (!filled)
	{
		if (!fread(&buff, 1, 1, fin))
		{
			done = 1;
			return 0;
		}
		filled = 8;
	}

	tmp = buff & 1;
	buff = buff >> 1;
	filled--;

	return tmp;	
}



int main(int argv, char **argc)
{
	unsigned int k;
	unsigned char *buf;

	if (argv != 3)
	{
		printf("usage: %s <infile> <outfile>\n", argc[0]);
		return 1;
	}

	fin = fopen(argc[1], "rb");
	if (fin)
		printf("reading from %s... ", argc[1]);
	else {
		printf("error opening %s!\n", argc[1]);
		return 1;
	}

	k = (get_bit() << 2) | (get_bit() << 1) | get_bit();
	if (done)
	{
		printf("couldn't read three bits!\n");
		return 1;
	}
	printf("k = %d\n", k);

	fout = fopen(argc[2], "wb");
	if (fout)
	{
		printf("writing to %s\n", argc[2]);
		fflush(stdout);
	} else {
		printf("error opening %s!\n", argc[2]);
		return 1;
	}

	while (1)
	{
		int m = 1 << k, q = 0, x, i;

		while (get_bit()) q++;
		x = m * q;

		for (i=k-1; i>=0; i--) x = x | (get_bit() << i);

		if (done) break;	
		if (!fwrite(&x, 1, 1, fout))
		{
			printf("error writing to file!\n");
			return 1;
		}
	}

	fclose(fin);
	fclose(fout);
	printf("done!\n");
	return 0;
}