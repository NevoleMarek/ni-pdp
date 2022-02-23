mpiCC -fopenmp main.cpp -DTHREADS=2 -o main

as=(10 10 10 10 15 15 15 20 20 20)

i=0
for file in test/*; do
    mpirun -np 3 ./main $file ${as[i]}
    ((i=i+1))
done