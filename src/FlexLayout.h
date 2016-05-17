
#pragma once

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#ifndef __cplusplus
#   include <stdbool.h>
#endif

static const float FlexUndefined = 999999;
static const float FlexAuto = 999998;

typedef enum {
    FlexHorizontal,
    FlexVertical,
    FlexHorizontalReverse,
    FlexVerticalReverse
} FlexDirection;

static const FlexDirection FLEX_WIDTH = FlexHorizontal;
static const FlexDirection FLEX_HEIGHT = FlexVertical;

typedef enum {
    FlexInherit,
    FlexStretch,
    FlexStart,
    FlexCenter,
    FlexEnd,
    FlexSpaceBetween,
    FlexSpaceAround
} FlexAlign;

typedef struct {
    float size[2];
} FlexSize;

typedef enum {
    FLEX_LEFT = 0,
    FLEX_TOP,
    FLEX_RIGHT,
    FLEX_BOTTOM,
    FLEX_START,
    FLEX_END
} FlexPositionIndex;

typedef struct {
    float position[2];
    float size[2];
    float margin[4];
    float border[4];
} FlexResult;

typedef enum {
    FlexLengthTypeDefault,
    FlexLengthTypePercent,
    FlexLengthTypePx,
    FlexLengthTypeCm,       // 1cm = 96px/2.54
    FlexLengthTypeMm,       // 1mm = 1/10th of 1cm
    FlexLengthTypeQ,        // 1q = 1/40th of 1cm
    FlexLengthTypeIn,       // 1in = 2.54cm = 96px
    FlexLengthTypePc,       // 1pc = 1/6th of 1in
    FlexLengthTypePt,       // 1pt = 1/72th of 1in
//    FlexLengthTypeEm,       // font size of element
//    FlexLengthTypeEx,       // x-height of the element’s font
//    FlexLengthTypeCh,       // width of the "0" (ZERO, U+0030) glyph in the element’s font
//    FlexLengthTypeRem,      // font size of the root element
    FlexLengthTypeVw,       // 1% of viewport’s width
    FlexLengthTypeVh,       // 1% viewport’s height
    FlexLengthTypeVmin,     // 1% of viewport’s smaller dimension
    FlexLengthTypeVmax      // 1% of viewport’s larger dimension
} FlexLengthType;

typedef struct {
    float value;
    FlexLengthType type;
} FlexLength;

inline FlexLength flexLength(float value, FlexLengthType type) {
    FlexLength r;
    r.value = value;
    r.type = type;
    return r;
}

#define FlexLengthZero      flexLength(0, FlexLengthTypeDefault)
#define FlexLengthAuto      flexLength(FlexAuto, FlexLengthTypeDefault)
#define FlexLengthUndefined flexLength(FlexUndefined, FlexLengthTypeDefault)

typedef struct FlexNode {
    bool fixed;
    bool wrap;
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
    FlexLength border[6];       // length
    FlexLength spacing;         // length, percentage(relative to its inner main size)
    FlexLength lineSpacing;     // length, percentage(relative to its inner cross size)
    
    FlexResult result;
    
    // internal fields
    float flexBaseSize;
    float resolvedMargin[4];
    float resolvedPadding[4];
    
    // cache measure results
    void* measuredSizeCache;
    
    void* context;
    size_t childrenCount;
    FlexSize (*measure)(void* context, FlexSize constraintedSize);
    struct FlexNode* (*childAt)(void* context, size_t index);
} FlexNode;

FlexNode* newFlexNode();
void initFlexNode(FlexNode* node);
void freeFlexNode(FlexNode* node);
void layoutFlexNode(FlexNode* node, float constraintedWidth, float constraintedHeight, float scale);

#ifdef __cplusplus
}
#endif
