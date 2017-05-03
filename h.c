// Created Time:    2017-04-27 21:30:30
// Modified Time:   2017-04-27 21:40:34




#include <stdio.h>
#include <string.h>
void getResult(long ulDataInput, char *string)
{
        int temp, array[30];
            int i, j;
                char a[7];
                    for(i = 2, j = 0; i < ulDataInput; i++)
                                if(ulDataInput % i == 0)
                                            {
                                                            array[j] = i;
                                                                        ulDataInput = ulDataInput / i;
                                                                                    i = 1;
                                                                                                j++;
                                                                                                        }
                        
                        if(j == 0)
                                {
                                            array[0] = ulDataInput;
                                                        }
                        else
                            array[j] = ulDataInput;
                            for(i = 0; i <= j; i++)
                                    {
                                                sprintf(a, "%d ", array[i]);
                                                        strcat(string, a);
                                                            }
                                return;
}
    
int
main(void)
{
        long value;
            char result[50];
                while(scanf("%d", &value) != EOF)
                        {
                            result[0] = '\0';
                                    getResult(value, result);
                                            printf("%s\n", result);
                                                }
                    
                    return 0;
}
