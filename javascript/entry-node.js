var nbind = require('nbind').init();

var FlexDirection = {
    Horizontal: 0,
    Vertical: 1,
    HorizontalReverse: 2,
    VerticalReverse: 3,
};

var FlexWrapMode = {
    NoWrap: 0,
    Wrap: 1,
    WrapReverse: 2,
};

var FlexAlign = {
    Inherit: 0,
    Stretch: 1,
    Start: 2,
    Center: 3,
    End: 4,
    SpaceBetween: 5,
    SpaceAround: 6,
    Baseline: 7,
};

var FlexLengthType = {
    LengthTypeUndefined: 0,
    LengthTypePoint: 1,
    LengthTypePercent: 2,
    LengthTypeAuto: 3,
    LengthTypeContent: 4,
};

module.exports = Object.assign(nbind.lib, {
    Undefined: NaN,
}, FlexDirection, FlexWrapMode, FlexAlign, FlexLengthType);
