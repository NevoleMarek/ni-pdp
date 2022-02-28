#include<iostream>
#include<fstream>
#include<vector>
#include<utility>
#include<omp.h>
#include<mpi.h>
#include<queue>

#ifndef THREADS
#define THREADS 1
#endif

#ifndef DEPTH
#define DEPTH 5
#endif

#define KILL -1
#define READY 1
#define TASK 2


using namespace std;

vector<vector<pair<int,int>>> loadGraph(char* filename){
    int n, weight;
    ifstream file(filename);
    file >> n;
    vector<vector<pair<int,int>>> graph(n);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            file >> weight;
            if(weight > 0)
                graph[i].push_back(make_pair(j, weight));
        }
    }
    file.close();
    return graph;
}

void sendGraph(char* filename, int comm_size, int a){
    int n;

    ifstream file(filename);
    file >> n;
    int16_t data[n * n];
    for (int i = 0; i < n * n; i++)
    {
        file >> data[i];
    }
    file.close();



    for (int i = 1; i < comm_size; i++)
    {
        MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&data, n*n, MPI_SHORT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&a, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
}

vector<vector<pair<int,int>>> receiveGraph(){
    int n;
    MPI_Recv(&n, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int16_t data[n * n];
    MPI_Recv(&data, n * n, MPI_SHORT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


    vector<vector<pair<int,int>>> graph(n);
    for (int i = 0; i < n*n; i++)
    {
        if(data[i] > 0)
            graph[i/n].push_back(make_pair(i%n, data[i]));

    }
    return graph;
}



class Solver{
private:
    int a;
    vector<vector<pair<int,int>>> graph;

    int minCost;
    vector<vector<int>> solutions;

    int computeCost(int i, vector<int> &vertices);
    int recomputeCostAfterAddingVertex(int i, vector<int> &vertices);
    int estimateRemainingCost(int i, vector<int> &vertices);
    int computeCostIfRestInY(int i, vector<int> &vertices);
    void bbDFS(int i, int n, int cost, vector<int> &vertices);
public:
    Solver(int a, vector<vector<pair<int,int>>> &graph);
    void masterSolve();
    void slaveSolve(int i, vector<int> &vertices);
    int getMinCost();
};

Solver::Solver(int a, vector<vector<pair<int,int>>> &graph){
    this->a = a;
    this->graph = graph;
    this->minCost = INT32_MAX;
    this->solutions.clear();
}

int Solver::computeCost(int i, vector<int> &vertices){
    int cost = 0;
    for (int j = 0; j < i; j++)
        if(vertices[j]==1)
            for (auto &&p : this->graph[j])
                if(p.first < i && vertices[p.first] == 0)
                    cost += p.second;
    return cost;
}

int Solver::recomputeCostAfterAddingVertex(int i, vector<int> &vertices){
    int cost = 0;
    int inXCost = 0, inYCost = 0;
    for (auto &&p : this->graph[i])
    {
        if(p.first <= i){
            if(vertices[p.first] == 0)
                inXCost += p.second;
            else
                inYCost += p.second;
        }
    }
    cost += vertices[i] == 1? inXCost: inYCost;
    return cost;
}

int Solver::estimateRemainingCost(int i, vector<int> &vertices){
    int remainingCost = 0;
    for (unsigned int j = i + 1; j < vertices.size(); j++)
    {
        int inXCost = 0, inYCost = 0;
        for (auto &&p : this->graph[j])
        {
            if(p.first <= i){
                if(vertices[p.first] == 0)
                    inXCost += p.second;
                else
                    inYCost += p.second;
            }
        }
        remainingCost += inXCost < inYCost ? inXCost : inYCost;
    }
    return remainingCost;
}

int Solver::computeCostIfRestInY(int i, vector<int> &vertices){
    int finalCost = 0;
    for (unsigned int j = i; j < vertices.size(); j++)
    {
        for (auto &&p : this->graph[j])
        {
            if(vertices[p.first] == 1)
                finalCost += p.second;
        }
    }
    return finalCost;
}

void Solver::bbDFS(int i, int n, int cost, vector<int> &vertices){
    if((this->a - n + i) > vertices.size())
        return;

    if(n == this->a){
        cost = cost + computeCostIfRestInY(i, vertices);
        if(cost <= this->minCost)
        #pragma omp critical
        {
            if(cost < this->minCost){
                this->minCost = cost;
                this->solutions.clear();
                this->solutions.push_back(vector<int>(vertices));
            }
            else if (cost == this->minCost){
                this->solutions.push_back(vector<int>(vertices));
            }
        }
        return;
    }

    int remainingCost = estimateRemainingCost(i, vertices);
    if((cost + remainingCost) > this->minCost)
        return;


    vertices[i] = 1;
    int newCost = cost + recomputeCostAfterAddingVertex(i, vertices);
    bbDFS(i+1, n+1, newCost, vertices);

    vertices[i] = 0;
    newCost = cost + recomputeCostAfterAddingVertex(i, vertices);
    bbDFS(i+1, n, newCost, vertices);
}

void Solver::masterSolve(){
    vector<pair<vector<int>,int>> tasks;
    int depth = DEPTH;
    int i = 0;
    queue<pair<vector<int>,int>> q;
    if(this->a * 2 == this->graph.size())
        q.push(make_pair(vector<int>(this->graph.size(), 0),1));
    else
        q.push(make_pair(vector<int>(this->graph.size(), 0),0));

    q.push(make_pair(vector<int>(),-1));
    while(depth !=0){
        auto p = q.front();
        q.pop();
        if(p.second != -1){
            auto v1 = p.first;
            auto v2 = p.first;
            v1[i] = 1;
            v2[i] = 0;

                q.push(make_pair(v1,p.second + 1));
                q.push(make_pair(v2,p.second + 1));
        }
        else{
            if(depth != 1)
                q.push(make_pair(vector<int>(),-1));
            i++;
            depth--;
        }
    }

    int msg;
    MPI_Status status;
    while(!q.empty()){
        //cout << "Tasks left: " << q.size() << endl;
        MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        msg = TASK;
        //cout << "Sending task to: " << status.MPI_SOURCE << endl;
        MPI_Send(&msg, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        auto p = q.front();
        MPI_Send(&p.first[0], p.first.size(), MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        MPI_Send(&p.second, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
        q.pop();
    }

    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    msg = KILL;
    for (int i = 1; i < comm_size; i++)
    {
        MPI_Send(&msg, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    int pMinCost;
    for (int i = 1; i < comm_size; i++)
    {
        MPI_Recv(&pMinCost, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&pMinCost, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(pMinCost < this->minCost)
            this->minCost = pMinCost;
    }
}

void Solver::slaveSolve(int i, vector<int> &vertices){
    vector<pair<vector<int>,int>> tasks;
    int depth = DEPTH;
    queue<pair<vector<int>,int>> q;
    q.push(make_pair(vertices,i));
    q.push(make_pair(vector<int>(),-1));
    while(depth !=0){
        auto p = q.front();
        q.pop();
        if(p.second != -1){
            auto v1 = p.first;
            auto v2 = p.first;
            v1[i] = 1;
            v2[i] = 0;
            if(depth == 1){
                tasks.push_back(make_pair(v1,p.second + 1));
                tasks.push_back(make_pair(v2,p.second + 1));
            }
            else{
                q.push(make_pair(v1,p.second + 1));
                q.push(make_pair(v2,p.second + 1));
            }
        }
        else{
            q.push(make_pair(vector<int>(),-1));
            i++;
            depth--;
        }
    }

    #pragma omp parallel for schedule(dynamic) num_threads(THREADS)
    for (int i = 0; i < tasks.size(); i++)
    {
        int n = 0;

        for (int j = 0; j < tasks[i].second; j++)
        {
            if(tasks[i].first[j] == 1)
                n++;
        }

        int cost = computeCost(tasks[i].second, tasks[i].first);
        bbDFS(tasks[i].second, n, cost, tasks[i].first);
    }
}

int Solver::getMinCost(){
    return this->minCost;
}

int main(int argc, char* argv[]){
    if(argc != 3){
        cout<<"Arguments missing"<<endl;
        return 1;
    }

    MPI_Init(&argc, &argv);

    int comm_size;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double start, end;

    if(rank == 0)
    {
        int a = stoi(argv[2]);
        start = MPI_Wtime();

        sendGraph(argv[1], comm_size, a);
        vector<vector<pair<int,int>>> graph = loadGraph(argv[1]);
        Solver solver(a, graph);
        solver.masterSolve();
	cout << argv[1] << " " << solver.getMinCost() << " ";
        end = MPI_Wtime();
    }
    else
    {
        int a;
        vector<vector<pair<int,int>>> graph = receiveGraph();
        MPI_Recv(&a, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        Solver solver(a, graph);

        bool finished = false;
        int msg;

        while(!finished){
            msg = READY;
            MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(msg == TASK){
                vector<int> vertices;
                vertices.resize(graph.size());
                int i;
                MPI_Recv(&vertices[0], vertices.size(), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&i, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                solver.slaveSolve(i, vertices);
            }
            else if(msg == KILL){
                finished = true;
                //cout << "Process "<< rank << " has been killed" << endl;
                int best = solver.getMinCost();
                //cout << "Process "<< rank << " best cost is " << best << endl;
                MPI_Send(&best, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }
    }

    if(rank == 0){
        cout << end-start<< endl;
    }

    MPI_Finalize();
    return 0;
}
