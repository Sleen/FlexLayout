//
//  FlexLayout.m
//  flex_layout
//
//  Created by Sleen on 16/1/25.
//  Copyright © 2016年 Sleen. All rights reserved.
//

#include "FlexLayout.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <memory.h>


#if DEBUG
#   define flex_assert(e) assert(e)
#else
#   define flex_assert(e) ((void)0)
#endif


#define FlexTreatNanAsInf(n) (isnan(n) ? INFINITY : n)
#define FlexFloatEquals(a, b) ((isnan(a) && isnan(b)) || a == b)
#define FlexLengthEquals(a, b) (FlexFloatEquals(a.value, b.value) && a.type == b.type)
#define FlexPixelRound(value, scale) (roundf((value) * (scale)) / (scale))
#define FlexIsResolved(n) !FlexIsUndefined(n)
#define FlexCacheSizeUndefined (FlexSize){ -1000, -1000 }

#define FlexVector(type) FlexVector_##type
#define FlexVectorRef(type) FlexVector(type)*
#define FlexVector_new(type, initialCapacity) _FlexVector_new_##type(initialCapacity)
#define FlexVector_free(type, vector) _FlexVector_free_##type(vector)
#define FlexVector_insert(type, vector, value, index) _FlexVector_insert_##type(vector, value, index)
#define FlexVector_add(type, vector, value) _FlexVector_add_##type(vector, value)
#define FlexVector_removeAt(type, vector, index) _FlexVector_removeAt_##type(vector, index)
#define FlexVector_remove(type, vector, value) _FlexVector_remove_##type(vector, value)
#define FlexVector_size(type, vector) _FlexVector_size_##type(vector)

#define FLEX_VECTOR_INIT_WITH_EQUALS(type, equals)                                      \
typedef struct {                                                                        \
    size_t size;                                                                        \
    size_t capacity;                                                                    \
    type *data;                                                                         \
} FlexVector(type);                                                                     \
                                                                                        \
 FlexVectorRef(type) _FlexVector_new_##type(size_t initialCapacity) {                   \
    flex_assert(initialCapacity > 0);                                                   \
    FlexVectorRef(type) vector = (FlexVectorRef(type))malloc(sizeof(FlexVector(type))); \
    vector->size = 0;                                                                   \
    vector->capacity = initialCapacity;                                                 \
    vector->data = (type *)malloc(sizeof(type) * vector->capacity);                     \
    return vector;                                                                      \
}                                                                                       \
                                                                                        \
void _FlexVector_free_##type(FlexVectorRef(type) vector) {                              \
    free(vector->data);                                                                 \
    free(vector);                                                                       \
}                                                                                       \
                                                                                        \
void _FlexVector_insert_##type(FlexVectorRef(type) vector, type value, size_t index) {  \
   if (vector->size == vector->capacity) {                                              \
       vector->capacity *= 2;                                                           \
       vector->data = (type *)realloc(vector->data, sizeof(type) * vector->capacity);   \
   }                                                                                    \
   for (size_t i = index; i < vector->size; i++) {                                      \
       vector->data[i + 1] = vector->data[i];                                           \
   }                                                                                    \
   vector->data[index] = value;                                                         \
   vector->size++;                                                                      \
}                                                                                       \
                                                                                        \
void _FlexVector_add_##type(FlexVectorRef(type) vector, type value) {                   \
    _FlexVector_insert_##type(vector, value, vector->size);                             \
}                                                                                       \
                                                                                        \
void _FlexVector_removeAt_##type(FlexVectorRef(type) vector, size_t index) {            \
    for (size_t i = index + 1; i < vector->size; i++) {                                 \
        vector->data[i - 1] = vector->data[i];                                          \
    }                                                                                   \
    vector->size--;                                                                     \
}                                                                                       \
                                                                                        \
void _FlexVector_remove_##type(FlexVectorRef(type) vector, type value) {                \
    for (size_t i = 0; i < vector->size; i++) {                                         \
        if (equals(vector->data[i], value)) {                                           \
            _FlexVector_removeAt_##type(vector, i);                                     \
            return;                                                                     \
        }                                                                               \
    }                                                                                   \
}                                                                                       \
                                                                                        \
size_t _FlexVector_size_##type(FlexVectorRef(type) vector) {                            \
    return vector ? vector->size : 0;                                                   \
}                                                                                       \

#define FLEX_VECTOR_EQUALS_FUNC(a, b) (a == b)
#define FLEX_VECTOR_INIT(type) FLEX_VECTOR_INIT_WITH_EQUALS(type, FLEX_VECTOR_EQUALS_FUNC)


static const FlexDirection FLEX_WIDTH = FlexHorizontal;
static const FlexDirection FLEX_HEIGHT = FlexVertical;

typedef enum {
    FLEX_LEFT = 0,
    FLEX_TOP,
    FLEX_RIGHT,
    FLEX_BOTTOM,
    FLEX_START,
    FLEX_END
} FlexPositionIndex;

static FlexPositionIndex flex_start[4] = { FLEX_LEFT, FLEX_TOP, FLEX_RIGHT, FLEX_BOTTOM };
static FlexPositionIndex flex_end[4] = { FLEX_RIGHT, FLEX_BOTTOM, FLEX_LEFT, FLEX_TOP };
static FlexDirection flex_dim[4] = { FLEX_WIDTH, FLEX_HEIGHT, FLEX_WIDTH, FLEX_HEIGHT };


typedef struct {
    float scale;
} FlexLayoutContext;

typedef enum {
    FlexLayoutFlagMeasureWidth = 1 << 0,    // only measure width
    FlexLayoutFlagMeasureHeight = 1 << 1,   // only measure height
    FlexLayoutFlagLayout = 1 << 2,          // measure and layout
} FlexLayoutFlags;

typedef struct {
    float itemsSize;
    size_t itemsCount;
    float ascender;
} FlexLine;

typedef struct {
    float position[2];
    float size[2];
    float margin[4];
    float padding[4];
} FlexResult;

typedef struct {
    FlexSize availableSize;
    FlexSize measuredSize;
} FlexMeasureCache;

FLEX_VECTOR_INIT(FlexNodeRef)
#define FlexSizeEquals(a, b) (FlexFloatEquals(a.width, b.width) && FlexFloatEquals(a.height, b.height))
#define FlexMeasureCacheEquals(a, b) (FlexSizeEquals(a.availableSize, b.availableSize) && FlexSizeEquals(a.measuredSize, b.measuredSize))
FLEX_VECTOR_INIT_WITH_EQUALS(FlexMeasureCache, FlexMeasureCacheEquals)

typedef struct FlexNode {
    FlexWrapMode wrap;
    FlexDirection direction;
    FlexAlign alignItems;
    FlexAlign alignSelf;
    FlexAlign alignContent;
    FlexAlign justifyContent;
    FlexLength flexBasis;       // length, percentage(relative to the flex container's inner main size), auto, content
    float flexGrow;
    float flexShrink;
    FlexLength size[2];         // length, percentage(relative to the flex container's inner size), auto
    FlexLength minSize[2];      // length, percentage(relative to the flex container's inner size)
    FlexLength maxSize[2];      // length, percentage(relative to the flex container's inner size), none
    FlexLength margin[6];       // length, percentage(relative to the flex container's inner width), auto
    FlexLength padding[6];      // length, percentage(relative to the flex container's inner width)
    float border[6];            // length

    // extension
    bool fixed;
    FlexLength spacing;         // the spacing between each two items. length, percentage(relative to its inner main size)
    FlexLength lineSpacing;     // the spacing between each two lines. length, percentage(relative to its inner cross size)
    unsigned int lines;         // the maximum number of lines, 0 means no limit
    unsigned int itemsPerLine;  // the maximum number of items per line, 0 means no limit

    FlexResult result;

    void* context;
    FlexMeasureFunc measure;
    FlexBaselineFunc baseline;

    FlexVectorRef(FlexNodeRef) children;
    FlexNodeRef parent;
    
    // internal fields
    float flexBaseSize;
    float resolvedMargin[4];
    float ascender;
    FlexLength calculatedFlexBasis;
    FlexAlign calculatedAlignSelf;

    // measure cache
    FlexVectorRef(FlexMeasureCache) measuredSizeCache;

    // layout cache
    FlexSize lastConstrainedSize;
    FlexLength lastSize[2];

} FlexNode;


static const FlexNode defaultFlexNode = {
    .wrap = FlexNoWrap,
    .direction = FlexHorizontal,
    .alignItems = FlexStretch,
    .alignSelf = FlexInherit,
    .alignContent = FlexStretch,
    .justifyContent = FlexStart,
    .flexBasis = FlexLengthAuto,
    .flexGrow  = 0,
    .flexShrink = 1,
    .size = { FlexLengthAuto, FlexLengthAuto },
    .minSize = { FlexLengthZero, FlexLengthZero },
    .maxSize = { FlexLengthUndefined, FlexLengthUndefined },
    .margin = { FlexLengthZero, FlexLengthZero, FlexLengthZero, FlexLengthZero, FlexLengthUndefined, FlexLengthUndefined },
    .padding = { FlexLengthZero, FlexLengthZero, FlexLengthZero, FlexLengthZero, FlexLengthUndefined, FlexLengthUndefined },
    .border = { 0, 0, 0, 0, FlexUndefined, FlexUndefined },

    .fixed = false,
    .spacing = FlexLengthZero,
    .lineSpacing = FlexLengthZero,
    .lines = 0,
    .itemsPerLine = 0,

    .result = {
        .position = { 0, 0 },
        .size = { 0, 0 },
        .margin = { 0, 0, 0, 0 },
        .padding = { 0, 0, 0, 0 },
    },

    .context = NULL,
    .measure = NULL,
    .baseline = NULL,

    .children = NULL,
    .parent = NULL,

    .measuredSizeCache = NULL,
    .lastConstrainedSize = FlexCacheSizeUndefined,
};


