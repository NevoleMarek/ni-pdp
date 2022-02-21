#include<iostream>
#include<fstream>
#include<vector>
#include<utility>
#include<omp.h>
#include<queue>

#ifndef THREADS
#define THREADS 1
#endif


using namespace std;

vector<vector<pair<int,int>>> loadGraph(char* filename){
    ifstream file(filename);
    int n, weight;
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
    void solve();
};

Solver::Solver(int a, vector<vector<pair<int,int>>> &graph){
    this->a = a;
    this->graph = graph;
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
    /*
    cout <<"i: "<< i <<" n: "<< n <<endl;
    for (auto &&p : this->vertices)
    {
        cout << p << " ";
    }
    cout << endl;
    */
    if((unsigned int)(this->a - n + i) > vertices.size())
        return;

    if(cost > this->minCost){
        return;
    }

    if(n == this->a){
        cost = cost + computeCostIfRestInY(i, vertices);
        if(cost < this->minCost)
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

    if(n < this->a){
        vertices[i] = 1;
        int newCost = cost + recomputeCostAfterAddingVertex(i, vertices);
        bbDFS(i+1, n+1, newCost, vertices);
    }

    vertices[i] = 0;
    int newCost = cost + recomputeCostAfterAddingVertex(i, vertices);
    bbDFS(i+1, n, newCost, vertices);
}

void Solver::solve(){
    this->minCost = INT32_MAX;
    this->solutions.clear();
    double s = omp_get_wtime();

    #pragma omp parallel num_threads(THREADS)
    {
        #pragma omp single
        {
            vector<pair<vector<int>,int>> tasks;
            int depth = 4;
            int i = 0;
            queue<pair<vector<int>,int>> q;
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
            for (auto &&task : tasks)
            {
                #pragma omp task
                {
                    int n = 0;
                    for (int i = 0; i < task.second; i++)
                    {
                        if(task.first[i] == 1)
                            n++;
                    }
                    int cost = computeCost(task.second, task.first);
                    bbDFS(task.second, n, cost, task.first);
                }
            }
        }
    }

    double timeElapsed = omp_get_wtime() - s;
    cout <<" "<< timeElapsed <<" " << this->minCost << " "<< this->solutions.size() << endl;
    /*
    for (auto &&sol : this->solutions)
    {
        for (auto &&v : sol)
        {
            cout << v << " ";
        }
        cout<<endl;
    }
    */
}

int main(int argc, char* argv[]){
    if(argc != 3){
        cout<<"Arguments missing"<<endl;
        return 1;
    }
    //cout << "Threads: " << THREADS << endl;
    int a = stoi(argv[2]);
    cout << argv[1]<< " " << argv[2];
    vector<vector<pair<int,int>>> graph = loadGraph(argv[1]);
    Solver solver(a, graph);
    solver.solve();

    return 0;
}