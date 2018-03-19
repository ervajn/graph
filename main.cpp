#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <algorithm>


static void read_csv(const std::string& filename,
                     std::map<int, int>& id2index,
                     std::map<int, std::list<std::pair<int, int> > >& graph)
{
    std::cout << "Reading file " << filename << std::endl;
    std::ifstream input(filename);
    int lineno = 0;
    std::string line;  
    while (std::getline(input, line))
    {
        std::stringstream ss(line);
        lineno++;
        char c1, c2;
        int from, to, n;
        if (ss >> from >> c1 >> to >> c2 >> n)
        {
            id2index.insert(std::make_pair(from, id2index.size()));
            id2index.insert(std::make_pair(to, id2index.size()));      
            graph[from].push_back(std::make_pair(to, n));
        }
        else
        {
            std::cout << "Skipping line " << lineno << ": \"" << line << "\"" << std::endl;
        }
    }

    std::cout << "Number of nodes: " << id2index.size() << std::endl;
    std::cout << "Number of graph entries: " << graph.size() << std::endl;
}

std::vector<std::vector<std::pair<int, int> > > createIndexGraph(
    const std::map<int, std::list<std::pair<int, int> > >& idGraph,
    const std::map<int, int>& id2index)
{
    std::vector<std::vector<std::pair<int, int> > > indexGraph(id2index.size());    
    for (const auto& node : idGraph)
    {
        for (const auto& link : node.second)
        {
            indexGraph[id2index.at(node.first)].push_back(std::make_pair(id2index.at(link.first), link.second));
        }
    }
    return indexGraph;
}

std::string getGridAsString(const std::vector<int>& position, const int width, std::map<int, int> id2index)
{
    const int height = position.size() / width;
    std::vector<std::vector<int> > grid(width, std::vector<int>(height));
    for (int i=0; i<position.size(); ++i)
    {
        const int p = position[i];
        grid[p % width][p / width] = i < id2index.size() ? i : -1;
    }

    std::stringstream ss;    
    for (int y=0; y<height; ++y)
    {        
        if (y > 0)
            ss << std::endl;        
        for (int x=0; x<width; ++x)
        {
            if (grid[x][y] < 0)
            {                
                ss << " " << std::setw(3) << ".";
            }
            else
            {
                const int id = std::find_if(id2index.begin(), id2index.end(), [grid, x, y](const std::pair<int, int>& p){return p.second == grid[x][y];})->first;
                ss << " " << std::setw(3) << id;
            }            
        }
    }

    return ss.str();
}

static double dist(const int width, const int p1, const int p2)
{
    const int dx = p2 % width - p1 % width;
    const int dy = p2 / width - p1 / width;
    return sqrt(dx*dx + dy*dy);    
}

static double nodeCost(const std::vector<std::vector<std::pair<int, int> > >& indexGraph, 
                    const std::vector<int>& index2position, 
                    const int width,
                    const int nodeIndex)
{
    double cost = 0;
    if (nodeIndex < indexGraph.size())
    {
        const int p0 = index2position[nodeIndex];
        for (const auto& link : indexGraph[nodeIndex])
        {
            cost += dist(width, p0, index2position[link.first]) * link.second;
        }
    }    
    return cost;
}

static double gridCost(const std::vector<std::vector<std::pair<int, int> > >& indexGraph, 
                    const std::vector<int>& index2position, 
                    const int width)

{    
    double cost = 0;
    for (int i=0; i<indexGraph.size(); ++i)
    {
        cost += nodeCost(indexGraph, index2position, width, i);
    }
    return cost;
}

int index2id(const std::map<int, int> id2index, int index)
{
    std::map<int, int>::const_iterator it = std::find_if(id2index.begin(), id2index.end(), [index](std::pair<int, int> x){ return x.second == index;});
    
    return it->first;
}

int main(int argc, const char * argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <graph file>" << std::endl;
        return -1;
    }
  
    std::map<int, int> id2index;
    std::map<int, std::list<std::pair<int, int> > > idGraph;
    read_csv(argv[1], id2index, idGraph);
    assert(id2index.size() >= idGraph.size());

    const int width = ceil(sqrt(id2index.size()));
    const int height = width;
    assert(width*height >= id2index.size());
    std::cout << "height=" << height << " width=" << width << std::endl;
    
    const auto indexGraph = createIndexGraph(idGraph, id2index);
    assert(indexGraph.size() == id2index.size());

    std::vector<int> index2position(height * width);
    for (auto& p : index2position)
    {
        p = &p - &index2position[0];
    }
    
    std::cout << getGridAsString(index2position, width, id2index) << std::endl;

    for (int i=0; i<1000000; i++)
    {
        const int i1 = rand() % index2position.size();
        const int i2 = rand() % index2position.size();
        if (i1 == i2)
            continue;
        
        const double pre = gridCost(indexGraph, index2position, width);
        std::swap(index2position[i1], index2position[i2]);
        const double post = gridCost(indexGraph, index2position, width);

        if (post >= pre)
        {
            std::swap(index2position[i1], index2position[i2]);
        }
        else
        {
            std::cout << "pre=" << pre << " post=" << post << std::endl;
            std::cout << getGridAsString(index2position, width, id2index) << std::endl;
        }        
    }

    return 0;
}