void flex_markDirty(FlexNodeRef node) {
    node->lastConstrainedSize = FlexCacheSizeUndefined;
    if (node->parent) {
        flex_markDirty(node->parent);
    }
}


// implementation of getters and setters
#define FLEX_GETTER(type, Name, field) \
    type Flex_get##Name(FlexNodeRef node) { \
        return node->field; \
    }
#define FLEX_SETTER(type, Name, field) \
    void Flex_set##Name(FlexNodeRef node, type Name) { \
        if (node->field != Name) { \
            node->field = Name; \
            flex_markDirty(node); \
        } \
    }
#define FLEX_SETTER_LENGTH(Name, field) \
    void Flex_set##Name(FlexNodeRef node, FlexLength Name) { \
        if (!FlexLengthEquals(node->field, Name)) { \
            node->field = Name; \
            flex_markDirty(node); \
        } \
    }
#define FLEX_SETTER_LENGTH_VALUE(Name, field, Type) \
    void Flex_set##Name(FlexNodeRef node, float Name) { \
        FlexLength value = {Name, FlexLengthType##Type}; \
        if (!FlexLengthEquals(node->field, value)) { \
            node->field = value; \
            flex_markDirty(node); \
        } \
    }
#define FLEX_SETTER_LENGTH_TYPE(Name, field, Type) \
    void Flex_set##Name##Type(FlexNodeRef node) { \
        if (node->field.type != FlexLengthType##Type) { \
            node->field.type = FlexLengthType##Type; \
            flex_markDirty(node); \
        } \
    }

FLEX_PROPERTYES()
FLEX_CALLBACK_PROPERTIES()
FLEX_EXT_PROPERTYES()
FLEX_RESULT_PROPERTYES()

#undef FLEX_GETTER
#undef FLEX_SETTER
#undef FLEX_SETTER_LENGTH
#undef FLEX_SETTER_LENGTH_VALUE
#undef FLEX_SETTER_LENGTH_TYPE


// for internal use
#define FLEX_GETTER(type, Name, field) \
    void Flex_set##Name(FlexNodeRef node, type Name) { \
        node->field = Name; \
    }
FLEX_RESULT_PROPERTYES()
#undef FLEX_GETTER


static inline bool flex_isAbsolute(FlexLength length) {
    return length.type == FlexLengthTypePoint;
}

static inline float flex_absoluteValue(FlexLength length, FlexLayoutContext *context) {
    flex_assert(flex_isAbsolute(length));    // absolute value is requested
    return length.value;
}

static inline float flex_resolve(FlexLength length, FlexLayoutContext *context, float relativeTo) {
    if (flex_isAbsolute(length)) {
        return flex_absoluteValue(length, context);
    }
    else if (length.type == FlexLengthTypePercent && FlexIsResolved(relativeTo)) {
        return length.value / 100 * relativeTo;
    }
    
    return NAN;
}

static inline float flex_auto_to_0(float value) {
    return FlexIsResolved(value) ? value : 0;
}

static inline float flex_clamp(float value, float minValue, float maxValue) {
    if (FlexIsUndefined(value)) {
        return value;
    }
    
    if (FlexIsUndefined(maxValue)) {
        return fmaxf(value, minValue);
    } else {
        return fmaxf(fminf(value, maxValue), minValue);
    }
}

static inline float flex_clampMax(float value, float maxValue) {
    if (FlexIsUndefined(value)) {
        return value;
    }
    
    if (FlexIsUndefined(maxValue)) {
        return value;
    } else {
        return fminf(value, maxValue);
    }
}

static inline float flex_inset(float *inset, FlexDirection direction) {
    float inset_start = inset[flex_start[direction]];
    float inset_end = inset[flex_end[direction]];
    return (!FlexIsUndefined(inset_start) ? inset_start : 0) + (!FlexIsUndefined(inset_end) ? inset_end : 0);
}

static inline bool flex_hasAutoMargin(FlexNodeRef node, FlexDirection direction) {
    return FlexIsUndefined(node->resolvedMargin[flex_start[direction]]) || FlexIsUndefined(node->resolvedMargin[flex_end[direction]]);
}

void flex_resolveMarginAndPadding(FlexNodeRef node, FlexLayoutContext *context, float widthOfContainingBlock, FlexDirection parentDirection) {
    bool isMainHorizontal = parentDirection == FlexHorizontal || parentDirection == FlexHorizontalReverse;
    FlexDirection crossAxis = isMainHorizontal ? FlexVertical : FlexHorizontal;
    node->resolvedMargin[flex_start[parentDirection]] = flex_resolve(!node->fixed && isMainHorizontal && node->margin[FLEX_START].type != FlexLengthTypeUndefined ? node->margin[FLEX_START] : node->margin[flex_start[parentDirection]], context, widthOfContainingBlock);
    node->resolvedMargin[flex_end[parentDirection]] = flex_resolve(!node->fixed && isMainHorizontal && node->margin[FLEX_END].type != FlexLengthTypeUndefined ? node->margin[FLEX_END] : node->margin[flex_end[parentDirection]], context, widthOfContainingBlock);
    node->resolvedMargin[flex_start[crossAxis]] = flex_resolve(!node->fixed && !isMainHorizontal && node->margin[FLEX_START].type != FlexLengthTypeUndefined ? node->margin[FLEX_START] : node->margin[flex_start[crossAxis]], context, widthOfContainingBlock);
    node->resolvedMargin[flex_end[crossAxis]] = flex_resolve(!node->fixed && !isMainHorizontal && node->margin[FLEX_END].type != FlexLengthTypeUndefined ? node->margin[FLEX_END] : node->margin[flex_end[crossAxis]], context, widthOfContainingBlock);
    float border[4];
    border[flex_start[parentDirection]] = isMainHorizontal && !FlexIsUndefined(node->border[FLEX_START]) ? node->border[FLEX_START] : node->border[flex_start[parentDirection]];
    border[flex_end[parentDirection]] = isMainHorizontal && !FlexIsUndefined(node->border[FLEX_END]) ? node->border[FLEX_END] : node->border[flex_end[parentDirection]];
    border[flex_start[crossAxis]] = !isMainHorizontal && !FlexIsUndefined(node->border[FLEX_START]) ? node->border[FLEX_START] : node->border[flex_start[crossAxis]];
    border[flex_end[crossAxis]] = !isMainHorizontal && !FlexIsUndefined(node->border[FLEX_END]) ? node->border[FLEX_END] : node->border[flex_end[crossAxis]];
    node->result.padding[flex_start[parentDirection]] = flex_auto_to_0(flex_resolve(isMainHorizontal && node->padding[FLEX_START].type != FlexLengthTypeUndefined ? node->padding[FLEX_START] : node->padding[flex_start[parentDirection]], context, widthOfContainingBlock) + border[flex_start[parentDirection]]);
    node->result.padding[flex_end[parentDirection]] = flex_auto_to_0(flex_resolve(isMainHorizontal && node->padding[FLEX_END].type != FlexLengthTypeUndefined ? node->padding[FLEX_END] : node->padding[flex_end[parentDirection]], context, widthOfContainingBlock) + border[flex_end[parentDirection]]);
    node->result.padding[flex_start[crossAxis]] = flex_auto_to_0(flex_resolve(!isMainHorizontal && node->padding[FLEX_START].type != FlexLengthTypeUndefined ? node->padding[FLEX_START] : node->padding[flex_start[crossAxis]], context, widthOfContainingBlock) + border[flex_start[crossAxis]]);
    node->result.padding[flex_end[crossAxis]] = flex_auto_to_0(flex_resolve(!isMainHorizontal && node->padding[FLEX_END].type != FlexLengthTypeUndefined ? node->padding[FLEX_END] : node->padding[flex_end[crossAxis]], context, widthOfContainingBlock) + border[flex_end[crossAxis]]);
    node->result.margin[0] = node->resolvedMargin[0];
    node->result.margin[1] = node->resolvedMargin[1];
    node->result.margin[2] = node->resolvedMargin[2];
    node->result.margin[3] = node->resolvedMargin[3];
}

void flex_baseline(FlexNodeRef node, float *ascender, float *descender) {
    FlexDirection crossAxis = FlexVertical;
    float _ascender, _descender;
    
    if (!FlexIsUndefined(node->ascender)) {
        _ascender = node->ascender;
        _descender = node->result.size[crossAxis] - _ascender;
    }
    else if (node->baseline) {
        FlexSize size;
        size.width = node->result.size[FLEX_WIDTH] - flex_inset(node->result.padding, FLEX_WIDTH);
        size.height = node->result.size[FLEX_HEIGHT] - flex_inset(node->result.padding, FLEX_HEIGHT);
        _descender = node->baseline(node->context, size) + node->result.padding[flex_end[crossAxis]];
        _ascender = node->result.size[crossAxis] - _descender;
    }
    else {
        _ascender = node->result.size[crossAxis];
        for (int i=0;i<Flex_getChildrenCount(node);i++) {
            FlexNodeRef child = Flex_getChild(node, i);
            if (!child->fixed) {
                flex_baseline(child, NULL, NULL);
                _ascender = child->ascender + (isnan(child->result.position[flex_start[crossAxis]]) ? 0 : child->result.position[flex_start[crossAxis]]);
                break;
            }
        }
        _descender = node->result.size[crossAxis] - _ascender;
    }
    
    node->ascender = _ascender;
    _ascender += FlexIsUndefined(node->resolvedMargin[flex_start[crossAxis]]) ? 0 : node->resolvedMargin[flex_start[crossAxis]];
    _descender += FlexIsUndefined(node->resolvedMargin[flex_end[crossAxis]]) ? 0 : node->resolvedMargin[flex_end[crossAxis]];
    
    if (ascender) *ascender = _ascender;
    if (descender) *descender = _descender;
}

