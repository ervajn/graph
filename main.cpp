#include <unistd.h>
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

class Config
{
  const std::string helpString_;
public:
  explicit Config()
    :helpString_("Usage: grid [option(s)]\n"
                 "options:\n"
                 " -h          This text\n"
                 " -v          Verbosity\n"
                 " -x <width>  Set width\n"
                 " -y <height> Set height\n"
                 " -g          Use global cost\n"
                 " -d          Use directed graph\n"
                 " -o <file>   Output coordinate csv file\n"
                 " -i <file>   Input graph csv file\n"
                 " -e <n>      Number loops without change for termination\n"
                 " -s <n>      Max number of candidates involved in swapping per turn\n")
    ,verbosity_(0)
    ,width_(0)
    ,height_(0)
    ,input_("graph.csv")
    ,output_("xy_out.csv")
    ,useGlobalCost_(false)
    ,directed_(false)
    ,endCondition_(1000000)
    ,maxInSwap_(2)
  {}

  bool parseOptions(int argc, char *argv[])
  {
    bool showHelp = false;
    int opt;
    while ((opt = getopt(argc, argv, "hvx:y:i:gde:s:")) != -1)
    {
      switch (opt)
      {
      case 'h':
        showHelp = true;
        break;
      case 'v':
        ++verbosity_;
        break;
      case 'x':
        width_ = atoi(optarg);
        break;
      case 'y':
        height_ = atoi(optarg);
        break;
      case 'i':
        input_ = optarg;
        break;
      case 'o':
        output_ = optarg;
        break;
      case 'g':
        useGlobalCost_ = true;
        break;
      case 'd':
        directed_ = true;
        break;
      case 'e':
        endCondition_ = atoi(optarg);
        break;
      case 's':
        maxInSwap_ = atoi(optarg);
        break;
      default:
        std::cout << ">>> Illegal option" << "\n" << helpString_ << std::endl;
        return false;
      }
    }

    if (showHelp)
    {
      std::cout << helpString_ << toString() << std::endl;
      return false;
    }

    return true;
  }

#define OS(x) "\n  " << #x << "=" << x##_
  std::string toString() const
  {
    std::stringstream ss;
    ss << "Config: " <<
      OS(verbosity) <<
      OS(width) <<
      OS(height) <<
      OS(input) <<
      OS(useGlobalCost) <<
      OS(directed) <<
      OS(output) <<
      OS(endCondition) <<
      OS(maxInSwap);
    return ss.str();
  }

public:
  int verbosity_;
  int width_;
  int height_;
  std::string input_;
  std::string output_;
  bool useGlobalCost_;
  bool directed_;
  int endCondition_;
  int maxInSwap_;
} config;

#define DBG(msg) if (config.verbosity_) std::cout << msg << std::endl
#define DBG2(msg) if (config.verbosity_ > 1) std::cout << msg << std::endl

using IdGraph = std::map<int, std::list<std::pair<int, int> > >;
using Id2index = std::map<int, int>;
using IndexGraph = std::vector<std::vector<std::pair<int, int> > >;
using Index2position = std::vector<int>;

static void read_csv(const std::string& filename,
                     Id2index& id2index,
                     IdGraph& graph)
{
  DBG("Reading file " << filename);
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
      if (!config.directed_)
      {
        graph[to].push_back(std::make_pair(from, n));
      }
    }
    else
    {
      DBG("Skipping line " << lineno << ": \"" << line << "\"");
    }
  }

  DBG("Number of nodes: " << id2index.size());
  DBG("Number of graph entries: " << graph.size());
}

static void write_csv(const std::string& filename,
                      const Id2index& id2index,
                      const Index2position& index2position,
                      const int width)
{
  DBG("Writing file " << filename);
  std::ofstream output(filename);
  output << "id,x,y" << std::endl;
  for (const auto& entry : id2index)
  {
    const int p = index2position[entry.second];
    output << entry.first << "," << (p % width) << ',' << (p / width) << std::endl;
  }
}

IndexGraph createIndexGraph(const IdGraph& idGraph, const Id2index& id2index)
{
  IndexGraph indexGraph(id2index.size());
  for (const auto& node : idGraph)
  {
    for (const auto& link : node.second)
    {
      indexGraph[id2index.at(node.first)].push_back(std::make_pair(id2index.at(link.first), link.second));
    }
  }
  return indexGraph;
}

