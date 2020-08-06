#include "pdrev.h"
#include "aux.h"

namespace PD{

void PdRev::setAlphaPDII(float alpha){
        alpha2 = alpha;
}

void PdRev::addNet(int numPins, std::vector<unsigned> x, std::vector<unsigned> y){
        my_graphs.push_back(new Graph(numPins, verbose, alpha1, alpha2,
                                                alpha3, alpha4, root_idx,
                                                beta, margin, seed,
                                                dist, x, y));
}

void PdRev::config(){
        num_nets = my_graphs.size();
   //     measure.start_clock();
//        cout << "\nGenerating nearest neighbor graph..." << endl;
        for (unsigned i = 0; i < num_nets; ++i) {
                // Guibas-Stolfi algorithm for computing nearest NE (north-east) neighbors
                if (i == net_num || !runOneNet) {
                        my_graphs[i]-> buildNearestNeighborsForSPT(my_graphs[i]->num_terminals);
                }
        }

  //      cout << "\nFinished generating nearest neighbor graph..." << endl;
  //      measure.stop_clock("Graph generation"); 
}

void PdRev::runPDII(){
        config();
    //    measure.start_clock();
    //    cout << "\nRunning PD-II... alpha = "
    //            << alpha2 << endl;
        for (unsigned i = 0; i < num_nets; ++i) {
                if (i == net_num || !runOneNet) {
                        my_graphs[i]-> PDBU_new_NN();
                }
        }
      /*  for (unsigned i = 0; i < num_nets; ++i) {
                cout << "  Net " << i
                        << " WL = " << my_graphs[i]->pdbu_wl
                        << " PL = " << my_graphs[i]->pdbu_pl
                        << " DC = " << my_graphs[i]->pdbu_dc
                        << endl;
        }
        cout << "Finished running PD-II..." << endl;
        measure.stop_clock("PD-II"); */
        runDAS();
}

void PdRev::runDAS(){
        //measure.start_clock();
        //cout << "\nRunning Steiner algorithm..." << endl;
        for (unsigned i = 0; i < num_nets; ++i) {
                if (i == net_num || !runOneNet) {
                        my_graphs[i]-> doSteiner_HoVW();
                }
        }
       /* for (unsigned i = 0; i < num_nets; ++i) {
                cout << "  Net " << i
                        << " WL = " << my_graphs[i]->st_wl
                        << " PL = " << my_graphs[i]->st_pl
                        << " DC = " << my_graphs[i]->st_dc
                        << endl;
        }
        cout << "Finished Steiner algorithm..." << endl;
        measure.stop_clock("HVW Steinerization");

        cout << "\nRunning DAS algorithm..." << endl; */
        for (unsigned i = 0; i < num_nets; ++i) {
                if (i == net_num || !runOneNet) {
                        my_graphs[i]-> fix_max_dc();
                }
        }
        /*for (unsigned i = 0; i < num_nets; ++i) {
                cout << "  Net " << i
                        << " WL = " << my_graphs[i]->daf_wl
                        << " PL = " << my_graphs[i]->daf_pl
                        << " DC = " << my_graphs[i]->daf_dc
                        << endl;
        }
        cout << "Finished DAS algorithm..." << endl;
        measure.stop_clock("DAS"); */
}

void PdRev::replaceNode(int graph, int originalNode){
        Graph * tree = my_graphs[graph];
        std::vector<Node> & nodes = tree->nodes;
        Node & node = nodes[originalNode];
        int nodeParent = node.parent;
        std::vector<int> & nodeChildren = node.children;

        int newNode = tree->nodes.size();
        Node newSP(newNode, node.x, node.y);

        //Replace parent in old node children
        //Add children to new node
        for (int child : nodeChildren){
                tree->replaceParent(tree->nodes[child], originalNode, newNode);
                tree->addChild(newSP, child);
        }
        //Delete children from old node
        nodeChildren.clear();
        //Set new node as old node's parent
        node.parent = newNode;
        //Set new node parent
        if (nodeParent != originalNode){
                newSP.parent = nodeParent;
                //Replace child in parent
                tree->replaceChild(tree->nodes[nodeParent], originalNode, newNode);
        } else
                newSP.parent = newNode;
        //Add old node as new node's child
        tree->addChild(newSP, originalNode);
        nodes.push_back(newSP);
}

void PdRev::transferChildren(int graph, int originalNode){
        Graph * tree = my_graphs[graph];
        std::vector<Node> & nodes = tree->nodes;
        Node & node = nodes[originalNode];
        std::vector<int> nodeChildren = node.children;

        int newNode = tree->nodes.size();
        Node newSP(newNode, node.x, node.y);

        //Replace parent in old node children
        //Add children to new node
        int count = 0;
        node.children.clear();
        for (int child : nodeChildren){
                if (count < 2){
                        tree->replaceParent(tree->nodes[child], originalNode, newNode);
                        tree->addChild(newSP, child);
                } else {
                        tree->addChild(node, child);
                }
                count++;
        }
        newSP.parent = originalNode;

        tree->addChild(node, newNode);
        nodes.push_back(newSP);
}

Tree PdRev::translateTree(int nTree){
        Graph* pdTree = my_graphs[nTree];
        Tree fluteTree;
        fluteTree.deg = pdTree->orig_num_terminals;
        fluteTree.branch = (Branch *)malloc((2* fluteTree.deg -2 )* sizeof(Branch));
        fluteTree.length = pdTree->daf_wl;
        if (pdTree->orig_num_terminals > 2){
                for (int i = 0; i < pdTree->orig_num_terminals; ++i){
                        Node & child = pdTree->nodes[i];
                        if (child.children.size() == 0 || (child.parent == i && child.children.size() == 1 && child.children[0] >= pdTree->orig_num_terminals))
                                continue;
                        replaceNode(nTree, i);
                }
                int nNodes = pdTree->nodes.size();
                for (int i = pdTree->orig_num_terminals; i < nNodes; ++i){
                        Node & child = pdTree->nodes[i];
                        while (pdTree->nodes[i].children.size() > 3 || (pdTree->nodes[i].parent != i && pdTree->nodes[i].children.size() == 3)){
                                transferChildren(nTree,i);
                        }
                }
                pdTree->RemoveSTNodes();
        }
        for (int i = 0; i < pdTree->nodes.size(); ++i){
                Node & child = pdTree->nodes[i];
                int parent = child.parent;
                Branch & newBranch = fluteTree.branch[i];
                newBranch.x = (DTYPE) child.x;
                newBranch.y = (DTYPE) child.y;
                newBranch.n = parent;
        }
        my_graphs.clear();
        return fluteTree;
}

void PdRev::printTree(Tree fluteTree){
        int i;
        for (i = 0; i < 2* fluteTree.deg-2; i++) {
                printf("%d \n", i);
                printf("%d %d\n", fluteTree.branch[i].x, fluteTree.branch[i].y);
                printf("%d %d\n\n", fluteTree.branch[fluteTree.branch[i].n].x,
                       fluteTree.branch[fluteTree.branch[i].n].y);
        }
}

}