FlexSize flex_measureNode(FlexNodeRef node, FlexSize availableSize) {
    if (node->measure) {
        for (int i = (int)FlexVector_size(FlexMeasureCache, node->measuredSizeCache) - 1; i >= 0; i--) {
            FlexSize key = node->measuredSizeCache->data[i].availableSize;
            FlexSize value = node->measuredSizeCache->data[i].measuredSize;
            if (FlexSizeEquals(availableSize, key)
                || (FlexTreatNanAsInf(availableSize.width) <= FlexTreatNanAsInf(key.width)
                && FlexTreatNanAsInf(availableSize.height) <= FlexTreatNanAsInf(key.height)
                && FlexTreatNanAsInf(availableSize.width) >= value.width
                && FlexTreatNanAsInf(availableSize.height) >= value.height)) {
                return value;
            }
        }
        FlexSize measuredSize = node->measure(node->context, availableSize);
        if (!node->measuredSizeCache) {
            node->measuredSizeCache = FlexVector_new(FlexMeasureCache, 2);
        }
        FlexVector_add(FlexMeasureCache, node->measuredSizeCache, ((FlexMeasureCache){availableSize, measuredSize}));
        return measuredSize;
    }

    return (FlexSize){0,0};
}

bool flex_canUseCache(FlexNodeRef node, FlexSize constrainedSize) {
    return FlexFloatEquals(node->lastConstrainedSize.width, constrainedSize.width)
        && FlexFloatEquals(node->lastConstrainedSize.height, constrainedSize.height)
        && FlexLengthEquals(node->size[FLEX_WIDTH], node->lastSize[FLEX_WIDTH])
        && FlexLengthEquals(node->size[FLEX_HEIGHT], node->lastSize[FLEX_HEIGHT]);
}

