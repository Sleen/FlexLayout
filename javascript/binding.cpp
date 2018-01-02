extern "C" {
    #include "../src/FlexLayout.c"
}

#include <vector>
#include <memory>
#include <functional>

template<typename T, bool isEnum = false> struct _WrapType { using type = T; };
template<typename T> struct _WrapType<T, true> { using type = typename std::underlying_type<T>::type; };
template<typename T> struct WrapType {
    using type = typename _WrapType<T, std::is_enum<T>::value>::type;
};

typedef struct Size {
    float width;
    float height;

    Size(float width, float height) {
        this->width = width;
        this->height = height;
    }

    Size(): Size(0, 0) {}

    float getWidth() { return width; }
    void setWidth(float width) { this->width = width; }

    float getHeight() { return height; }
    void setHeight(float height) { this->height = height; }

} Size;

struct Length {
    using LengthType = typename WrapType<FlexLengthType>::type;

    float value;
    LengthType type;

    Length(float value, LengthType type) {
        this->value = value;
        this->type = type;
    }

    Length(float value): Length(value, (LengthType)FlexLengthTypePoint) {}

    Length(): Length(0, (LengthType)FlexLengthTypePoint) {}

    float getValue() { return value; }
    void setValue(float value) { this->value = value; }

    LengthType getType() { return type; }
    void setType(LengthType type) { this->type = type; }
};

class Node {
private:
    FlexNodeRef _node;

    std::vector<std::shared_ptr<Node>> children;

    typedef std::function<Size (Size)> MeasureCallback;
    typedef std::function<float (Size)> BaselineCallback;

    MeasureCallback measure;
    BaselineCallback baseline;

    static FlexSize static_measure(void* context, FlexSize constrainedSize) {
        Node* node = (Node*)context;
        Size size = node->measure(Size{constrainedSize.width, constrainedSize.height});
        return FlexSize{size.width, size.height};
    }

    static float static_baseline(void* context, FlexSize constrainedSize) {
        Node* node = (Node*)context;
        return node->baseline(Size{constrainedSize.width, constrainedSize.height});
    }

public:
    Node() {
        // printf("%x\n", this);
        _node = Flex_newNode();
        Flex_setContext(_node, this);
    }

    Node(const Node& node) {
        // printf("%x -> %x\n", &node, this);
        _node = node._node;
        Flex_setContext(_node, this);
        children = node.children;
        measure = node.measure;
        baseline = node.baseline;
    }

    ~Node() {
        // printf("%x free\n", this);
        Flex_freeNode(_node);
    }

    void setMeasure(MeasureCallback measure) {
        this->measure = measure;
        Flex_setMeasureFunc(_node, measure ? Node::static_measure : NULL);
    }

    void setBaseline(BaselineCallback baseline) {
        this->baseline = baseline;
        Flex_setBaselineFunc(_node, baseline ? Node::static_baseline : NULL);
    }

#define FLEX_GETTER(TYPE, Name, field)                  WrapType<TYPE>::type get##Name() { return (WrapType<TYPE>::type)Flex_get##Name(_node); }
#define FLEX_SETTER(TYPE, Name, field)                  void set##Name(WrapType<TYPE>::type Name) { Flex_set##Name(_node, (TYPE)Name); }
#define FLEX_LENGTH_PROPERTY(Name, field) \
    Length get##Name() { FlexLength ret = Flex_get##Name(_node); return *(Length*)&ret; } \
    void set##Name(Length Name) { Flex_set##Name##_Length(_node, *(FlexLength*)&Name); }
#define FLEX_SETTER_LENGTH_VALUE(Name, field, Type)     
#define FLEX_SETTER_LENGTH_TYPE(Name, field, Type)      

FLEX_PROPERTYES()
FLEX_EXT_PROPERTYES()
FLEX_RESULT_PROPERTYES()

#undef FLEX_GETTER
#undef FLEX_SETTER
#undef FLEX_LENGTH_PROPERTY
#undef FLEX_SETTER_LENGTH_VALUE
#undef FLEX_SETTER_LENGTH_TYPE

    void layout(float constrainedWidth, float constrainedHeight, float scale) {
        Flex_layout(_node, constrainedWidth, constrainedHeight, scale);
    }

    void layout(float constrainedWidth, float constrainedHeight) {
        layout(constrainedWidth, constrainedHeight, 1);
    }

    void print() {
        Flex_print(_node, (FlexPrintOptions)0);
    }

    void add(std::shared_ptr<Node> node) {
        children.push_back(node);
        Flex_addChild(_node, node->_node);
    }

    void insert(std::shared_ptr<Node> node, size_t index) {
        children.insert(children.begin() + index, node);
        Flex_insertChild(_node, node->_node, index);
    }

    void remove(std::shared_ptr<Node> node) {
        for (auto it = children.begin(); it != children.end(); it++) {
            if ((*it)->_node == node->_node) {
                children.erase(it);
                Flex_removeChild(_node, node->_node);
                return;
            }
        }
    }

    std::shared_ptr<Node> childAt(size_t index) {
        return children[index];
    }

    size_t getChildrenCount() {
        return children.size();
    }
};

#include "nbind/nbind.h"

NBIND_CLASS(Size) {
    construct<>();
    construct<float, float>();
    getset(getWidth, setWidth);
    getset(getHeight, setHeight);
}

NBIND_CLASS(Length) {
    construct<>();
    construct<float>();
    construct<float, Length::LengthType>();
    getset(getValue, setValue);
    getset(getType, setType);
}

NBIND_CLASS(Node) {

#define FLEX_PROPERTY(type, Name, field)                getset(get##Name, set##Name)
#define FLEX_LENGTH_PROPERTY(Name, field)               getset(get##Name, set##Name)
#define FLEX_RESULT_PROPERTY(Name, field)               getter(getResult##Name)
#define FLEX_SETTER_LENGTH_VALUE(Name, field, Type)     
#define FLEX_SETTER_LENGTH_TYPE(Name, field, Type)      

FLEX_PROPERTYES()
FLEX_EXT_PROPERTYES()
FLEX_RESULT_PROPERTYES()

#undef FLEX_PROPERTY
#undef FLEX_LENGTH_PROPERTY
#undef FLEX_RESULT_PROPERTY
#undef FLEX_SETTER_LENGTH_VALUE
#undef FLEX_SETTER_LENGTH_TYPE

    construct<>();
    method(setMeasure);
    method(setBaseline);
    multimethod(layout, args(float, float));
    multimethod(layout, args(float, float, float), "layoutWithScale");
    method(add);
    method(insert);
    method(remove);
    method(childAt);
    getter(getChildrenCount);
    method(print);
}
