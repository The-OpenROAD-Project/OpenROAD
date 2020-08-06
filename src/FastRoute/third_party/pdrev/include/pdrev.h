#include <vector>
namespace PD{

typedef int DTYPE;

typedef struct {
        DTYPE x,y;
        int n;
} Branch;

typedef struct {
        int deg;
        DTYPE length;
        Branch *branch;
} Tree;

class PdRev{
public:
        PdRev() = default;
        void setAlphaPDII(float alpha);
        void addNet(int numPins, std::vector<unsigned> x, std::vector<unsigned> y);
        void runPDII();
        Tree translateTree(int nTree);
private:
        void runDAS();
        void config();
        void replaceNode(int graph, int originalNode);
        void transferChildren(int graph, int originalNode);
        void printTree(Tree fluteTree);
};

}