void flex_layoutInternal(FlexNodeRef node, FlexLayoutContext *context, FlexSize constrainedSize, FlexLayoutFlags flags, bool isRoot) {
    node->ascender = NAN;

    if (constrainedSize.width < 0) constrainedSize.width = 0;
    if (constrainedSize.height < 0) constrainedSize.height = 0;
    float constrainedWidth = constrainedSize.width;
    float constrainedHeight = constrainedSize.height;
    float resolvedWidth = flex_resolve(node->size[FLEX_WIDTH], context, constrainedWidth);
    float resolvedHeight = flex_resolve(node->size[FLEX_HEIGHT], context, constrainedHeight);
    if (isRoot && !FlexIsResolved(resolvedWidth) && !FlexIsUndefined(constrainedWidth)) resolvedWidth = constrainedWidth;
    if (isRoot && !FlexIsResolved(resolvedHeight) && !FlexIsUndefined(constrainedHeight)) resolvedHeight = constrainedHeight;
    resolvedWidth = flex_clamp(resolvedWidth, flex_resolve(node->minSize[FLEX_WIDTH], context, constrainedWidth), flex_resolve(node->maxSize[FLEX_WIDTH], context, constrainedWidth));
    resolvedHeight = flex_clamp(resolvedHeight, flex_resolve(node->minSize[FLEX_HEIGHT], context, constrainedHeight), flex_resolve(node->maxSize[FLEX_HEIGHT], context, constrainedHeight));
    FlexSize resolvedSize = {resolvedWidth, resolvedHeight};
    FlexSize resolvedInnerSize = {
        FlexIsResolved(resolvedWidth) ? resolvedWidth - flex_inset(node->result.padding, FLEX_WIDTH) : NAN,
        FlexIsResolved(resolvedHeight) ? resolvedHeight - flex_inset(node->result.padding, FLEX_HEIGHT) : NAN
    };
    
    if (FlexIsResolved(resolvedWidth)) node->result.size[FLEX_WIDTH] = resolvedWidth;
    if (FlexIsResolved(resolvedHeight)) node->result.size[FLEX_HEIGHT] = resolvedHeight;
    
    if (flags == FlexLayoutFlagMeasureWidth && FlexIsResolved(resolvedWidth)) {
        node->result.size[FLEX_WIDTH] = resolvedWidth;
        return;
    }
    if (flags == FlexLayoutFlagMeasureHeight && FlexIsResolved(resolvedHeight)) {
        node->result.size[FLEX_HEIGHT] = resolvedHeight;
        return;
    }
    
    float resolvedMarginWidth = flex_inset(node->resolvedMargin, FLEX_WIDTH);
    float resolvedMarginHeight = flex_inset(node->resolvedMargin, FLEX_HEIGHT);
    float resolvedPaddingWidth = flex_inset(node->result.padding, FLEX_WIDTH);
    float resolvedPaddingHeight = flex_inset(node->result.padding, FLEX_HEIGHT);
    FlexSize availableSize;
    availableSize.width = FlexIsResolved(resolvedWidth) ? resolvedWidth - resolvedPaddingWidth : FlexIsUndefined(constrainedWidth) ? NAN : (constrainedWidth - resolvedMarginWidth - resolvedPaddingWidth);
    availableSize.height = FlexIsResolved(resolvedHeight) ? resolvedHeight - resolvedPaddingHeight : FlexIsUndefined(constrainedHeight) ? NAN : (constrainedHeight - resolvedMarginHeight - resolvedPaddingHeight);
    
    if (flex_canUseCache(node, availableSize)) {
        return;
    }
    else if (flags == FlexLayoutFlagLayout) {
        node->lastConstrainedSize = availableSize;
    }
    else {
        node->lastConstrainedSize = FlexCacheSizeUndefined;
    }
    node->lastSize[FLEX_WIDTH] = node->size[FLEX_WIDTH];
    node->lastSize[FLEX_HEIGHT] = node->size[FLEX_HEIGHT];

    // measure non-container element
    if (Flex_getChildrenCount(node) == 0) {
        if (!FlexIsResolved(resolvedWidth) || !FlexIsResolved(resolvedHeight)) {
            FlexSize measuredSize = flex_measureNode(node, availableSize);
            
            if (!FlexIsResolved(resolvedWidth)) {
                node->result.size[FLEX_WIDTH] = flex_clamp(measuredSize.width + flex_inset(node->result.padding, FLEX_WIDTH), flex_resolve(node->minSize[FLEX_WIDTH], context, constrainedWidth), flex_resolve(node->maxSize[FLEX_WIDTH], context, constrainedWidth));
            }
            if (!FlexIsResolved(resolvedHeight)) {
                node->result.size[FLEX_HEIGHT] = flex_clamp(measuredSize.height + flex_inset(node->result.padding, FLEX_HEIGHT), flex_resolve(node->minSize[FLEX_HEIGHT], context, constrainedHeight), flex_resolve(node->maxSize[FLEX_HEIGHT], context, constrainedHeight));
            }
        }
        return;
    }

    // layout flex container
    
    bool isReverse = node->direction == FlexHorizontalReverse || node->direction == FlexVerticalReverse;
    bool isWrapReverse = node->wrap == FlexWrapReverse;
    FlexDirection mainAxis = flex_dim[node->direction];
    FlexDirection crossAxis = mainAxis == FlexHorizontal ? FlexVertical : FlexHorizontal;
    
    size_t i, j;
    
    //  [flex items ----------->|<----------- fixed items]
    FlexNodeRef* items = (FlexNodeRef*)malloc(sizeof(FlexNode) * Flex_getChildrenCount(node));
    int flexItemsCount = 0;
    int fixedItemsCount = 0;
    
    for (i=0;i<Flex_getChildrenCount(node);i++) {
        FlexNodeRef item = Flex_getChild(node, i);
        // resolve margin and padding for each items
        flex_resolveMarginAndPadding(item, context, resolvedInnerSize.width, node->direction);
        if (item->fixed) {
            items[Flex_getChildrenCount(node) - ++fixedItemsCount] = item;
        }
        else {
            items[flexItemsCount++] = item;
        }
    }
    
    // reference: http://www.w3.org/TR/css3-flexbox/#layout-algorithm
    
    ///////////////////////////////////////////////////////////////////////////
    //  Line Length Determination
    
    // 2. Determine the available main and cross space for the flex items. For each dimension, if that dimension of the flex container’s content box is a definite size, use that; otherwise, subtract the flex container’s margin, border, and padding from the space available to the flex container in that dimension and use that value. This might result in an infinite value.
    
    float resolvedMinCrossSize = flex_resolve(node->minSize[crossAxis], context, constrainedSize.size[crossAxis]);
    float resolvedMaxCrossSize = flex_resolve(node->maxSize[crossAxis], context, constrainedSize.size[crossAxis]);
    float resolvedMinMainSize = flex_resolve(node->minSize[mainAxis], context, constrainedSize.size[mainAxis]);
    float resolvedMaxMainSize = flex_resolve(node->maxSize[mainAxis], context, constrainedSize.size[mainAxis]);
    FlexSize resolvedMarginSize = {resolvedMarginWidth, resolvedMarginHeight};
    FlexSize resolvedPaddingSize = {resolvedPaddingWidth, resolvedPaddingHeight};
    
    FlexLine* lines = (FlexLine*)malloc(sizeof(FlexLine) * (flexItemsCount > 0 ? flexItemsCount : 1));
    lines[0].itemsCount = 0;
    lines[0].itemsSize = 0;
    size_t linesCount = 1;
    
    float spacing = flex_auto_to_0(flex_resolve(node->spacing, context, resolvedInnerSize.size[mainAxis]));
    float lineSpacing = flex_auto_to_0(flex_resolve(node->lineSpacing, context, resolvedInnerSize.size[crossAxis]));
    
    // 3. Determine the flex base size and hypothetical main size of each item:
    for (i=0;i<flexItemsCount;i++) {
        FlexNodeRef item = items[i];
        
        float itemHypotheticalMainSize;
        float resolvedFlexBasis = flex_resolve(item->calculatedFlexBasis, context, resolvedInnerSize.size[mainAxis]);
        
        // A. If the item has a definite used flex basis, that’s the flex base size.
        if (FlexIsResolved(resolvedFlexBasis)) {
            item->flexBaseSize = resolvedFlexBasis;
        }
        
        // B. If the flex item has ...
        //      * an intrinsic aspect ratio,
        //      * a used flex basis of content, and
        //      * a definite cross size
        //    then the flex base size is calculated from its inner cross size and the flex item’s intrinsic aspect ratio.
        // TODO supports aspect ratio ?
        
        // E. Otherwise, size the item into the available space using its used flex basis in place of its main size, treating a value of content as max-content. If a cross size is needed to determine the main size (e.g. when the flex item’s main size is in its block axis) and the flex item’s cross size is auto and not definite, in this calculation use fit-content as the flex item’s cross size. The flex base size is the item’s resulting main size.
        else {
            FlexLength oldMainSize = item->size[mainAxis];
            item->size[mainAxis] = item->calculatedFlexBasis;
            flex_layoutInternal(item, context, availableSize, mainAxis == FlexHorizontal ? FlexLayoutFlagMeasureWidth : FlexLayoutFlagMeasureHeight, false);
            item->size[mainAxis] = oldMainSize;
            item->flexBaseSize = item->result.size[mainAxis];
        }
        
        // When determining the flex base size, the item’s min and max main size properties are ignored (no clamping occurs).
        // The hypothetical main size is the item’s flex base size clamped according to its min and max main size properties.
        itemHypotheticalMainSize = flex_clamp(item->flexBaseSize, flex_resolve(item->minSize[mainAxis], context, resolvedInnerSize.size[mainAxis]), flex_resolve(item->maxSize[mainAxis], context, resolvedInnerSize.size[mainAxis]));
        item->result.size[mainAxis] = itemHypotheticalMainSize;
        float outerItemHypotheticalMainSize = itemHypotheticalMainSize + flex_inset(item->resolvedMargin, mainAxis);
        
        ///////////////////////////////////////////////////////////////////////////
        //  Main Size Determination
        
        // 5. Collect flex items into flex lines:
        //     * If the flex container is single-line, collect all the flex items into a single flex line.
        if (!node->wrap) {
            if (lines[linesCount - 1].itemsCount > 0) lines[linesCount - 1].itemsSize += spacing;
            lines[linesCount - 1].itemsSize += outerItemHypotheticalMainSize;
            lines[linesCount - 1].itemsCount++;
        }
        //     * Otherwise, starting from the first uncollected item, collect consecutive items one by one until the first time that the next collected item would not fit into the flex container’s inner main size (or until a forced break is encountered, see §10 Fragmenting Flex Layout). If the very first uncollected item wouldn’t fit, collect just it into the line.
        /* line break not supported yet */
        //       For this step, the size of a flex item is its outer hypothetical main size.
        //       Repeat until all flex items have been collected into flex lines.
        //       > Note that the "collect as many" line will collect zero-sized flex items onto the end of the previous line even if the last non-zero item exactly "filled up" the line.
        else {
            if (lines[linesCount - 1].itemsCount > 0) outerItemHypotheticalMainSize += spacing;
            // line break
            if ((node->itemsPerLine > 0 && lines[linesCount - 1].itemsCount >= node->itemsPerLine) ||
                lines[linesCount - 1].itemsSize + outerItemHypotheticalMainSize > FlexTreatNanAsInf(availableSize.size[mainAxis]) + 0.001) {
                if (node->lines > 0 && linesCount >= node->lines) {
                    for (int j=i;j<flexItemsCount;j++) {
                        FlexNodeRef item = items[j];
                        item->result.position[0] = 0;
                        item->result.position[1] = 0;
                        item->result.size[0] = -1;
                        item->result.size[1] = -1;
                    }
                    break;
                }
                
                if (lines[linesCount - 1].itemsCount == 0) {
                    lines[linesCount - 1].itemsCount = 1;
                    lines[linesCount - 1].itemsSize = outerItemHypotheticalMainSize;
                }
                else {
                    if (lines[linesCount - 1].itemsCount > 0) outerItemHypotheticalMainSize -= spacing;
                    lines[linesCount].itemsCount = 1;
                    lines[linesCount].itemsSize = outerItemHypotheticalMainSize;
                    linesCount++;
                }
            }
            else {
                lines[linesCount - 1].itemsCount++;
                lines[linesCount - 1].itemsSize += outerItemHypotheticalMainSize;
            }
        }
    }
    
    flex_assert(node->wrap || linesCount == 1);
    
    float itemsMainSize = 0;
    for (i=0;i<linesCount;i++) {
        if (lines[i].itemsSize > itemsMainSize) {
            // lineSize means the item's main size, and it will be used to store the lines' cross size below
            itemsMainSize = lines[i].itemsSize;
        }
    }
    
    // 4. Determine the main size of the flex container using the rules of the formatting context in which it participates. For this computation, auto margins on flex items are treated as 0.
    node->result.size[mainAxis] = FlexIsResolved(resolvedSize.size[mainAxis]) ? resolvedSize.size[mainAxis] : flex_clampMax(itemsMainSize + resolvedPaddingSize.size[mainAxis], !FlexIsUndefined(constrainedSize.size[mainAxis]) ? constrainedSize.size[mainAxis] - resolvedMarginSize.size[mainAxis] : NAN);
    node->result.size[mainAxis] = flex_clamp(node->result.size[mainAxis], resolvedMinMainSize, resolvedMaxMainSize);
    itemsMainSize = node->result.size[mainAxis] - resolvedPaddingSize.size[mainAxis];
    
    if ((flags == FlexLayoutFlagMeasureWidth && mainAxis == FLEX_WIDTH)
        || (flags == FlexLayoutFlagMeasureHeight && mainAxis == FLEX_HEIGHT)) {
        free(lines);
        free(items);
        return;
    }
    
    float innerMainSize = node->result.size[mainAxis] - resolvedPaddingSize.size[mainAxis];
    
    // 6. Resolve the flexible lengths of all the flex items to find their used main size (see section 9.7.).
    bool* isFrozen = (bool*)malloc(sizeof(bool) * flexItemsCount);
    char* violations = (char*)malloc(sizeof(char) * flexItemsCount);
    size_t lineStart = 0;
    for (i=0;i<linesCount;i++) {
        size_t count = lines[i].itemsCount;
        size_t unfrozenCount = count;
        size_t lineEnd = lineStart + count;
        
        // 6.1 Determine the used flex factor. Sum the outer hypothetical main sizes of all items on the line. If the sum is less than the flex container’s inner main size, use the flex grow factor for the rest of this algorithm; otherwise, use the flex shrink factor.
        float lineMainSize = 0;
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            lineMainSize += item->result.size[mainAxis] + flex_inset(item->resolvedMargin, mainAxis);
        }
        float itemsSpacing = (count - 1) * spacing;
        lineMainSize += itemsSpacing;
        bool usingFlexGrowFactor = lineMainSize < FlexTreatNanAsInf(availableSize.size[mainAxis]);
        float initialFreeSpace = innerMainSize - itemsSpacing;
        
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            // 6.2 Size inflexible items. Freeze, setting its target main size to its hypothetical main size…
            //       * any item that has a flex factor of zero
            //       * if using the flex grow factor: any item that has a flex base size greater than its hypothetical main size
            //       * if using the flex shrink factor: any item that has a flex base size smaller than its hypothetical main size
            // 6.3 Calculate initial free space. Sum the outer sizes of all items on the line, and subtract this from the flex container’s inner main size. For frozen items, use their outer target main size; for other items, use their outer flex base size.
            if ((usingFlexGrowFactor ? item->flexGrow : item->flexShrink * item->flexBaseSize) == 0
                || (usingFlexGrowFactor && item->flexBaseSize > item->result.size[mainAxis])
                || (!usingFlexGrowFactor && item->flexBaseSize < item->result.size[mainAxis])) {
                isFrozen[j] = true;
                unfrozenCount--;
                initialFreeSpace -= item->result.size[mainAxis] + flex_inset(item->resolvedMargin, mainAxis);
            } else {
                isFrozen[j] = false;
                initialFreeSpace -= item->flexBaseSize + flex_inset(item->resolvedMargin, mainAxis);
            }
        }
        
        // 6.4 Loop:
        while (true) {
            // a. Check for flexible items. If all the flex items on the line are frozen, free space has been distributed; exit this loop.
            if (unfrozenCount == 0) {
                break;
            }
            
            // b. Calculate the remaining free space as for initial free space, above. If the sum of the unfrozen flex items’ flex factors is less than one, multiply the initial free space by this sum. If the magnitude of this value is less than the magnitude of the remaining free space, use this as the remaining free space.
            float remainingFreeSpace = fmax(0.f, innerMainSize - itemsSpacing);
            float unFrozenFlexFactors = 0;
            
            for (j=lineStart;j<lineEnd;j++) {
                FlexNodeRef item = items[j];
                if (isFrozen[j]) {
                    remainingFreeSpace -= item->result.size[mainAxis] + flex_inset(item->resolvedMargin, mainAxis);
                }
                else {
                    remainingFreeSpace -= item->flexBaseSize + flex_inset(item->resolvedMargin, mainAxis);
                    unFrozenFlexFactors += usingFlexGrowFactor ? item->flexGrow : item->flexShrink;
                }
            }
            
            if (unFrozenFlexFactors < 1) {
                float value = initialFreeSpace * unFrozenFlexFactors;
                if (value < remainingFreeSpace) {
                    remainingFreeSpace = value;
                }
            }
            
            // c. Distribute free space proportional to the flex factors.
            //    If the remaining free space is zero
            //         Do nothing.
            if (remainingFreeSpace == 0) {
                
            }
            //    If using the flex grow factor
            //         Find the ratio of the item’s flex grow factor to the sum of the flex grow factors of all unfrozen items on the line. Set the item’s target main size to its flex base size plus a fraction of the remaining free space proportional to the ratio.
            else if (usingFlexGrowFactor && unFrozenFlexFactors > 0) {
                for (j=lineStart;j<lineEnd;j++) {
                    FlexNodeRef item = items[j];
                    if (!isFrozen[j]) {
                        float ratio = item->flexGrow / unFrozenFlexFactors;
                        item->result.size[mainAxis] = item->flexBaseSize + remainingFreeSpace * ratio;
                    }
                }
            }
            //    If using the flex shrink factor
            //         For every unfrozen item on the line, multiply its flex shrink factor by its inner flex base size, and note this as its scaled flex shrink factor. Find the ratio of the item’s scaled flex shrink factor to the sum of the scaled flex shrink factors of all unfrozen items on the line. Set the item’s target main size to its flex base size minus a fraction of the absolute value of the remaining free space proportional to the ratio. Note this may result in a negative inner main size; it will be corrected in the next step.
            else if (!usingFlexGrowFactor && unFrozenFlexFactors > 0) {
                float unFrozenScaledFlexFactors = 0;
                for (j=lineStart;j<lineEnd;j++) {
                    FlexNodeRef item = items[j];
                    if (!isFrozen[j]) {
                        unFrozenScaledFlexFactors += item->flexShrink * item->flexBaseSize;
                    }
                }
                for (j=lineStart;j<lineEnd;j++) {
                    FlexNodeRef item = items[j];
                    if (!isFrozen[j]) {
                        float ratio = item->flexShrink * item->flexBaseSize / unFrozenScaledFlexFactors;
                        item->result.size[mainAxis] = item->flexBaseSize + remainingFreeSpace * ratio;
                    }
                }
            }
            //    Otherwise
            //         Do nothing.
            
            // d. Fix min/max violations. Clamp each non-frozen item’s target main size by its min and max main size properties. If the item’s target main size was made smaller by this, it’s a max violation. If the item’s target main size was made larger by this, it’s a min violation.
            float totalViolation = 0;
            for (j=lineStart;j<lineEnd;j++) {
                FlexNodeRef item = items[j];
                if (!isFrozen[j]) {
                    float targetMainSize = item->result.size[mainAxis];
                    float clampedSize = flex_clamp(targetMainSize, flex_resolve(item->minSize[mainAxis], context, resolvedInnerSize.size[mainAxis]), flex_resolve(item->maxSize[mainAxis], context, resolvedInnerSize.size[mainAxis]));
                    violations[j] = clampedSize == targetMainSize ? 0 : clampedSize < targetMainSize ? 1 : -1;    // 1/-1 means max/min
                    totalViolation += clampedSize - targetMainSize;
                    item->result.size[mainAxis] = clampedSize;
                }
            }
            
            // e. Freeze over-flexed items. The total violation is the sum of the adjustments from the previous step ∑(clamped size - unclamped size). If the total violation is:
            //    Zero
            //         Freeze all items.
            if (totalViolation == 0) {
                unfrozenCount = 0;
            }
            //    Positive
            //         Freeze all the items with min violations.
            else if (totalViolation > 0) {
                for (j=lineStart;j<lineEnd;j++) {
                    if (!isFrozen[j]) {
                        if (violations[j] < 0) {
                            isFrozen[j] = true;
                            unfrozenCount--;
                        }
                    }
                }
            }
            //    Negative
            //         Freeze all the items with max violations.
            else {
                for (j=lineStart;j<lineEnd;j++) {
                    if (!isFrozen[j]) {
                        if (violations[j] > 0) {
                            isFrozen[j] = true;
                            unfrozenCount--;
                        }
                    }
                }
            }
            
            // f. Return to the start of this loop.
        }
        lineStart = lineEnd;
    }
    free(isFrozen);
    free(violations);
    
    ///////////////////////////////////////////////////////////////////////////
    //  Cross Size Determination
    
    // 7. Determine the hypothetical cross size of each item by performing layout with the used main size and the available space, treating auto as fit-content.
    for (i=0;i<flexItemsCount;i++) {
        FlexNodeRef item = items[i];
        FlexLength oldMainSize = item->size[mainAxis];
        item->size[mainAxis] = (FlexLength){item->result.size[mainAxis], FlexLengthTypePoint};
        flex_layoutInternal(item, context, availableSize, flags, false);
        item->size[mainAxis] = oldMainSize;
    }
    
    // 8. Calculate the cross size of each flex line.
    //    If the flex container is single-line and has a definite cross size, the cross size of the flex line is the flex container’s inner cross size.
    float itemsCrossSize = 0;
    float resolvedInnerCrossSize = FlexIsResolved(resolvedSize.size[crossAxis]) ? resolvedSize.size[crossAxis] - flex_inset(node->result.padding, crossAxis) : NAN;
    if (!node->wrap && FlexIsResolved(resolvedSize.size[crossAxis])) {
        itemsCrossSize = resolvedInnerCrossSize;
        lines[0].itemsSize = itemsCrossSize;
        
        // prepare for baseline alignment
        float maxAscender = 0;
        for (j=0;j<lines[0].itemsCount;j++) {
            FlexNodeRef item = items[j];
            if (mainAxis == FlexHorizontal && item->calculatedAlignSelf == FlexBaseline && !flex_hasAutoMargin(item, crossAxis)) {
                float ascender;
                flex_baseline(item, &ascender, NULL);
                
                if (ascender > maxAscender) {
                    maxAscender = ascender;
                }
            }
        }
        lines[0].ascender = maxAscender;
    }
    //Otherwise, for each flex line:
    else {
        lineStart = 0;
        for (i=0;i<linesCount;i++) {
            size_t lineEnd = lineStart + lines[i].itemsCount;
            
            float maxOuterHypotheticalCrossSize = 0;
            float maxAscender = 0;
            float maxDescender = 0;
            for (j=lineStart;j<lineEnd;j++) {
                FlexNodeRef item = items[j];
                
                //    1. Collect all the flex items whose inline-axis is parallel to the main-axis, whose align-self is baseline, and whose cross-axis margins are both non-auto. Find the largest of the distances between each item’s baseline and its hypothetical outer cross-start edge, and the largest of the distances between each item’s baseline and its hypothetical outer cross-end edge, and sum these two values.
                // only the horizontal text layout is supported
                if (mainAxis == FlexHorizontal && item->calculatedAlignSelf == FlexBaseline && !flex_hasAutoMargin(item, crossAxis)) {
                    float ascender, descender;
                    flex_baseline(item, &ascender, &descender);
                    
                    if (ascender > maxAscender) maxAscender = ascender;
                    if (descender > maxDescender) maxDescender = descender;
                }
                //    2. Among all the items not collected by the previous step, find the largest outer hypothetical cross size.
                else {
                    float itemOuterHypotheticalCrossSize = item->result.size[crossAxis] + flex_inset(item->resolvedMargin, crossAxis);
                    if (itemOuterHypotheticalCrossSize > maxOuterHypotheticalCrossSize) {
                        maxOuterHypotheticalCrossSize = itemOuterHypotheticalCrossSize;
                    }
                }
            }
            
            lines[i].ascender = maxAscender;
            
            //    3. The used cross-size of the flex line is the largest of the numbers found in the previous two steps and zero.
            float usedCrossSize = fmax(0.f, fmax(maxOuterHypotheticalCrossSize, maxAscender + maxDescender));
            
            //       If the flex container is single-line, then clamp the line’s cross-size to be within the container’s computed min and max cross-size properties. Note that if CSS 2.1’s definition of min/max-width/height applied more generally, this behavior would fall out automatically.
            if (!node->wrap) {
                usedCrossSize = flex_clamp(usedCrossSize, resolvedMinCrossSize, resolvedMaxCrossSize);
            }
            if (i > 0) itemsCrossSize += lineSpacing;
            itemsCrossSize += usedCrossSize;
            lines[i].itemsSize = usedCrossSize;
            
            lineStart = lineEnd;
        }
    }
    
    node->ascender = lines[0].ascender ? lines[0].ascender + node->result.padding[flex_start[crossAxis]] : NAN;
    
    if (flags == FlexLayoutFlagLayout) {
    // 9. Handle 'align-content: stretch'. If the flex container has a definite cross size, align-content is stretch, and the sum of the flex lines' cross sizes is less than the flex container’s inner cross size, increase the cross size of each flex line by equal amounts such that the sum of their cross sizes exactly equals the flex container’s inner cross size.
    if (FlexIsResolved(resolvedSize.size[crossAxis]) && node->alignContent == FlexStretch && itemsCrossSize < FlexTreatNanAsInf(resolvedInnerCrossSize)) {
        float increment = (resolvedInnerCrossSize - itemsCrossSize) / linesCount;
        for (i=0;i<linesCount;i++) {
            lines[i].itemsSize += increment;
        }
    }
    
    // 10. Collapse visibility:collapse items. If any flex items have visibility: collapse, note the cross size of the line they’re in as the item’s strut size, and restart layout from the beginning.
    //     In this second layout round, when collecting items into lines, treat the collapsed items as having zero main size. For the rest of the algorithm following that step, ignore the collapsed items entirely (as if they were display:none) except that after calculating the cross size of the lines, if any line’s cross size is less than the largest strut size among all the collapsed items in the line, set its cross size to that strut size.
    //     Skip this step in the second layout round.
    // TODO supports visibility property ?
    
    // 11. Determine the used cross size of each flex item. If a flex item has align-self: stretch, its computed cross size property is auto, and neither of its cross-axis margins are auto, the used outer cross size is the used cross size of its flex line, clamped according to the item’s min and max cross size properties. Otherwise, the used cross size is the item’s hypothetical cross size.
    //     If the flex item has align-self: stretch, redo layout for its contents, treating this used size as its definite cross size so that percentage-sized children can be resolved.
    //     > Note that this step does not affect the main size of the flex item, even if it has an intrinsic aspect ratio.
    lineStart = 0;
    for (i=0;i<linesCount;i++) {
        size_t lineEnd = lineStart + lines[i].itemsCount;
        
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            
            FlexLength oldCrossSize = item->size[crossAxis];
            if (item->calculatedAlignSelf == FlexStretch && item->size[crossAxis].type == FlexLengthTypeAuto && !flex_hasAutoMargin(item, crossAxis)) {
                item->size[crossAxis].type = FlexLengthTypePoint;
                item->size[crossAxis].value = item->result.size[crossAxis] = lines[i].itemsSize - flex_inset(item->resolvedMargin, crossAxis);
            }
            
            item->result.size[crossAxis] = flex_clamp(item->result.size[crossAxis], flex_resolve(item->minSize[crossAxis], context, resolvedInnerSize.size[crossAxis]), flex_resolve(item->maxSize[crossAxis], context, resolvedInnerSize.size[crossAxis]));
            
            if (item->calculatedAlignSelf == FlexStretch) {
                FlexLength oldMainSize = item->size[mainAxis];
                item->size[crossAxis] = (FlexLength){item->result.size[crossAxis], FlexLengthTypePoint};
                item->size[mainAxis] = (FlexLength){item->result.size[mainAxis], FlexLengthTypePoint};
                flex_layoutInternal(item, context, availableSize, flags, false);
                item->size[mainAxis] = oldMainSize;
            }
            item->size[crossAxis] = oldCrossSize;
        }
        
        lineStart = lineEnd;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  Main-Axis Alignment
    
    // 12. Distribute any remaining free space. For each flex line:
    lineStart = 0;
    for (i=0;i<linesCount;i++) {
        size_t lineEnd = lineStart + lines[i].itemsCount;
        
        float lineMainSize = 0;
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            lineMainSize += item->result.size[mainAxis] + flex_inset(item->resolvedMargin, mainAxis);
        }
        lineMainSize += (lines[i].itemsCount - 1) * spacing;
        
        float remainMainSize = innerMainSize - lineMainSize;
        //       1. If the remaining free space is positive and at least one main-axis margin on this line is auto, distribute the free space equally among these margins.
        if (remainMainSize > 0) {
            size_t count = 0;
            for (j=lineStart;j<lineEnd;j++) {
                FlexNodeRef item = items[j];
                if (FlexIsUndefined(item->resolvedMargin[flex_start[node->direction]])) count++;
                if (FlexIsUndefined(item->resolvedMargin[flex_end[node->direction]])) count++;
            }
            if (count > 0) {
                float autoMargin = remainMainSize / count;
                remainMainSize = 0;
                for (j=lineStart;j<lineEnd;j++) {
                    FlexNodeRef item = items[j];
                    if (FlexIsUndefined(item->resolvedMargin[flex_start[node->direction]])) item->result.margin[flex_start[node->direction]] = autoMargin;
                    if (FlexIsUndefined(item->resolvedMargin[flex_end[node->direction]])) item->result.margin[flex_end[node->direction]] = autoMargin;
                }
            }
        }
        //          Otherwise, set all auto margins to zero.
        else {
            for (j=lineStart;j<lineEnd;j++) {
                FlexNodeRef item = items[j];
                if (FlexIsUndefined(item->resolvedMargin[flex_start[node->direction]])) item->result.margin[flex_start[node->direction]] = 0;
                if (FlexIsUndefined(item->resolvedMargin[flex_end[node->direction]])) item->result.margin[flex_end[node->direction]] = 0;
            }
        }
        
        //       2. Align the items along the main-axis per justify-content.
        float offsetStart = node->result.padding[flex_start[node->direction]];
        float offsetStep = 0;
        switch (node->justifyContent) {
            case FlexStart:
                break;
            case FlexCenter:
                offsetStart += remainMainSize / 2;
                break;
            case FlexEnd:
                offsetStart += remainMainSize;
                break;
            case FlexSpaceBetween:
                offsetStep = remainMainSize / (lines[i].itemsCount - 1);
                break;
            case FlexSpaceAround:
                offsetStep = remainMainSize / lines[i].itemsCount;
                offsetStart += offsetStep / 2;
                break;
            default:
                flex_assert(false);
        }
        
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            offsetStart += item->result.margin[flex_start[node->direction]];
            item->result.position[flex_start[mainAxis]] = isReverse ? node->result.size[mainAxis] - offsetStart - item->result.size[mainAxis] : offsetStart;
            offsetStart += item->result.size[mainAxis] + item->result.margin[flex_end[node->direction]] + offsetStep + spacing;
        }
        
        lineStart = lineEnd;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  Cross-Axis Alignment
    
    // 13. Resolve cross-axis auto margins. If a flex item has auto cross-axis margins:
    //       * If its outer cross size (treating those auto margins as zero) is less than the cross size of its flex line, distribute the difference in those sizes equally to the auto margins.
    lineStart = 0;
    for (i=0;i<linesCount;i++) {
        size_t lineEnd = lineStart + lines[i].itemsCount;
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            
            float remindCrossSize = lines[i].itemsSize - (item->result.size[crossAxis] + flex_inset(item->resolvedMargin, crossAxis));
            if (remindCrossSize > 0) {
                if (FlexIsUndefined(item->resolvedMargin[flex_start[crossAxis]]) && FlexIsUndefined(item->resolvedMargin[flex_end[crossAxis]])) {
                    item->result.margin[flex_start[crossAxis]] = item->result.margin[flex_end[crossAxis]] = remindCrossSize / 2;
                } else if (FlexIsUndefined(item->resolvedMargin[flex_start[crossAxis]])) {
                    item->result.margin[flex_start[crossAxis]] = remindCrossSize;
                } else if (FlexIsUndefined(item->resolvedMargin[flex_end[crossAxis]])) {
                    item->result.margin[flex_end[crossAxis]] = remindCrossSize;
                }
            }
            //       * Otherwise, if the block-start or inline-start margin (whichever is in the cross axis) is auto, set it to zero. Set the opposite margin so that the outer cross size of the item equals the cross size of its flex line.
            else {
                if (FlexIsUndefined(item->resolvedMargin[flex_start[crossAxis]])) {
                    item->result.margin[flex_start[crossAxis]] = 0;
                }
//                item->result.margin[flex_end[crossAxis]] = elementsSizeInLine[i] - (item->result.size[crossAxis] + item->result.margin[flex_start[crossAxis]]);
            }
            
            // 14. Align all flex items along the cross-axis per align-self, if neither of the item’s cross-axis margins are auto.
            remindCrossSize = lines[i].itemsSize - (item->result.size[crossAxis] + flex_inset(item->result.margin, crossAxis));
            float itemResultCrossPosition = item->result.margin[flex_start[crossAxis]];
            switch (item->calculatedAlignSelf) {
                case FlexStart:
                    break;
                case FlexCenter:
                    itemResultCrossPosition += remindCrossSize / 2;
                    break;
                case FlexEnd:
                    itemResultCrossPosition += remindCrossSize;
                    break;
                case FlexStretch:
                    break;
                case FlexBaseline:
                    if (mainAxis == FlexHorizontal && !flex_hasAutoMargin(item, crossAxis)) {
                        itemResultCrossPosition = lines[i].ascender - item->ascender;
                    }
                    break;
                default:
                    flex_assert(false);
                    break;
            }
            item->result.position[flex_start[crossAxis]] = isWrapReverse ? lines[i].itemsSize - itemResultCrossPosition - item->result.size[crossAxis] : itemResultCrossPosition;
        }
        
        lineStart = lineEnd;
    }
    }
    
    // 15. Determine the flex container’s used cross size:
    //       * If the cross size property is a definite size, use that, clamped by the min and max cross size properties of the flex container.
    if (FlexIsResolved(resolvedSize.size[crossAxis])) {
        node->result.size[crossAxis] = flex_clamp(resolvedSize.size[crossAxis], resolvedMinCrossSize, resolvedMaxCrossSize);
    }
    //       * Otherwise, use the sum of the flex lines' cross sizes, clamped by the min and max cross size properties of the flex container.
    else {
        node->result.size[crossAxis] = flex_clamp(itemsCrossSize + resolvedPaddingSize.size[crossAxis], resolvedMinCrossSize, resolvedMaxCrossSize);
    }
    
    if (flags != FlexLayoutFlagLayout) {
        free(lines);
        free(items);
        return;
    }
    
    float innerCrossSize = node->result.size[crossAxis] - resolvedPaddingSize.size[crossAxis];
    
    // 16. Align all flex lines per align-content.
    lineStart = 0;
    float offsetStart = node->result.padding[flex_start[crossAxis]];
    float offsetStep = 0;
    float remindCrossSize = innerCrossSize - itemsCrossSize;
    switch (node->alignContent) {
        case FlexStretch:
        case FlexStart:
            break;
        case FlexCenter:
            offsetStart += remindCrossSize / 2;
            break;
        case FlexEnd:
            offsetStart += remindCrossSize;
            break;
        case FlexSpaceBetween:
            offsetStep = remindCrossSize / (linesCount - 1);
            break;
        case FlexSpaceAround:
            offsetStep = remindCrossSize / linesCount;
            offsetStart += offsetStep / 2;
            break;
        default:
            flex_assert(false);
    }
    float lineCrossPositionStart = offsetStart;
    for (i=0;i<linesCount;i++) {
        size_t lineEnd = lineStart + lines[i].itemsCount;
        for (j=lineStart;j<lineEnd;j++) {
            FlexNodeRef item = items[j];
            item->result.position[flex_start[crossAxis]] += isWrapReverse ? node->result.size[crossAxis] - lines[i].itemsSize - lineCrossPositionStart : lineCrossPositionStart;
        }
        lineCrossPositionStart += offsetStep + lines[i].itemsSize + lineSpacing;
        lineStart = lineEnd;
    }
    
    // layout fixed items
    for (i=0;i<fixedItemsCount;i++) {
        FlexNodeRef item = items[Flex_getChildrenCount(node) - 1 - i];
        
        FlexSize childConstrainedSize;
        childConstrainedSize.width = node->result.size[FLEX_WIDTH];
        childConstrainedSize.height = node->result.size[FLEX_HEIGHT];
        
        FlexLength oldWidth = item->size[FLEX_WIDTH];
        FlexLength oldHeight = item->size[FLEX_HEIGHT];
        if (item->size[FLEX_WIDTH].type == FlexLengthTypeAuto && !flex_hasAutoMargin(item, FLEX_WIDTH)) {
            item->size[FLEX_WIDTH] = (FlexLength){node->result.size[FLEX_WIDTH] - flex_inset(item->resolvedMargin, FLEX_WIDTH), FlexLengthTypePoint};
        }
        if (item->size[FLEX_HEIGHT].type == FlexLengthTypeAuto && !flex_hasAutoMargin(item, FLEX_HEIGHT)) {
            item->size[FLEX_HEIGHT] = (FlexLength){node->result.size[FLEX_HEIGHT] - flex_inset(item->resolvedMargin, FLEX_HEIGHT), FlexLengthTypePoint};
        }
        flex_layoutInternal(item, context, childConstrainedSize, flags, false);
        item->size[FLEX_WIDTH] = oldWidth;
        item->size[FLEX_HEIGHT] = oldHeight;
        
        float remindWidth = node->result.size[FLEX_WIDTH] - item->result.size[FLEX_WIDTH] - flex_inset(item->resolvedMargin, FLEX_WIDTH);
        float remindHeight = node->result.size[FLEX_HEIGHT] - item->result.size[FLEX_HEIGHT] - flex_inset(item->resolvedMargin, FLEX_HEIGHT);
        if (remindWidth > 0) {
            if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_WIDTH]]) && FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_WIDTH]])) {
                item->result.margin[flex_start[FLEX_WIDTH]] = item->result.margin[flex_end[FLEX_WIDTH]] = remindWidth / 2;
            } else if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_WIDTH]])) {
                item->result.margin[flex_start[FLEX_WIDTH]] = remindWidth;
            } else if (FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_WIDTH]])) {
                item->result.margin[flex_end[FLEX_WIDTH]] = remindWidth;
            }
        } else {
            if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_WIDTH]])) item->result.margin[flex_start[FLEX_WIDTH]] = 0;
            if (FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_WIDTH]])) item->result.margin[flex_end[FLEX_WIDTH]] = 0;
        }
        
        flex_assert(FlexIsResolved(item->result.margin[0]));
        
        if (remindHeight > 0) {
            if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_HEIGHT]]) && FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_HEIGHT]])) {
                item->result.margin[flex_start[FLEX_HEIGHT]] = item->result.margin[flex_end[FLEX_HEIGHT]] = remindHeight / 2;
            } else if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_HEIGHT]])) {
                item->result.margin[flex_start[FLEX_HEIGHT]] = remindHeight;
            } else if (FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_HEIGHT]])) {
                item->result.margin[flex_end[FLEX_HEIGHT]] = remindHeight;
            }
        } else {
            if (FlexIsUndefined(item->resolvedMargin[flex_start[FLEX_HEIGHT]])) item->result.margin[flex_start[FLEX_HEIGHT]] = 0;
            if (FlexIsUndefined(item->resolvedMargin[flex_end[FLEX_HEIGHT]])) item->result.margin[flex_end[FLEX_HEIGHT]] = 0;
        }
        
        item->result.position[flex_start[FLEX_WIDTH]] = item->result.margin[flex_start[FLEX_WIDTH]];
        item->result.position[flex_start[FLEX_HEIGHT]] = item->result.margin[flex_start[FLEX_HEIGHT]];
    }
    
    // pixel snapping
    for (i=0;i<Flex_getChildrenCount(node);i++) {
        FlexNodeRef item = items[i];
        float right = item->result.position[FLEX_LEFT] + item->result.size[FLEX_WIDTH];
        float bottom = item->result.position[FLEX_TOP] + item->result.size[FLEX_HEIGHT];
        item->result.position[FLEX_LEFT] = FlexPixelRound(item->result.position[FLEX_LEFT], context->scale);
        item->result.position[FLEX_TOP] = FlexPixelRound(item->result.position[FLEX_TOP], context->scale);
        item->result.size[FLEX_WIDTH] = FlexPixelRound(item->result.size[FLEX_WIDTH], context->scale);//FlexPixelRound(right - item->result.position[FLEX_LEFT], context->scale);
        item->result.size[FLEX_HEIGHT] = FlexPixelRound(item->result.size[FLEX_HEIGHT], context->scale);//FlexPixelRound(bottom - item->result.position[FLEX_TOP], context->scale);
    }
    
    free(items);
    free(lines);
}

