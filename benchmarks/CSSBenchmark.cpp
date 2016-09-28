/**
 * Copyright (c) 2014-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "CSSBenchmark.h"

#include "FlexLayout.h"
#include <time.h>

#include <vector>

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
    
    void layout() {
        createTree();
        layoutFlexNode(flexNode, FlexAuto, FlexAuto, 1);
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

static int count = 0;

// Measure functions can be quite slow, for example when measuring text.
// Simulate this by sleeping for 1 millisecond.
static FlexSize _measure(void *context, FlexSize constraintedSize) {
  const struct timespec sleeptime = {0, 1000000};
  nanosleep(&sleeptime, NULL);
  count++;
  return (FlexSize){
      .size = {
          10,10//constraintedSize.size[FLEX_WIDTH] == FlexAuto ? 10 : constraintedSize.size[FLEX_WIDTH],
          //constraintedSize.size[FLEX_HEIGHT] == FlexAuto ? 10 : constraintedSize.size[FLEX_HEIGHT]
      }
  };
}

CSS_BENCHMARKS({

  CSS_BENCHMARK("Stack with flex", {
    Node root;
    root.flexNode->size[FLEX_WIDTH] = flexLength(100, FlexLengthTypeDefault);
    root.flexNode->size[FLEX_HEIGHT] = flexLength(100, FlexLengthTypeDefault);
    
    for (uint32_t i = 0; i < 10; i++) {
      Node *child = new Node();
      child->flexNode->measure = _measure;
      child->flexNode->flexGrow = 1;
//      child->flexNode->flexBasis = FlexLengthZero;
      root.children.push_back(child);
    }

//    count = 0;
    root.layout();
//    printf("count: %d\n", count);
//    root.print();
  });

  CSS_BENCHMARK("Align stretch in undefined axis", {
    Node root;
      
    for (uint32_t i = 0; i < 10; i++) {
      Node *child = new Node();
      child->flexNode->measure = _measure;
      child->flexNode->size[FLEX_HEIGHT] = flexLength(20, FlexLengthTypeDefault);
      root.children.push_back(child);
    }

//    count = 0;
    root.layout();
//    printf("count: %d\n", count);
//    root.print();
  });

  CSS_BENCHMARK("Nested flex", {
    Node root;
      
    for (uint32_t i = 0; i < 10; i++) {
      Node *child = new Node();
      child->flexNode->measure = _measure;
      child->flexNode->flexGrow = 1;
      root.children.push_back(child);
      
      for (uint32_t ii = 0; ii < 10; ii++) {
        Node *grandChild = new Node();
        grandChild->flexNode->measure = _measure;
        grandChild->flexNode->flexGrow = 1;
        child->children.push_back(grandChild);
      }
    }

//    count = 0;
    root.layout();
//    printf("count: %d\n", count);
//    root.print();
  });

});
