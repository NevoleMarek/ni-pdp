#include<iostream>
#include<fstream>
#include<vector>
#include<array>
#include<utility>
#include"../time.cpp"

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
    vector<int> vertices;

    int callCount;
    int minCost;
    vector<vector<int>> solutions;

    int computeCost(int i);
    int estimateRemainingCost(int i);
    int computeCostIfRestInY(int i);
    void bbDFS(int i, int n, int cost);
public:
    Solver(int a, vector<vector<pair<int,int>>> &graph);
    void solve();
};

Solver::Solver(int a, vector<vector<pair<int,int>>> &graph){
    this->a = a;
    this->graph = graph;
}

int Solver::computeCost(int i){
    int cost = 0;
    int inXCost = 0, inYCost = 0;
    for (auto &&p : this->graph[i])
    {
        if(p.first <= i){
            if(this->vertices[p.first] == 0)
                inXCost += p.second;
            else
                inYCost += p.second;
        }
    }
    cost += this->vertices[i] == 1? inXCost: inYCost;
    return cost;
}

int Solver::estimateRemainingCost(int i){
    int remainingCost = 0;
    for (int j = i + 1; j < this->vertices.size(); j++)
    {
        int inXCost = 0, inYCost = 0;
        for (auto &&p : this->graph[j])
        {
            if(p.first <= i){
                if(this->vertices[p.first] == 0)
                    inXCost += p.second;
                else
                    inYCost += p.second;
            }
        }
        remainingCost += inXCost < inYCost ? inXCost : inYCost;
    }
    return remainingCost;
}

int Solver::computeCostIfRestInY(int i){
    int finalCost = 0;
    for (int j = i; j < this->vertices.size(); j++)
    {
        for (auto &&p : this->graph[j])
        {
            if(this->vertices[p.first] == 1)
                finalCost += p.second;
        }
    }
    return finalCost;
}

void Solver::bbDFS(int i, int n, int cost){
    this->callCount ++;
    /*
    cout <<"i: "<< i <<" n: "<< n <<endl;
    for (auto &&p : this->vertices)
    {
        cout << p << " ";
    }
    cout << endl;
    */
    if((this->a - n + i) > this->vertices.size())
        return;

    if(n == this->a){
        cost = cost + computeCostIfRestInY(i);
        if(cost < this->minCost){
            this->minCost = cost;
            this->solutions.clear();
            this->solutions.push_back(vector<int>(this->vertices));
        }
        else if (cost == this->minCost)
            this->solutions.push_back(vector<int>(this->vertices));
        return;
    }

    int remainingCost = estimateRemainingCost(i);
    if((cost + remainingCost) > this->minCost)
        return;

    if(n < this->a){
        this->vertices[i] = 1;
        int newCost = cost + computeCost(i);
        bbDFS(i+1, n+1, newCost);
    }

    this->vertices[i] = 0;
    int newCost = cost + computeCost(i);
    bbDFS(i+1, n, newCost);
}

void Solver::solve(){
    this->callCount = 0;
    this->minCost = INT32_MAX;
    this->solutions.clear();
    this->vertices = vector<int>(this->graph.size(), 0);
    float s = GetMyCPUTime();
    bbDFS(0, 0, 0);
    float timeElapsed = GetMyCPUTime() - s;
    cout <<" "<< timeElapsed <<" "<< this->callCount<<" " << this->minCost << " "<< this->solutions.size() << endl;
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
    int a = stoi(argv[2]);
    cout << argv[1]<< " " << argv[2];
    vector<vector<pair<int,int>>> graph = loadGraph(argv[1]);
    Solver solver(a, graph);
    solver.solve();

    return 0;
}