void flex_setupProperties(FlexNodeRef node) {
//    flex_assert(node->size[FLEX_WIDTH].value >= 0);
//    flex_assert(node->size[FLEX_HEIGHT].value >= 0);
//    flex_assert(node->minSize[FLEX_WIDTH].value >= 0);
//    flex_assert(node->minSize[FLEX_HEIGHT].value >= 0);
//    flex_assert(node->maxSize[FLEX_WIDTH].value >= 0);
//    flex_assert(node->maxSize[FLEX_HEIGHT].value >= 0);
//    flex_assert(node->flexBasis.value >= 0);
    flex_assert(node->alignSelf == FlexInherit || node->alignSelf == FlexStretch || node->alignSelf == FlexStart || node->alignSelf == FlexCenter || node->alignSelf == FlexEnd || node->alignSelf == FlexBaseline);
    flex_assert(node->alignItems == FlexStretch || node->alignItems == FlexStart || node->alignItems == FlexCenter || node->alignItems == FlexEnd || node->alignItems == FlexBaseline);
    flex_assert(node->justifyContent == FlexStart || node->justifyContent == FlexCenter || node->justifyContent == FlexEnd || node->justifyContent == FlexSpaceBetween || node->justifyContent == FlexSpaceAround);
    flex_assert(node->alignContent == FlexStretch || node->alignContent == FlexStart || node->alignContent == FlexCenter || node->alignContent == FlexEnd || node->alignContent == FlexSpaceBetween || node->alignContent == FlexSpaceAround);
    
#if DEBUG
    node->result.position[FLEX_LEFT] = NAN;
    node->result.position[FLEX_TOP] = NAN;
    node->result.size[FLEX_WIDTH] = NAN;
    node->result.size[FLEX_HEIGHT] = NAN;
#endif
    
    for (size_t i=0;i<Flex_getChildrenCount(node);i++) {
        FlexNodeRef item = Flex_getChild(node, i);
        
        if (item->flexBasis.type == FlexLengthTypeAuto) {
            item->calculatedFlexBasis = item->size[flex_dim[node->direction]];
        }
        else if (item->flexBasis.type == FlexLengthTypeContent) {
            item->calculatedFlexBasis = FlexLengthAuto;
        }
        else {
            item->calculatedFlexBasis = item->flexBasis;
        }
        
        if (item->alignSelf == FlexInherit) {
            item->calculatedAlignSelf = node->alignItems;
        }
        else {
            item->calculatedAlignSelf = item->alignSelf;
        }
        
        flex_setupProperties(item);
    }
}

