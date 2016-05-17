# FlexLayout

This project implements the [CSS Flexible Box](https://www.w3.org/TR/css-flexbox/) layout model follow the [Flex Layout Algorithm](https://www.w3.org/TR/css-flexbox/#layout-algorithm).


# Features

- **percentage** and **unit** supports
- extra **spacing** and **lineSpacing** property

# Usage

- init a node

	```
FlexNode* node = newFlexNode();
initFlexNode(node);
	```

- add children to node  
	set the `childrenCount` and `childAt`
	
- free a node

	```
freeFlexNode(node);
	```

for more details, check the **example**.