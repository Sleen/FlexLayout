## FlexLayout 与 css-layout 的性能比较

使用 css-layout 提供的测试用例 https://github.com/facebook/css-layout/tree/master/benchmarks

#### 用例一、Stack with flex

```c
const CSSNodeRef root = CSSNodeNew();
CSSNodeStyleSetWidth(root, 100);
CSSNodeStyleSetHeight(root, 100);

for (uint32_t i = 0; i < 10; i++) {
  const CSSNodeRef child = CSSNodeNew();
  CSSNodeSetMeasureFunc(child, _measure);
  CSSNodeStyleSetFlex(child, 1);
  CSSNodeInsertChild(root, child, 0);
}
```
|               | css-layout | FlexLayout |
| ------------- | ---------- | ---------- |
| time          |      0.010 |      0.273 |
| measure calls |          0 |         20 |
\* *注：time: 创建node和layout的总耗时; measure calls: 调用measure函数的次数*

因为 FlexLayout 支持设置 flex-basis 且默认为 auto，如果改成 0，measure calls 可降至 10。但是在父容器尺寸确定、flex-basis为0、flex-grow不为0、align-self为stretch的情况下，确实不需要measure，这里FlexLayout有优化空间。

#### 用例二、Align stretch in undefined axis

```c
const CSSNodeRef root = CSSNodeNew();

for (uint32_t i = 0; i < 10; i++) {
  const CSSNodeRef child = CSSNodeNew();
  CSSNodeStyleSetHeight(child, 20);
  CSSNodeSetMeasureFunc(child, _measure);
  CSSNodeInsertChild(root, child, 0);
}
```
|               | css-layout | FlexLayout |
| ------------- | ---------- | ---------- |
| time          |      0.333 |      0.247 |
| measure calls |         10 |         10 |

FlexLayout 与 css-layout 的耗时都波动较大，两者耗时差不多

#### 用例三、Nested flex

```c
const CSSNodeRef root = CSSNodeNew();

for (uint32_t i = 0; i < 10; i++) {
  const CSSNodeRef child = CSSNodeNew();
  CSSNodeSetMeasureFunc(child, _measure);
  CSSNodeStyleSetFlex(child, 1);
  CSSNodeInsertChild(root, child, 0);

  for (uint32_t ii = 0; ii < 10; ii++) {
    const CSSNodeRef grandChild = CSSNodeNew();
    CSSNodeSetMeasureFunc(grandChild, _measure);
    CSSNodeStyleSetFlex(grandChild, 1);
    CSSNodeInsertChild(child, grandChild, 0);
  }
}
```
|               | css-layout | FlexLayout |
| ------------- | ---------- | ---------- |
| time          |      0.474 |      14.53 |
| measure calls |         10 |        500 |

这个用例虽然结果是 css-layout 快了许多，但是我打印了它的布局结果，发现并不对，所有 grand child 的宽高都为 NaN。
FlexLayout 的 measure calls 为 500次，应该尚有优化空间。

----------------------------

layout 的耗时主要在 measure 函数上，在 measure calls 相同的情况下，两者的性能差不多。

css-layout 在大多数情况下有较少的 measure calls，但是 layout 的结果可能会有问题。

在真实的应用场景里，可以预估 FlexLayout 的性能相比 css-layout 较差，但拥有更可靠的结果。