void Flex_layout(FlexNodeRef node, float constrainedWidth, float constrainedHeight, float scale) {
    FlexLayoutContext context;
    context.scale = scale;
    
    flex_setupProperties(node);
    flex_resolveMarginAndPadding(node, &context, constrainedWidth, FlexVertical);
    node->result.margin[FLEX_LEFT] = FlexIsResolved(node->resolvedMargin[FLEX_LEFT]) ? FlexPixelRound(node->resolvedMargin[FLEX_LEFT], scale) : 0;
    node->result.margin[FLEX_TOP] = FlexIsResolved(node->resolvedMargin[FLEX_TOP]) ? FlexPixelRound(node->resolvedMargin[FLEX_TOP], scale) : 0;
    node->result.margin[FLEX_RIGHT] = FlexIsResolved(node->resolvedMargin[FLEX_RIGHT]) ? FlexPixelRound(node->resolvedMargin[FLEX_RIGHT], scale) : 0;
    node->result.margin[FLEX_BOTTOM] = FlexIsResolved(node->resolvedMargin[FLEX_BOTTOM]) ? FlexPixelRound(node->resolvedMargin[FLEX_BOTTOM], scale) : 0;
    FlexSize constrainedSize;
    constrainedSize.width = FlexIsUndefined(constrainedWidth) ? NAN : (constrainedWidth - flex_inset(node->resolvedMargin, FLEX_WIDTH));
    constrainedSize.height = FlexIsUndefined(constrainedHeight) ? NAN : (constrainedHeight - flex_inset(node->resolvedMargin, FLEX_HEIGHT));
    flex_layoutInternal(node, &context, constrainedSize, FlexLayoutFlagLayout, true);
    node->result.position[FLEX_LEFT] = node->result.margin[FLEX_LEFT];
    node->result.position[FLEX_TOP] = node->result.margin[FLEX_TOP];
    node->result.size[FLEX_WIDTH] = FlexPixelRound(node->result.size[FLEX_WIDTH], scale);
    node->result.size[FLEX_HEIGHT] = FlexPixelRound(node->result.size[FLEX_HEIGHT], scale);
}

