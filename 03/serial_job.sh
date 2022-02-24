#!/bin/sh

#  ===========================================================================
# |                                                                           |
# |             COMMAND FILE FOR SUBMITTING SGE JOBS                          |
# |                                                                           |
# |                                                                           |
# | SGE keyword statements begin with #$                                      |
# |                                                                           |
# | Comments begin with #                                                     |
# | Any line whose first non-blank character is a pound sign (#)              |
# | and is not a SGE keyword statement is regarded as a comment.              |
#  ===========================================================================

# Request Bourne shell as shell for job
#$ -S /bin/sh

# Execute the job from the current working directory.
#$ -cwd

# Defines  or  redefines  the  path used for the standard error stream of the job.
#$ -e ./results

# The path used for the standard output stream of the job.
#$ -o ./results

# Do not change.
#$ -pe ompi 1

thds=(1 2 4 6 8 12 16 20) 
for t in 1 2 4 6 8 12 16 20; do 
	g++ -std=c++11 -DDEPTH=11 -DTHREADS=$t -fopenmp -Ofast main.cpp -o main
	./main test/graf_40_13.txt 20
done
