# FlexLayout

[中文介绍](#中文介绍)

`FlexLayout` is an `C` implementation of `Flexible Box` layout. It follows the standard [algorithm](https://www.w3.org/TR/css-flexbox-1/#layout-algorithm) and supports almost all features.

`FlexLayout` is battle tested. Please feel free to use.

### Usage

#### C/C++
```C
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
```

#### Javascript

`FlexLayout` has ported to javascript by using [`nbind`](https://github.com/charto/nbind).

```js
    var root = new flex.Node();
    root.direction = flex.Vertical;

    var child1 = new flex.Node();
    child1.width = new flex.Length(50);
    child1.height = new flex.Length(50);
    root.add(child1);

    var child2 = new flex.Node();
    child2.setMeasure(function(size){return new flex.Size(10, 10);});
    root.add(child2);

    root.layout(NaN, NaN);
    root.print();
```

### Features

- margin/padding
- min/max size
- flex-direction
- wrap
- align-items
- align-self
- align-content
- justify-content
- flex-basis
- flex-grow
- flex-shrink
- percentage

### Additions

`FlexLayout` added some useful properties:

- `spacing / line-spacing` spacing between items/lines
- `fixed` do not participate the flex layout，like `position: absolute`
- `lines` maximum number of lines
- `items-per-line` maximum number of items in each line

### Known Issues

- `min-width`/`min-height` do not support [`auto`](https://www.w3.org/TR/css-flexbox-1/#min-size-auto)
- `order` is not supported

### License

Licensed under the MIT License.

---

## 中文介绍

`FlexLayout` 是 `Flexible Box` 布局算法的 `C` 语言实现。按照标准[算法](https://www.w3.org/TR/css-flexbox-1/#layout-algorithm)实现，几乎支持所有特性。

`FlexLayout` 经过充分测试，请放心食用。

### 使用

#### C/C++
```C
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
```

#### Javascript

`FlexLayout` 使用 [`nbind`](https://github.com/charto/nbind) 移植了 javascript 版本。

```js
    var root = new flex.Node();
    root.direction = flex.Vertical;

    var child1 = new flex.Node();
    child1.width = new flex.Length(50);
    child1.height = new flex.Length(50);
    root.add(child1);

    var child2 = new flex.Node();
    child2.setMeasure(function(size){return new flex.Size(10, 10);});
    root.add(child2);

    root.layout(NaN, NaN);
    root.print();
```

### 支持的特性

- margin/padding
- min/max size
- flex-direction
- wrap
- align-items
- align-self
- align-content
- justify-content
- flex-basis
- flex-grow
- flex-shrink
- 百分比

### 扩展属性

`FlexLayout` 增加了以下扩展属性：

- `spacing / line-spacing` 为每两个元素/行之间增加间距
- `fixed` 元素不参与 flex 布局，类似于 `position: absolute`
- `lines` 最大行数
- `items-per-line` 每行最大元素个数

### 已知问题

- `min-width`/`min-height` 不支持设置为 [`auto`](https://www.w3.org/TR/css-flexbox-1/#min-size-auto)
- 不支持 `order` 属性

### 协议

遵循 MIT 协议。
