
#include "FlexLayout.h"

int main(int argc, char const *argv[])
{
    FlexNodeRef root = Flex_newNode();

    FlexNodeRef child1 = Flex_newNode();
    Flex_setWidth(child1, 50);
    Flex_setHeight(child1, 50);
    Flex_addChild(root, child1);

    FlexNodeRef child2 = Flex_newNode();
    Flex_setWidth(child2, 50);
    Flex_setHeight(child2, 50);
    Flex_addChild(root, child2);

    Flex_layout(root, FlexUndefined, FlexUndefined, 1);
    Flex_print(root, FlexPrintDefault);

    Flex_freeNodeRecursive(root);

    return 0;
}
