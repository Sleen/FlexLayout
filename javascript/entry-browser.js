var nbind = require('./build/Release/nbind.js');

var ran = false;
var ret = null;

nbind({}, function (err, result) {

    if (ran)
        return;

    ran = true;

    if (err)
        throw err;

    ret = result;

});

if (!ran)
    throw new Error('Failed to load the yoga module - it needed to be loaded synchronously, but didn\'t');

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

module.exports = Object.assign(ret.lib, {
    Undefined: NaN,
}, FlexDirection, FlexWrapMode, FlexAlign, FlexLengthType);
