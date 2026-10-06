# Support {#elements_support}

A steel column base with the manufacturer's dimensions on a plane: base plate, tube, head plate, screws and anchors. `Joint::support` lets its head plate into a column end and drills the column screws, a subtract feature from the joint.

```cpp
explicit Support(const Plane& plane, const std::string& name = "support")
Point column_foot() const
static std::shared_ptr<Joint> Joint::support(const Support& support, const Column& column)
```

![Support](elements/element_support.png)

A support on the xy plane and a 220 square column 400 high on it, joined by its support joint.

\include{lineno} elements/element_support.cpp
