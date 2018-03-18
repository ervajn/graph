#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <list>

int main(int argc, const char * argv[])
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <graph file>" << std::endl;
    return -1;
  }
  
  std::cout << "Reading file " << argv[1] << std::endl;

  std::map<int, int> index;
  std::map<int, std::list<std::pair<int, int> > > graph;

  std::ifstream input(argv[1]);
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
      index.insert(std::make_pair(from, index.size()));
      index.insert(std::make_pair(to, index.size()));      
      graph[from].push_back(std::make_pair(to, n));
    }
    else
    {
      std::cout << lineno << ": Failed to parse \"" << line << "\"" << std::endl;
    }
  }

  std::cout << "Number of nodes: " << index.size() << std::endl;
  std::cout << "Number of graph entries: " << graph.size() << std::endl;

  return 0;
}
