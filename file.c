#include <stdio.h>
#include <stdlib.h>

#define PAGES_COUNT (26*26)

int main(int argc, char **argv)
{
    int i = 0;
    char pages[PAGES_COUNT][3], c1, c2;
    FILE *output;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <pathname_of_batch_file>\n", argv[0]);
        exit(1);
    }

    for (c1 = 'A'; c1 <= 'Z'; c1++)
    {
        for (c2 = 'A'; c2 <= 'Z'; c2++)
        {
            pages[i][0] = c1;
            pages[i][1] = c2;
            pages[i][2] = '\0';
            i++;
        }
    }

    output = fopen(argv[1], "w");
    if (output == NULL)
    {
        perror("output file can not open");
        exit (1);
    }
    
    for (i = 0; i < PAGES_COUNT; i++)
    {
        fprintf(output, "wget https://en.wikipedia.org/wiki/%s -O %s.html & \n", pages[i], pages[i]);
    }
    fprintf(output, "barrier\n");

    
    for (i = 0; i < PAGES_COUNT; i++)
    {
        fprintf(output, "html2txt %s.html > %s.txt & \n", pages[i], pages[i]);
    }
    fprintf(output, "barrier\n");

  
    fprintf(output, "grep -oh \"[a-zA-Z]*\" ");
    for (i = 0; i < PAGES_COUNT; i++)
    {
        fprintf(output, "%s.txt ", pages[i]);
    }
    fprintf(output, " > allwords.txt\n");


    fprintf(output, "sort -o allwords_sorted.txt allwords.txt\n");
    fprintf(output, "uniq -ic allwords_sorted.txt count_uniqwords.txt\n");

    fprintf(output, "sort -k 1,1n count_uniqwords.txt\n");

    fclose(output);

    return 0;
}
