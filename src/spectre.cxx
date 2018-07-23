// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "spectreconfig.h"

int main (int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stdout,"%s Version %d.%d.%d (%s)\n",
                argv[0],
                Spectre_VERSION_MAJOR,
                Spectre_VERSION_MINOR,
                Spectre_VERSION_BUGFIX,
                Spectre_COMMIT_HASH);
        fprintf(stdout,"Usage: %s number\n",argv[0]);
        return 1;
    }
    double inputValue = atof(argv[1]);
    double outputValue = sqrt(inputValue);
    fprintf(stdout,"The square root of %g is %g\n",
            inputValue, outputValue);
    return 0;
}