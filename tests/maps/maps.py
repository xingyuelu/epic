""" The MIT License (MIT)

    Copyright (c) 2015 Kyle Hollins Wray, University of Massachusetts

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
    the Software, and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

import os
import sys

thisFilePath = os.path.dirname(os.path.realpath(__file__))

sys.path.append(os.path.join(thisFilePath, "..", "..", "python"))
from inertia.harmonic import *
from inertia.harmonic_map import *


#harmonicMapFile = os.path.join(thisFilePath, "images/simple.png")
#harmonicMapFile = os.path.join(thisFilePath, "images/simple_big.png")
#harmonicMapFile = os.path.join(thisFilePath, "images/basic.png")
#harmonicMapFile = os.path.join(thisFilePath, "images/maze_1.png")
#harmonicMapFile = os.path.join(thisFilePath, "images/maze_2.png")
harmonicMapFile = os.path.join(thisFilePath, "images/awesome.png")

harmonicMap = HarmonicMap()
harmonicMap.load(harmonicMapFile)

print(harmonicMap)
harmonicMap.show()

timing = harmonicMap.solve(epsilon=1e-2)

print(harmonicMap)
harmonicMap.show()