FlexNodeRef Flex_newNode() {
    FlexNodeRef flexNode = (FlexNodeRef)malloc(sizeof(FlexNode));
    memcpy(flexNode, &defaultFlexNode, sizeof(FlexNode));
    return flexNode;
}

void Flex_freeNode(FlexNodeRef node) {
    if (node->measuredSizeCache) {
        FlexVector_free(FlexMeasureCache, node->measuredSizeCache);
    }
    if (node->children) {
        FlexVector_free(FlexNodeRef, node->children);
    }
    free(node);
}

void Flex_freeNodeRecursive(FlexNodeRef node) {
    for (size_t i = 0; i < Flex_getChildrenCount(node); i++) {
        Flex_freeNodeRecursive(Flex_getChild(node, i));
    }
    Flex_freeNode(node);
}

void Flex_insertChild(FlexNodeRef node, FlexNodeRef child, size_t index) {
    if (!node->children) {
        node->children = FlexVector_new(FlexNodeRef, 4);
    }
    flex_markDirty(node);
    FlexVector_insert(FlexNodeRef, node->children, child, index);
}

void Flex_addChild(FlexNodeRef node, FlexNodeRef child) {
    if (!node->children) {
        node->children = FlexVector_new(FlexNodeRef, 4);
    }
    flex_markDirty(node);
    FlexVector_add(FlexNodeRef, node->children, child);
}

