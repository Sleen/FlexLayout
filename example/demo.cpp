
#include "FlexLayout.h"

#include <vector>
#include <stdio.h>

struct Node {
    FlexNode* flexNode;
    std::vector<Node*> children;

    Node() {
        flexNode = newFlexNode();
        initFlexNode(flexNode);
    }

    ~Node() {
        freeFlexNode(flexNode);
        for (Node* child : children) {
            delete child;
        }
    }

    static FlexNode* childAt(void* context, size_t index) {
        Node *node = (Node *)context;
        return node->children[index]->flexNode;
    }

    void createTree() {
        flexNode->context = this;
        flexNode->childAt = Node::childAt;
        flexNode->childrenCount = children.size();
        for (Node* child : children) {
            child->createTree();
        }
    }
    
    void print(int indent = 0) {
        for (int i=0;i<indent;i++) {
            printf("  ");
        }
        printf("%.1f, %.1f - %.1f x %.1f\n",
               flexNode->result.position[FLEX_LEFT],
               flexNode->result.position[FLEX_TOP],
               flexNode->result.size[FLEX_WIDTH],
               flexNode->result.size[FLEX_HEIGHT]);
        for (Node* child : children) {
            child->print(indent + 1);
        }
    }
};

int main(int argc, char const *argv[])
{
    Node root;
    root.flexNode->size[FLEX_WIDTH] = flexLength(100, FlexLengthTypeDefault);
    root.flexNode->size[FLEX_HEIGHT] = flexLength(100, FlexLengthTypeDefault);
    root.flexNode->direction = FlexVertical;
    root.flexNode->justifyContent = FlexCenter;

    Node *node1 = new Node();
    node1->flexNode->size[FLEX_WIDTH] = flexLength(50, FlexLengthTypeDefault);
    node1->flexNode->size[FLEX_HEIGHT] = flexLength(50, FlexLengthTypeDefault);
    node1->flexNode->alignSelf = FlexCenter;
    root.children.push_back(node1);

    Node *node2 = new Node();
    node2->flexNode->size[FLEX_HEIGHT] = flexLength(20, FlexLengthTypeDefault);
    root.children.push_back(node2);

    root.createTree();
    layoutFlexNode(root.flexNode, FlexAuto, FlexAuto, 1);
    
    root.print();

    return 0;
}
