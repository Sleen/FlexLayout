var flex = require('./entry-node');

function demo() {
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
}

demo();
