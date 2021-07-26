# TODO add old versions
# Magic Hypercube Generator
*The product of roughly 6 years of very intermittent iteration and improvement*

## Background
A magic square, somewhat similar to sudoku, is a matrix of integers in which all rows and columns of the matrix add up to the same value. A magic square of a size n * n will contain the digits 1 to n^2, and each row will add up to (n^3 + n) / 2. eg. 

2 | 7 | 6
--|---|--
9 | 5 | 1
4 | 3 |	8

Now technically, squares whose rows and columns add to same the value are actually "semi-magic", and to be truly magic their diagonals must also add the same value. This means that magic squares are a subset of semi magic squares, an "edge-case" if you will, and so because I find the computational problem of generating all semi-magic squares to be more general and interesting, this is what the program actually does. Knowing that, for brevity, I will be referring to semi-magic squares as magic squares for the rest of this document.

## Implementation
(Following explanation is in reference to magic squares of size 4 * 4)

Rather than generating every possible permutation of the matrix's element and checking if each row and column sum to the correct value, this algorithm builds the magic square segment by segment, ensuring each segment's sum is correct before building the next. It currently goes about doing this in somewhat of a "closing shell" approach:

1 | 1 | 1 | 1
--|---|---|--
2 | 3 | 3 | 3
2 | 4 |	5 | 5
2 | 4 | 6 | 7

finding a combination of 4 elements from the total set of [1, 16] that sum to 34 for the top row (segment 1), then 3 elements that complete the left-most column's sum (segment 2), then 3 elements to complete the second row's sum (segment 4), and so on and so forth until the entire square is complete and is printed to the text file. 

This process of completing one segment before moving onto the next operates recursively, with each segment tested with every possible combination of available elements (those not used by previous segments) for validity, each valid combination permuted, and each permutation transitioning to the next segment to complete the same combination and permutation process.

//TODO further explanation of changes