std::string getGridAsString(const std::vector<int>& position, const int width, Id2index id2index)
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
    if (y > 0) ss << std::endl;
    for (int x=0; x<width; ++x)
    {
      if (grid[x][y] < 0)
      {
        ss << " " << std::setw(6) << "......";
      }
      else
      {
        const int id = std::find_if(id2index.begin(), id2index.end(), [grid, x, y](const std::pair<int, int>& p){return p.second == grid[x][y];})->first;
        ss << " " << std::setw(6) << id;
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
                       const Index2position& index2position,
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

static double nodesCost(const std::vector<std::vector<std::pair<int, int> > >& indexGraph, 
                        const Index2position& index2position,
                        const int width,
                        const int* beginIndex,
                        const int* endIndex)
{
  return std::accumulate(beginIndex, endIndex, 0,
                         [indexGraph, index2position, width](int acc, int index){
                           return acc + nodeCost(indexGraph, index2position, width, index);});
}

static int index2id(const Id2index& id2index, int index)
{
  std::map<int, int>::const_iterator it = std::find_if(id2index.begin(), id2index.end(), [index](std::pair<int, int> x){return x.second == index;});
  return it->first;
}

static void rearrange(const IndexGraph& indexGraph,
                      const Id2index& id2index,
                      Index2position& index2position,
                      const int width,
                      const int endCondition,
                      const std::size_t maxSwapPerTurn)
{
  const int maxSwap2 = std::min(maxSwapPerTurn, index2position.size()) - 2;
  std::vector<int> candidates(index2position.size());
  std::iota(candidates.begin(), candidates.end(), 0);
  int succ = 0;
  int fail = 0;
  for (int count=1;; count++)
  {
    const int numCandidates = 2 + (maxSwap2 ? (rand() % maxSwap2) : 0);
    for (int i=0; i<numCandidates; ++i)
    {
      std::swap(candidates[i], candidates[i + rand() % (candidates.size() - i)]);
    }

    const double pre = config.useGlobalCost_ ? gridCost(indexGraph, index2position, width) :
      nodesCost(indexGraph, index2position, width, &candidates[0], &candidates[numCandidates]);

    // Rotate the candidates left
    const int tmp = index2position[candidates[0]];
    for (int i=1; i<numCandidates; ++i)
      index2position[candidates[i-1]] = index2position[candidates[i]];
    index2position[candidates[numCandidates-1]] = tmp;

    const double post = config.useGlobalCost_ ? gridCost(indexGraph, index2position, width):
      nodesCost(indexGraph, index2position, width, &candidates[0], &candidates[numCandidates]);

    if (post >= pre)
    {
      // Rotate the candidates right to restore
      const int tmp = index2position[candidates[numCandidates-1]];
      for (int i=numCandidates-1; i>0; --i)
        index2position[candidates[i]] = index2position[candidates[i-1]];
      index2position[candidates[0]] = tmp;
      if (++fail > endCondition)
      {
        break;
      }
    }
    else
    {
      fail = 0;
      succ++;
      DBG("pre=" << pre << " post=" << post);
      DBG2(getGridAsString(index2position, width, id2index));
    }

    if (config.verbosity_ && (count % 10000000)==0)
    {
      DBG(succ << "/" << count << " " << double(succ)/count << ": Global cost=" << gridCost(indexGraph, index2position, width));
      DBG2(getGridAsString(index2position, width, id2index));
    }
  }
}

int main(int argc, char * argv[])
{
  if (!config.parseOptions(argc, argv))
  {
    return -1;
  }
  DBG(config.toString());

  Id2index id2index;
  IdGraph idGraph;
  read_csv(config.input_, id2index, idGraph);
  assert(id2index.size() >= idGraph.size());

  const int width = config.width_ ? config.width_ : ceil(sqrt(id2index.size()));
  const int height = config.height_ ? config.height_ : ceil(double(id2index.size()) / width);
  DBG("width=" << width << " height=" << height);

  assert(width*height >= id2index.size());

  const auto indexGraph = createIndexGraph(idGraph, id2index);
  assert(indexGraph.size() == id2index.size());

  std::vector<int> index2position(height * width);
  std::iota(index2position.begin(), index2position.end(), 0);

  DBG2("Initial grid\n" << getGridAsString(index2position, width, id2index));

  rearrange(indexGraph, id2index, index2position, width, config.endCondition_, config.maxInSwap_);

  if (config.output_ != "")
  {
    write_csv(config.output_, id2index, index2position, width);
  }

  std::cout << getGridAsString(index2position, width, id2index) << std::endl;

  return 0;
}