void Flex_removeChild(FlexNodeRef node, FlexNodeRef child) {
    FlexVector_remove(FlexNodeRef, node->children, child);
    flex_markDirty(node);
}

FlexNodeRef Flex_getChild(FlexNodeRef node, size_t index) {
    return node->children->data[index];
}

size_t Flex_getChildrenCount(FlexNodeRef node) {
    return FlexVector_size(FlexNodeRef, node->children);
}


void flex_print_float(float value) {
    printf("%.1f", value);
}

void flex_print_FlexLength(FlexLength value) {
    switch (value.type) {
        case FlexLengthTypePoint:
            printf("%.1f", value.value);
            break;
        case FlexLengthTypePercent:
            printf("%.1f%%", value.value);
            break;
        case FlexLengthTypeAuto:
            printf("auto");
            break;
        case FlexLengthTypeContent:
            printf("content");
            break;
        case FlexLengthTypeUndefined:
            printf("undefined");
            break;
    }
}

void flex_print_FlexWrapMode(FlexWrapMode value) {
    switch (value) {
        case FlexWrap:
            printf("wrap");
            break;
        case FlexNoWrap:
            printf("nowrap");
            break;
        case FlexWrapReverse:
            printf("wrap-reverse");
            break;
    }
}

void flex_print_FlexDirection(FlexDirection value) {
    switch (value) {
        case FlexVertical:
            printf("vertical");
            break;
        case FlexHorizontal:
            printf("horizontal");
            break;
        case FlexVerticalReverse:
            printf("vertical-reverse");
            break;
        case FlexHorizontalReverse:
            printf("horizontal-reverse");
            break;
    }
}

void flex_print_FlexAlign(FlexAlign value) {
    switch (value) {
        case FlexEnd:
            printf("end");
            break;
        case FlexStart:
            printf("start");
            break;
        case FlexCenter:
            printf("center");
            break;
        case FlexInherit:
            printf("auto");
            break;
        case FlexStretch:
            printf("stretch");
            break;
        case FlexBaseline:
            printf("baseline");
            break;
        case FlexSpaceAround:
            printf("space-around");
            break;
        case FlexSpaceBetween:
            printf("space-between");
            break;
    }
}

void flex_print_bool(bool value) {
    printf("%s", value ? "true" : "false");
}

void flex_print_int(int value) {
    printf("%d", value);
}

void flex_printIndent(int indent) {
    for (int i=0;i<indent;i++) {
        printf("  ");
    }
}

void flex_printNode(FlexNodeRef node, FlexPrintOptions options, int indent) {
#define FLEX_PRINT(type, name, field) \
    if (showUnspecified || memcmp(&node->field, &defaultFlexNode.field, sizeof(node->field))){ \
        flex_printIndent(indent); \
        printf(name ": "); \
        flex_print_##type(node->field); \
        printf("\n"); \
    }
#define FLEX_PRINT_INSET(type, name, field) \
    if (showUnspecified || memcmp(&node->field, &defaultFlexNode.field, sizeof(node->field))){ \
        flex_printIndent(indent); \
        printf(name ": "); \
        flex_print_##type(node->field[FLEX_TOP]); \
        printf(", "); \
        flex_print_##type(node->field[FLEX_LEFT]); \
        printf(", "); \
        flex_print_##type(node->field[FLEX_BOTTOM]); \
        printf(", "); \
        flex_print_##type(node->field[FLEX_RIGHT]); \
        printf("\n"); \
    }

    if (options == FlexPrintDefault) {
        options = (FlexPrintOptions)0xffffffff;
    }

    bool showUnspecified = !(options & FlexPrintHideUnspecified);

    flex_printIndent(indent); printf("{\n");
    indent++;
    if (options & FlexPrintStyle) {
        FLEX_PRINT(FlexWrapMode, "wrap", wrap);
        FLEX_PRINT(FlexDirection, "direction", direction);
        FLEX_PRINT(FlexAlign, "align-items", alignItems);
        FLEX_PRINT(FlexAlign, "align-self", alignSelf);
        FLEX_PRINT(FlexAlign, "align-content", alignContent);
        FLEX_PRINT(FlexAlign, "justify-content", justifyContent);
        FLEX_PRINT(FlexLength, "flex-basis", flexBasis);
        FLEX_PRINT(float, "flex-grow", flexGrow);
        FLEX_PRINT(float, "flex-shrink", flexShrink);
        FLEX_PRINT(FlexLength, "width", size[FLEX_WIDTH]);
        FLEX_PRINT(FlexLength, "height", size[FLEX_HEIGHT]);
        FLEX_PRINT(FlexLength, "min-width", minSize[FLEX_WIDTH]);
        FLEX_PRINT(FlexLength, "min-height", minSize[FLEX_HEIGHT]);
        FLEX_PRINT(FlexLength, "max-width", maxSize[FLEX_WIDTH]);
        FLEX_PRINT(FlexLength, "max-height", maxSize[FLEX_HEIGHT]);
        FLEX_PRINT_INSET(FlexLength, "margin", margin);
        FLEX_PRINT_INSET(FlexLength, "padding", padding);
        FLEX_PRINT_INSET(float, "border", border);
        FLEX_PRINT(bool, "fixed", fixed);
        FLEX_PRINT(FlexLength, "spacing", spacing);
        FLEX_PRINT(FlexLength, "line-spacing", lineSpacing);
        FLEX_PRINT(int, "lines", lines);
        FLEX_PRINT(int, "items-per-line", itemsPerLine);
        FLEX_PRINT(bool, "has-measure-func", measure);
        FLEX_PRINT(bool, "has-baseline-func", baseline);
    }
    if (options & FlexPrintResult) {
        FLEX_PRINT(float, "result-x", result.position[FLEX_LEFT]);
        FLEX_PRINT(float, "result-y", result.position[FLEX_TOP]);
        FLEX_PRINT(float, "result-width", result.size[FLEX_WIDTH]);
        FLEX_PRINT(float, "result-height", result.size[FLEX_HEIGHT]);
        FLEX_PRINT_INSET(float, "result-margin", result.margin);
        FLEX_PRINT_INSET(float, "result-padding", result.padding);
    }
    if ((options & FlexPrintChildren) && Flex_getChildrenCount(node) > 0) {
        flex_printIndent(indent); printf("children: [\n");
        for (size_t i=0;i<Flex_getChildrenCount(node);i++) {
            flex_printNode(Flex_getChild(node, i), options, indent + 1);
        }
        flex_printIndent(indent); printf("]\n");
    }
    indent--;
    flex_printIndent(indent); printf("}\n");
}

void Flex_print(FlexNodeRef node, FlexPrintOptions options) {
    flex_printNode(node, options, 0);
}
