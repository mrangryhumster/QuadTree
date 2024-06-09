# QuadTree

`QuadTree` is C++ implementation of [quadtree](https://en.wikipedia.org/wiki/Quadtree).

Features : 
* Header only
* Easy to use
* Fast (well at least i hope so)
* Supports STL compatible custom allocators

Caveats :
* Only integral types for coordinates are supported `is_integral<_T_PositionType>` check
* For very big trees it is recomended to use custom allocator with memory pool support
* Range (0,0) - (2,2) will create tree with origin at (0,0) and width/height (2,2). So constructor arguments max_x max_y it is actually width and height

# Example
## Code
```C++
	//Create tree in coordinates (0,0) - (10,10)
	QuadTree<int> Tree(0, 0, 10, 10);

    	//custom allocator and coordinate type are supported
    	// Example : QuadTree<MyObject,uint16_t,std::allocator<MyObject>>

	//Add elements
	Tree.insert(0, 1, 100);
	Tree.insert(3, 8, 101);
	Tree.insert(9, 0, 102);
	Tree.insert(5, 5, 103);

	//Search element
	int* res = Tree.find(5, 5);
	if (res != nullptr) {
		std::cout << "Found : " << *res << std::endl;
	}

	//Iterate over all elements
	for (auto i : Tree) {
		std::cout << "Tree contains : " << i << std::endl;
	}

	//Erase element
	Tree.erase(5, 5);
	std::cout << "Element at (5,5) erased" << std::endl;

	//Iterate over all elements
	for (auto i : Tree) {
		std::cout << "Tree contains : " << i << std::endl;
	}

	Tree.clear();
```
## Output
```
Found : 103
Tree contains : 100
Tree contains : 102
Tree contains : 101
Tree contains : 103
Element at (5,5) erased
Tree contains : 100
Tree contains : 102
Tree contains : 101
```
